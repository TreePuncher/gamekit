#include "SVOGI.h"

namespace FlexKit
{   /************************************************************************************************/


    GILightingEngine::GILightingEngine(RenderSystem& IN_renderSystem, iAllocator& allocator) :
            renderSystem    { IN_renderSystem },

            octreeBuffer    {
                { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(128 * MEGABYTE))  },
                { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(128 * MEGABYTE))  } },

            gatherSignature     { &allocator },
            dispatchSignature   { &allocator },
            removeSignature     { &allocator }
    {
        gatherSignature.SetParameterAsUAV(0, 0, 0, PIPELINE_DEST_CS);
        gatherSignature.SetParameterAsUAV(1, 1, 0, PIPELINE_DEST_CS);
        gatherSignature.SetParameterAsUAV(2, 2, 0, PIPELINE_DEST_CS);
        gatherSignature.SetParameterAsUINT(3, 4, 0, 0, PIPELINE_DEST_CS);
        gatherSignature.Build(&renderSystem, &allocator);

        dispatchSignature.SetParameterAsUAV(0, 0, 0, PIPELINE_DEST_CS);
        dispatchSignature.SetParameterAsUAV(1, 1, 0, PIPELINE_DEST_CS);
        dispatchSignature.Build(&renderSystem, &allocator);

        DesciptorHeapLayout layout{};
        layout.SetParameterAsShaderUAV(0, 2, 1);

        removeSignature.AllowIA = false;
        removeSignature.AllowSO = false;
        removeSignature.SetParameterAsUAV(0, 0, 0, PIPELINE_DEST_CS);
        removeSignature.SetParameterAsUAV(1, 1, 0, PIPELINE_DEST_CS);
        removeSignature.SetParameterAsDescriptorTable(2, layout, 0, PIPELINE_DEST_CS);
        removeSignature.SetParameterAsUINT(3, 4, 0, 0, PIPELINE_DEST_CS);
        removeSignature.Build(&renderSystem, &allocator);

        draw = renderSystem.CreateIndirectLayout(
            { ILE_DrawCall },
            &allocator);

        dispatch = renderSystem.CreateIndirectLayout({
            { ILE_DispatchCall } },
            &allocator);

        remove = renderSystem.CreateIndirectLayout({
            { ILE_RootDescriptorUINT, IndirectDrawDescription::Constant{.rootParameterIdx = 3, .destinationOffset = 0, .numValues = 4 } },
            { ILE_DispatchCall } },
            &allocator,
            & removeSignature);

        renderSystem.SetDebugName(octreeBuffer[0], "Octree");
        renderSystem.SetDebugName(octreeBuffer[1], "Octree");

        renderSystem.RegisterPSOLoader(VXGI_MARKERASE,                  { &renderSystem.Library.RSDefault, CreateMarkErasePSO });
        renderSystem.RegisterPSOLoader(VXGI_DRAWVOLUMEVISUALIZATION,    { &renderSystem.Library.RSDefault, CreateUpdateVolumeVisualizationPSO});
        renderSystem.RegisterPSOLoader(VXGI_SAMPLEINJECTION,            { &renderSystem.Library.RSDefault, CreateInjectVoxelSamplesPSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERDISPATCHARGS,         { &renderSystem.Library.RSDefault, [this](RenderSystem* rs) { return CreateVXGIGatherDispatchArgsPSO(rs); } });
        renderSystem.RegisterPSOLoader(VXGI_GATHERREMOVEARGS,           { &renderSystem.Library.RSDefault, [this](RenderSystem* rs) { return CreateVXGIEraseDispatchArgsPSO(rs); } });
        renderSystem.RegisterPSOLoader(VXGI_DECREMENTCOUNTER,           { &renderSystem.Library.RSDefault, [this](RenderSystem* rs) { return CreateVXGIDecrementDispatchArgsPSO(rs); } });
        renderSystem.RegisterPSOLoader(VXGI_GATHERDRAWARGS,             { &renderSystem.Library.RSDefault, CreateVXGIGatherDrawArgsPSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERSUBDIVISIONREQUESTS,  { &renderSystem.Library.RSDefault, CreateVXGIGatherSubDRequestsPSO });
        renderSystem.RegisterPSOLoader(VXGI_PROCESSSUBDREQUESTS,        { &renderSystem.Library.RSDefault, CreateVXGIProcessSubDRequestsPSO });
        renderSystem.RegisterPSOLoader(VXGI_INITOCTREE,                 { &renderSystem.Library.RSDefault, CreateVXGI_InitOctree });
        renderSystem.RegisterPSOLoader(VXGI_REMOVENODES,                { &removeSignature, [this](RenderSystem* rs){ return CreateRemovePSO(rs); } });

        gatherDispatchArgs  = renderSystem.GetPSO(VXGI_GATHERDISPATCHARGS);
        gatherDispatchArgs2 = renderSystem.GetPSO(VXGI_GATHERREMOVEARGS);
    }


    /************************************************************************************************/


    GILightingEngine::~GILightingEngine()
    {
        renderSystem.ReleaseResource(octreeBuffer[0]);
        renderSystem.ReleaseResource(octreeBuffer[1]);
    }


    /************************************************************************************************/


    void GILightingEngine::InitializeOctree(FrameGraph& frameGraph)
    {
        struct InitOctree
        {
            FrameResourceHandle octree[2];
            FrameResourceHandle freeList;
        };

        frameGraph.AddNode<InitOctree>(
            InitOctree{},
            [&](FrameGraphNodeBuilder& builder, InitOctree& data)
            {
                data.octree[0] = builder.UnorderedAccess(octreeBuffer[0]);
                data.octree[1] = builder.UnorderedAccess(octreeBuffer[0]);
            },
            [](InitOctree& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ctx.BeginEvent_DEBUG("Init Octree");

                auto Init       = resources.GetPipelineState(VXGI_INITOCTREE);
                auto& rootSig   = resources.renderSystem().Library.RSDefault;

                struct alignas(64) OctTreeNode
                {
                    uint    nodes[8];
                    uint4   volumeCord;
                    uint    data;
                    uint    flags;
                };


                for (uint32_t I = 0; I < 2; I++)
                {
                    DescriptorHeap heap;
                    heap.Init2(ctx, rootSig.GetDescHeap(1), 1, &allocator);
                    heap.SetUAVStructured(ctx, 0, resources.UAV(data.octree[I], ctx), resources.UAV(data.octree[1], ctx), sizeof(OctTreeNode), 0);

                    ctx.SetComputeRootSignature(rootSig);
                    ctx.SetComputeDescriptorTable(5, heap);
                    ctx.Dispatch(Init, { 1, 1, 1 });
                }

                ctx.EndEvent_DEBUG();
            });
    }


    /************************************************************************************************/


    void GILightingEngine::_GatherArgs(FrameResourceHandle source, FrameResourceHandle argsBuffer, ResourceHandler& resources, Context& ctx)
    {
        ctx.SetComputeRootSignature(gatherSignature);

        ctx.SetComputeUnorderedAccessView(0, resources.UAV(source, ctx));
        ctx.SetComputeUnorderedAccessView(1, resources.UAV(argsBuffer, ctx));

        ctx.Dispatch(gatherDispatchArgs, { 1, 1, 1 });
    }


    /************************************************************************************************/


    void GILightingEngine::_GatherArgs2(FrameResourceHandle freeList, FrameResourceHandle octree, FrameResourceHandle argsBuffer, ResourceHandler& resources, Context& ctx)
    {
        ctx.SetComputeRootSignature(gatherSignature);

        ctx.SetComputeUnorderedAccessView(0, resources.UAV(freeList, ctx));
        ctx.SetComputeUnorderedAccessView(1, resources.UAV(octree, ctx));
        ctx.SetComputeUnorderedAccessView(2, resources.UAV(argsBuffer, ctx));

        ctx.Dispatch(gatherDispatchArgs2, { 1, 1, 1 });
    }


    /************************************************************************************************/


    void GILightingEngine::CleanUpPhase(UpdateVoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
    {
        auto markNodes          = resources.GetPipelineState(VXGI_MARKERASE);
        auto removeNodes        = resources.GetPipelineState(VXGI_REMOVENODES);
        auto decrement          = resources.GetPipelineState(VXGI_DECREMENTCOUNTER);
        auto gatherDispatchArgs = resources.GetPipelineState(VXGI_GATHERDISPATCHARGS);
        auto& rootSig           = resources.renderSystem().Library.RSDefault;

        const auto cameraConstantValues = GetCameraConstants(data.camera);
        CBPushBuffer constantBuffer{ data.reserveCB(AlignedSize<Camera::ConstantBuffer>()) };
        const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };

        ctx.BeginEvent_DEBUG("SampleCleanup");

        {
            ctx.BeginEvent_DEBUG("Clear temporaries");
            ctx.SetComputeRootSignature(rootSig);

            // Create Dispatch args
            ctx.DiscardResource(resources.GetResource(data.indirectArgs));
            ctx.DiscardResource(resources.GetResource(data.scratchPad));
            ctx.DiscardResource(resources.GetResource(data.freeList));
            ctx.ClearUAVBuffer(resources.GetResource(data.indirectArgs));
            ctx.ClearUAVBufferRange(resources.GetResource(data.scratchPad), 0, 4096);
            ctx.ClearUAVBufferRange(resources.GetResource(data.freeList), 0, 4096);
            ctx.AddUAVBarrier(resources.GetResource(data.indirectArgs));
            ctx.AddUAVBarrier(resources.GetResource(data.scratchPad));
            ctx.AddUAVBarrier(resources.GetResource(data.freeList));

            ctx.EndEvent_DEBUG();
        }
        {   // Mark Erase Flags
            ctx.BeginEvent_DEBUG("MarkNodes");
            _GatherArgs(data.octree, data.indirectArgs, resources, ctx);


            ctx.SetComputeRootSignature(rootSig);
            ctx.SetPipelineState(markNodes);

            DescriptorHeap srvHeap;
            srvHeap.Init2(ctx, rootSig.GetDescHeap(0), 1, &allocator);
            srvHeap.SetSRV(ctx, 0, resources.NonPixelShaderResource(data.depthTarget, ctx), DeviceFormat::R32_FLOAT);

            DescriptorHeap uavHeap;
            uavHeap.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
            uavHeap.SetUAVStructured(ctx, 1, resources.UAV(data.octree, ctx), resources.GetResource(data.octree), sizeof(OctTreeNode), 0);
            uavHeap.SetUAVStructured(ctx, 2, resources.UAV(data.freeList, ctx), resources.GetResource(data.freeList), sizeof(uint32_t), 0);

            ctx.SetComputeConstantBufferView(0, cameraConstants);
            ctx.SetComputeDescriptorTable(4, srvHeap);
            ctx.SetComputeDescriptorTable(5, uavHeap);

            ctx.ExecuteIndirect(
                resources.IndirectArgs(data.indirectArgs, ctx),
                dispatch);

            ctx.AddUAVBarrier(resources.GetResource(data.octree));
            ctx.AddUAVBarrier(resources.GetResource(data.freeList));

            ctx.EndEvent_DEBUG();
        }
        {   // Remove nodes
            ctx.BeginEvent_DEBUG("RemoveNodes");
            _GatherArgs2(data.freeList, data.octree, data.indirectArgs, resources, ctx);


            DescriptorHeap uavHeap;
            uavHeap.Init2(ctx, rootSig.GetDescHeap(1), 1, &allocator);
            uavHeap.SetUAVStructured(ctx, 1, resources.UAV(data.scratchPad, ctx), resources.GetResource(data.scratchPad), sizeof(OctTreeNode), 0);

            ctx.SetComputeRootSignature(removeSignature);
            ctx.SetComputeUnorderedAccessView(0, resources.UAV(data.octree, ctx));
            ctx.SetComputeUnorderedAccessView(1, resources.UAV(data.freeList, ctx));

            ctx.SetPipelineState(removeNodes);

            ctx.ExecuteIndirect(
                resources.IndirectArgs(data.indirectArgs, ctx),
                remove);

            ctx.AddUAVBarrier(resources.GetResource(data.octree));
            ctx.AddUAVBarrier(resources.GetResource(data.freeList));

            ctx.SetPipelineState(decrement);

            ctx.SetComputeUnorderedAccessView(1, resources.UAV(data.scratchPad, ctx));

            ctx.ExecuteIndirect(
                resources.IndirectArgs(data.indirectArgs, ctx),
                remove, 32);

            ctx.AddUAVBarrier(resources.GetResource(data.octree));

            ctx.EndEvent_DEBUG();
        }
        ctx.EndEvent_DEBUG();
    }


    /************************************************************************************************/


    void GILightingEngine::CreateNodePhase(UpdateVoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
    {
        auto sampleInjection        = resources.GetPipelineState(VXGI_SAMPLEINJECTION);
        auto gatherDispatchArgs     = resources.GetPipelineState(VXGI_GATHERDISPATCHARGS);
        auto gatherSubDRequests     = resources.GetPipelineState(VXGI_GATHERSUBDIVISIONREQUESTS);
        auto processSubDRequests    = resources.GetPipelineState(VXGI_PROCESSSUBDREQUESTS);

        auto& rootSig = resources.renderSystem().Library.RSDefault;
        const auto WH = resources.GetTextureWH(data.depthTarget);

        ctx.BeginEvent_DEBUG("SampleInjection");

        ctx.SetComputeRootSignature(rootSig);

        ctx.DiscardResource(resources.GetResource(data.counters));
        ctx.ClearUAVBuffer(resources.UAV(data.counters, ctx));
        ctx.AddUAVBarrier(resources.GetResource(data.counters));

        DescriptorHeap resourceHeap;
        resourceHeap.Init2(ctx, rootSig.GetDescHeap(0), 1, &allocator);
        resourceHeap.SetSRV(ctx, 0, resources.GetResource(data.depthTarget), DeviceFormat::R32_FLOAT);

        ctx.SetComputeDescriptorTable(4, resourceHeap);
        const auto cameraConstantValues = GetCameraConstants(data.camera);
        CBPushBuffer constantBuffer{ data.reserveCB(AlignedSize<Camera::ConstantBuffer>()) };
        const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };

        ctx.SetComputeConstantBufferView(0, cameraConstants);

        DescriptorHeap uavHeap2;
        uavHeap2.Init2(ctx, rootSig.GetDescHeap(1), 4, &allocator);
        uavHeap2.SetUAVStructured(ctx, 0, resources.UAV(data.sampleBuffer, ctx),    resources.UAV(data.counters, ctx),   32, 0);
        uavHeap2.SetUAVStructured(ctx, 1, resources.UAV(data.octree, ctx),          resources.GetResource(data.octree),         sizeof(OctTreeNode), 0);
        uavHeap2.SetUAVStructured(ctx, 2, resources.UAV(data.freeList, ctx),        resources.GetResource(data.freeList),       sizeof(uint32_t), 0);

        ctx.SetComputeDescriptorTable(5, uavHeap2);

        const auto XY = uint2{ float2{  WH[0] / 128.0f, WH[1] / 128.0f }.ceil() };
        ctx.Dispatch(sampleInjection, { XY, 1 });

        // Create Dispatch args
        ctx.DiscardResource(resources.GetResource(data.indirectArgs));
        ctx.ClearUAVBuffer(resources.GetResource(data.indirectArgs));

        ctx.AddUAVBarrier(resources.GetResource(data.counters));
        ctx.AddUAVBarrier(resources.GetResource(data.indirectArgs));

        _GatherArgs(data.counters, data.indirectArgs, resources, ctx);

        ctx.SetPipelineState(gatherSubDRequests);
        ctx.SetComputeRootSignature(rootSig);
        ctx.SetComputeConstantBufferView(0, cameraConstants);
        ctx.SetComputeDescriptorTable(4, resourceHeap);
        ctx.SetComputeDescriptorTable(5, uavHeap2);

        ctx.ExecuteIndirect(
            resources.IndirectArgs(data.indirectArgs, ctx),
            dispatch);

        ctx.AddUAVBarrier(resources.GetResource(data.octree));

        _GatherArgs(data.octree, data.indirectArgs, resources, ctx);

        ctx.SetComputeRootSignature(rootSig);
        ctx.SetPipelineState(processSubDRequests);
        ctx.SetComputeConstantBufferView(0, cameraConstants);
        ctx.SetComputeDescriptorTable(4, resourceHeap);
        ctx.SetComputeDescriptorTable(5, uavHeap2);

        ctx.ExecuteIndirect(
            resources.IndirectArgs(data.indirectArgs, ctx),
            dispatch);

        ctx.EndEvent_DEBUG();
    }


    /************************************************************************************************/


    UpdateVoxelVolume& GILightingEngine::UpdateVoxelVolumes(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
			    ResourceHandle                  depthTarget,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator)
    {
        return frameGraph.AddNode<UpdateVoxelVolume>(
            UpdateVoxelVolume{
                reserveCB, camera
            },
			[&](FrameGraphNodeBuilder& builder, UpdateVoxelVolume& data)
			{
                data.depthTarget        = builder.NonPixelShaderResource(depthTarget);
                data.octree             = builder.UnorderedAccess(octreeBuffer[0]);
                data.counters           = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8192), DRS_UAV);
                data.indirectArgs       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);
                data.freeList           = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DRS_UAV);
                data.sampleBuffer       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DRS_UAV);
                data.scratchPad         = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DRS_UAV);

                builder.SetDebugName(data.counters,     "Counters");
                builder.SetDebugName(data.indirectArgs, "IndirectArgs");
                builder.SetDebugName(data.freeList,     "Freelist");
                builder.SetDebugName(data.sampleBuffer, "SampleBuffer");
            },
            [this](UpdateVoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ctx.BeginEvent_DEBUG("VXGI");

                CleanUpPhase(data, resources, ctx, allocator);
                CreateNodePhase(data, resources, ctx, allocator);
                
                ctx.EndEvent_DEBUG();
            });
    }


    /************************************************************************************************/


    DEBUGVIS_VoxelVolume& GILightingEngine::DrawVoxelVolume(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
                OITPass&                        target,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator)
    {
        return frameGraph.AddNode<DEBUGVIS_VoxelVolume>(
            DEBUGVIS_VoxelVolume{
                reserveCB, camera
            },
			[&](FrameGraphNodeBuilder& builder, DEBUGVIS_VoxelVolume& data)
			{
                data.accumlator     = target.accumalatorObject;
                data.counter        = target.counterObject;
                data.depthTarget    = builder.ReadTransition(target.depthTarget, DRS_DEPTHBUFFERREAD);

                data.indirectArgs   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);
                data.octree         = builder.NonPixelShaderResource(octreeBuffer[0]);
            },
			[&draw = draw](DEBUGVIS_VoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ctx.BeginEvent_DEBUG("VXGI_DrawVolume");

                auto state                  = resources.GetPipelineState(VXGI_DRAWVOLUMEVISUALIZATION);
                auto gatherDrawArgs         = resources.GetPipelineState(VXGI_GATHERDRAWARGS);

                const uint32_t blockSize    = 8;
                auto& rootSig               = resources.renderSystem().Library.RSDefault;

                ctx.SetComputeRootSignature(rootSig);
                ctx.SetPipelineState(gatherDrawArgs);

                DescriptorHeap uavHeap;
                uavHeap.Init2(ctx, rootSig.GetDescHeap(1), 2, &allocator);
                uavHeap.SetUAVStructured(ctx, 0, resources.UAV(data.octree, ctx), 4);
                uavHeap.SetUAVStructured(ctx, 1, resources.UAV(data.indirectArgs, ctx), 16, 0);


                ctx.SetComputeDescriptorTable(5, uavHeap);
                ctx.Dispatch(gatherDrawArgs, { 1, 1, 1 });

                ctx.SetRootSignature(rootSig);
                ctx.SetPipelineState(state);

                CBPushBuffer constantBuffer{ data.reserveCB(AlignedSize<Camera::ConstantBuffer>()) };

                const auto cameraConstantValues = GetCameraConstants(data.camera);
                const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };

                DescriptorHeap resourceHeap;
                resourceHeap.Init2(ctx, rootSig.GetDescHeap(0), 2, &allocator);
                resourceHeap.SetStructuredResource(ctx, 0, resources.NonPixelShaderResource(data.octree, ctx), sizeof(OctTreeNode), 4096 / sizeof(OctTreeNode));

                ctx.SetGraphicsConstantBufferView(0, cameraConstants);
                ctx.SetPrimitiveTopology(EInputTopology::EIT_POINT);

                ctx.SetScissorAndViewports({
                        resources.GetRenderTarget(data.accumlator),
                        resources.GetRenderTarget(data.counter),
                    });

                ctx.SetRenderTargets(
                    {
                        resources.GetRenderTarget(data.accumlator),
                        resources.GetRenderTarget(data.counter),
                    },
                    true,
                    resources.GetResource(data.depthTarget));

                ctx.SetGraphicsDescriptorTable(4, resourceHeap);
                ctx.ExecuteIndirect(resources.IndirectArgs(data.indirectArgs, ctx), draw);

                ctx.EndEvent_DEBUG();
            });
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateMarkErasePSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("MarkEraseNodes", "cs_6_0", R"(assets\shaders\VXGI_Erase.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }

    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateRemovePSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("ReleaseNodes", "cs_6_0", R"(assets\shaders\VXGI_Remove.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            removeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateInjectVoxelSamplesPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("Injection", "cs_6_0", R"(assets\shaders\VXGI.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGIGatherDispatchArgsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateIndirectArgs", "cs_6_0", R"(assets\shaders\VXGI_DispatchArgs.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            gatherSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGIEraseDispatchArgsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateRemoveArgs", "cs_6_0", R"(assets\shaders\VXGI_RemoveArgs.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            gatherSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGIDecrementDispatchArgsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateIndirectArgs", "cs_6_0", R"(assets\shaders\VXGI_DecrementCounter.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            removeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGIGatherDrawArgsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateDrawArgs", "cs_6_0", R"(assets\shaders\VXGI_DrawArgs.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGIGatherSubDRequestsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("GatherSubdivionRequests", "cs_6_0", R"(assets\shaders\VXGI.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGIProcessSubDRequestsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("ProcessSubdivionRquests", "cs_6_0", R"(assets\shaders\VXGI.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateVXGI_InitOctree(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("Init", "cs_6_0", R"(assets\shaders\VXGI_InitOctree.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* GILightingEngine::CreateUpdateVolumeVisualizationPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VolumePoint_VS", "vs_6_0", "assets\\shaders\\VoxelDebugVis.hlsl");
	    auto GShader = RS->LoadShader("VoxelDebug_GS",  "gs_6_0", "assets\\shaders\\VoxelDebugVis.hlsl");
	    auto PShader = RS->LoadShader("VoxelDebug_PS",  "ps_6_0", "assets\\shaders\\VoxelDebugVis.hlsl");

	    D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	    D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	    Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable                  = true;
        Depth_Desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;

	    D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		    PSO_Desc.pRootSignature        = RS->Library.RSDefault;
		    PSO_Desc.VS                    = VShader;
		    PSO_Desc.GS                    = GShader;
		    PSO_Desc.PS                    = PShader;
		    PSO_Desc.RasterizerState       = Rast_Desc;
		    PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		    PSO_Desc.SampleMask            = UINT_MAX;
		    PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		    PSO_Desc.NumRenderTargets      = 2;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // count
		    PSO_Desc.SampleDesc.Count      = 1;
		    PSO_Desc.SampleDesc.Quality    = 0;
		    PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
		    PSO_Desc.InputLayout           = { nullptr, 0 };
		    PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.IndependentBlendEnable          = true;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlend        = D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend       = D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[1].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[1].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[1].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[1].SrcBlend        = D3D12_BLEND_ZERO;
            PSO_Desc.BlendState.RenderTarget[1].SrcBlendAlpha   = D3D12_BLEND_ZERO;

            PSO_Desc.BlendState.RenderTarget[1].DestBlend       = D3D12_BLEND_INV_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[1].DestBlendAlpha  = D3D12_BLEND_INV_SRC_ALPHA;
	    }

	    ID3D12PipelineState* PSO = nullptr;
	    auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
	    FK_ASSERT(SUCCEEDED(HR));

	    return PSO;
    }


}   /************************************************************************************************/
