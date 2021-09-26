#include "SVOGI.h"

namespace FlexKit
{   /************************************************************************************************/


    GILightingEngine::GILightingEngine(RenderSystem& IN_renderSystem, iAllocator& allocator) :
            renderSystem    { IN_renderSystem },

            octreeBuffer    { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(512 * MEGABYTE)) },

            gatherSignature     { &allocator },
            dispatchSignature   { &allocator },
            removeSignature     { &allocator },

            staticVoxelizer     { renderSystem, allocator }
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

        renderSystem.SetDebugName(octreeBuffer, "OctreeBuffer");

        renderSystem.RegisterPSOLoader(VXGI_DRAWVOLUMEVISUALIZATION,    { &renderSystem.Library.RSDefault, CreateUpdateVolumeVisualizationPSO});

        renderSystem.RegisterPSOLoader(VXGI_SAMPLEINJECTION,            { &renderSystem.Library.RSDefault, CreateInjectVoxelSamplesPSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERDISPATCHARGS,         { &renderSystem.Library.RSDefault, [this](RenderSystem* rs) { return CreateVXGIGatherDispatchArgsPSO(rs); } });
        renderSystem.RegisterPSOLoader(VXGI_GATHERDRAWARGS,             { &renderSystem.Library.RSDefault, CreateVXGIGatherDrawArgsPSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERSUBDIVISIONREQUESTS,  { &renderSystem.Library.RSDefault, CreateVXGIGatherSubDRequestsPSO });
        renderSystem.RegisterPSOLoader(VXGI_PROCESSSUBDREQUESTS,        { &renderSystem.Library.RSDefault, CreateVXGIProcessSubDRequestsPSO });
        renderSystem.RegisterPSOLoader(VXGI_INITOCTREE,                 { &renderSystem.Library.RSDefault, CreateVXGI_InitOctree });
        renderSystem.RegisterPSOLoader(VXGI_ALLOCATENODES,              { &removeSignature, [this](RenderSystem* rs){ return CreateAllocatePSO(rs); } });

        gatherDispatchArgs  = renderSystem.GetPSO(VXGI_GATHERDISPATCHARGS);
    }


    /************************************************************************************************/


    GILightingEngine::~GILightingEngine()
    {
        renderSystem.ReleaseResource(octreeBuffer);
    }


    /************************************************************************************************/


    void GILightingEngine::InitializeOctree(FrameGraph& frameGraph)
    {
        struct InitOctree
        {
            FrameResourceHandle octree;
            FrameResourceHandle freeList;
        };

        frameGraph.AddNode<InitOctree>(
            InitOctree{},
            [&](FrameGraphNodeBuilder& builder, InitOctree& data)
            {
                data.octree = builder.UnorderedAccess(octreeBuffer);
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


                DescriptorHeap heap;
                heap.Init2(ctx, rootSig.GetDescHeap(1), 1, &allocator);
                heap.SetUAVStructured(ctx, 0, resources.UAV(data.octree, ctx), resources.UAV(data.octree, ctx), sizeof(OctTreeNode), 0);

                ctx.SetComputeRootSignature(rootSig);
                ctx.SetComputeDescriptorTable(5, heap);
                ctx.Dispatch(Init, { 1, 1, 1 });

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
        /*
        auto markNodes          = resources.GetPipelineState(VXGI_MARKERASE);
        auto allocateNodes      = resources.GetPipelineState(VXGI_ALLOCATENODES);
        auto transferNodes      = resources.GetPipelineState(VXGI_TRANSFERNODES);
        auto decrement          = resources.GetPipelineState(VXGI_DECREMENTCOUNTER);
        auto gatherDispatchArgs = resources.GetPipelineState(VXGI_GATHERDISPATCHARGS);
        auto& rootSig           = resources.renderSystem().Library.RSDefault;

        const auto cameraConstantValues = GetCameraConstants(data.camera);
        CBPushBuffer constantBuffer{ data.reserveCB(AlignedSize<Camera::ConstantBuffer>()) };
        const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };

        DescriptorHeap srvHeap;
        srvHeap.Init2(ctx, rootSig.GetDescHeap(0), 1, &allocator);
        srvHeap.SetSRV(ctx, 0, resources.NonPixelShaderResource(data.depthTarget, ctx), DeviceFormat::R32_FLOAT);

        ctx.BeginEvent_DEBUG("SampleCleanup");

        {
            ctx.BeginEvent_DEBUG("Clear temporaries");
            ctx.SetComputeRootSignature(rootSig);

            // Create Dispatch args
            ctx.DiscardResource(resources.GetResource(data.indirectArgs));
            ctx.ClearUAVBuffer(resources.GetResource(data.indirectArgs));
            ctx.ClearUAVBufferRange(resources.GetResource(data.Fresh_octree), 0, 16, { 1, 0, 0, 0 });
            ctx.AddUAVBarrier(resources.GetResource(data.indirectArgs));
            ctx.AddUAVBarrier(resources.GetResource(data.Fresh_octree));

            ctx.EndEvent_DEBUG();
        }
        {   // Mark Erase Flags
            ctx.BeginEvent_DEBUG("MarkNodes");
            _GatherArgs2(data.Stale_Octree, data.Stale_Octree, data.indirectArgs, resources, ctx);

            ctx.SetComputeRootSignature(rootSig);

            DescriptorHeap uavHeap;
            uavHeap.Init2(ctx, rootSig.GetDescHeap(1), 1, &allocator);
            uavHeap.SetUAVStructured(ctx, 0, resources.UAV(data.Stale_Octree, ctx), resources.GetResource(data.Stale_Octree), sizeof(OctTreeNode), 0);

            ctx.SetComputeConstantBufferView(0, cameraConstants);
            ctx.SetComputeDescriptorTable(4, srvHeap);
            ctx.SetComputeDescriptorTable(5, uavHeap);

            ctx.SetPipelineState(markNodes);

            for(uint I = 0; I < 8; I++)
            {
                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.indirectArgs, ctx),
                    dispatch,
                    16);

                ctx.AddUAVBarrier(resources.GetResource(data.Stale_Octree));
            }

            ctx.EndEvent_DEBUG();
        }
        {   // Move Unmarked nodes to fresh octree
            DescriptorHeap uavHeap;
            uavHeap.Init2(ctx, removeSignature.GetDescHeap(0), 1, &allocator);
            uavHeap.SetUAVStructured(ctx, 0, resources.UAV(data.Fresh_octree, ctx), resources.GetResource(data.Fresh_octree), sizeof(OctTreeNode), 0);

            ctx.SetComputeRootSignature(removeSignature);

            ctx.SetComputeUnorderedAccessView(0, resources.UAV(data.Stale_Octree, ctx), 4096);
            ctx.SetComputeDescriptorTable(2, uavHeap);

            ctx.SetPipelineState(allocateNodes);

            ctx.ExecuteIndirect(
                resources.IndirectArgs(data.indirectArgs, ctx),
                remove, 0);

            ctx.AddUAVBarrier(resources.GetResource(data.Stale_Octree));

            ctx.SetPipelineState(transferNodes);

            ctx.ExecuteIndirect(
                resources.IndirectArgs(data.indirectArgs, ctx),
                remove, 0);

            ctx.AddUAVBarrier(resources.GetResource(data.Fresh_octree));
        }

        ctx.EndEvent_DEBUG();
        */
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
        uavHeap2.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
        uavHeap2.SetUAVStructured(ctx, 0, resources.UAV(data.sampleBuffer, ctx), resources.UAV(data.counters, ctx),   32, 0);
        uavHeap2.SetUAVStructured(ctx, 1, resources.UAV(data.octree, ctx), resources.GetResource(data.octree), sizeof(OctTreeNode), 0);
        uavHeap2.SetUAVStructured(ctx, 2, resources.UAV(data.octree, ctx), 4, 0);

        ctx.SetComputeDescriptorTable(5, uavHeap2);

        const auto XY = uint2{ float2{  WH[0] / 128.0f, WH[1] / 128.0f }.ceil() };
        ctx.Dispatch(sampleInjection, { XY, 1 });

        // Create Dispatch args
        ctx.DiscardResource(resources.GetResource(data.indirectArgs));
        ctx.ClearUAVBuffer(resources.GetResource(data.indirectArgs));

        ctx.AddUAVBarrier(resources.GetResource(data.indirectArgs));
        ctx.AddUAVBarrier(resources.GetResource(data.counters));

        for(uint32_t I = 0; I < 1; I++)
        {
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

            ctx.AddUAVBarrier(resources.GetResource(data.octree));
        }

        ctx.EndEvent_DEBUG();
    }


    /************************************************************************************************/


    StaticVoxelizer::VoxelizePass& GILightingEngine::VoxelizeScene(
        FrameGraph&                     frameGraph,
        Scene&                          scene,
        ReserveConstantBufferFunction   reserveCB,
        GatherPassesTask&               passes)
    {
        return staticVoxelizer.VoxelizeScene(frameGraph, scene, { 2048, 2048, 2048 }, passes, reserveCB);
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
                data.octree             = builder.UnorderedAccess(octreeBuffer);

                data.depthTarget        = builder.NonPixelShaderResource(depthTarget);
                data.counters           = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8192), DRS_UAV);
                data.indirectArgs       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);
                data.sampleBuffer       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DRS_UAV);

                builder.SetDebugName(data.counters,     "Counters");
                builder.SetDebugName(data.indirectArgs, "IndirectArgs");
                builder.SetDebugName(data.sampleBuffer, "SampleBuffer");
            },
            [this](UpdateVoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ctx.BeginEvent_DEBUG("VXGI");

                CreateNodePhase(data, resources, ctx, allocator);
                
                ctx.EndEvent_DEBUG();
            });
    }


    /************************************************************************************************/


    DEBUGVIS_VoxelVolume& GILightingEngine::DrawVoxelVolume(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
                ResourceHandle                  depthTarget, 
                FrameResourceHandle             renderTarget, 
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator)
    {
        return frameGraph.AddNode<DEBUGVIS_VoxelVolume>(
            DEBUGVIS_VoxelVolume{
                reserveCB, camera
            },
			[&](FrameGraphNodeBuilder& builder, DEBUGVIS_VoxelVolume& data)
			{
                data.depthTarget    = builder.DepthRead(depthTarget);
                data.renderTarget   = builder.WriteTransition(renderTarget, DRS_RenderTarget);

                data.indirectArgs   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);
                data.octree         = builder.NonPixelShaderResource(octreeBuffer);
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
                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

                ctx.SetScissorAndViewports({
                        resources.GetRenderTarget(data.renderTarget),
                    });

                ctx.SetRenderTargets(
                    {
                        resources.GetRenderTarget(data.renderTarget),
                    },
                    true,
                    resources.GetResource(data.depthTarget));

                ctx.SetGraphicsDescriptorTable(4, resourceHeap);
                ctx.Draw(6);

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


    ID3D12PipelineState* GILightingEngine::CreateAllocatePSO(RenderSystem* RS)
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


    ID3D12PipelineState* GILightingEngine::CreateTransferPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("TransferNodes", "cs_6_0", R"(assets\shaders\VXGI_Remove.hlsl)");

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
        auto VShader = RS->LoadShader("FullScreenQuad_VS",  "vs_6_0", "assets\\shaders\\VoxelDebugVis.hlsl");
	    auto PShader = RS->LoadShader("VoxelDebug_PS",      "ps_6_0", "assets\\shaders\\VoxelDebugVis.hlsl");

	    D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	    D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	    Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable                  = true;
        Depth_Desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;

	    D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		    PSO_Desc.pRootSignature        = RS->Library.RSDefault;
		    PSO_Desc.VS                    = VShader;
		    PSO_Desc.PS                    = PShader;
		    PSO_Desc.RasterizerState       = Rast_Desc;
		    PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		    PSO_Desc.SampleMask            = UINT_MAX;
		    PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		    PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
		    PSO_Desc.SampleDesc.Count      = 1;
		    PSO_Desc.SampleDesc.Quality    = 0;
		    PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
		    PSO_Desc.InputLayout           = { nullptr, 0 };
		    PSO_Desc.DepthStencilState     = Depth_Desc;
	    }

	    ID3D12PipelineState* PSO = nullptr;
	    auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
	    FK_ASSERT(SUCCEEDED(HR));

	    return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* StaticVoxelizer::CreateVoxelizerPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("voxelize_VS", "vs_6_0", "assets\\shaders\\Voxelizer.hlsl");
	    auto GShader = RS->LoadShader("voxelize_GS", "gs_6_0", "assets\\shaders\\Voxelizer.hlsl");
	    auto PShader = RS->LoadShader("voxelize_PS", "ps_6_6", "assets\\shaders\\Voxelizer.hlsl");

	    D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.ConservativeRaster            = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

	    D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthEnable = false;

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

	    D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
		    PSO_Desc.pRootSignature        = voxelizeSignature;
		    PSO_Desc.VS                    = VShader;
		    PSO_Desc.GS                    = GShader;
		    PSO_Desc.PS                    = PShader;
		    PSO_Desc.RasterizerState       = Rast_Desc;
		    PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		    PSO_Desc.SampleMask            = UINT_MAX;
		    PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		    PSO_Desc.NumRenderTargets      = 0;
		    PSO_Desc.SampleDesc.Count      = 1;
		    PSO_Desc.SampleDesc.Quality    = 0;
		    PSO_Desc.InputLayout           = { InputElements, 3 };
		    PSO_Desc.DepthStencilState     = Depth_Desc;
        };

	    ID3D12PipelineState* PSO = nullptr;
	    auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
	    FK_ASSERT(SUCCEEDED(HR));

	    return PSO;
    }


    ID3D12PipelineState* StaticVoxelizer::CreateSortPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("SortBlock", "cs_6_6", R"(assets\shaders\SVO_VoxelSortBlock.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            sortingSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/

    ID3D12PipelineState* StaticVoxelizer::CreateMultiBlockSortPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("SortBlock", "cs_6_6", R"(assets\shaders\SVO_VoxelSortMultipleBlock.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            sortingSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* StaticVoxelizer::CreateGatherArgsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("Main", "cs_6_6", R"(assets\shaders\SVO_VoxelGatherArgs.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            sortingSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    StaticVoxelizer::StaticVoxelizer(RenderSystem& renderSystem, iAllocator& allocator) :
        voxelizeSignature{ &allocator },
        sortingSignature{ &allocator }
    {
        voxelizeSignature.AllowIA = true;
        voxelizeSignature.AllowSO = false;

        DesciptorHeapLayout layout{};
        layout.SetParameterAsShaderUAV(0, 0, 1, 0);

        voxelizeSignature.SetParameterAsUINT(0, 12, 0, 0, PIPELINE_DEST_ALL);
        voxelizeSignature.SetParameterAsCBV(1, 1, 0, PIPELINE_DEST_ALL);
        voxelizeSignature.SetParameterAsDescriptorTable(2, layout, -1, PIPELINE_DEST_PS);
        voxelizeSignature.Build(renderSystem, &allocator);

        sortingSignature.AllowIA = false;
        sortingSignature.AllowSO = false;

        sortingSignature.SetParameterAsUAV(0, 0, 0, PIPELINE_DEST_CS);
        sortingSignature.SetParameterAsUAV(1, 1, 0, PIPELINE_DEST_CS);
        sortingSignature.SetParameterAsUINT(2, 4, 0, 0, PIPELINE_DEST_CS);
        sortingSignature.SetParameterAsUINT(2, 4, 1, 0, PIPELINE_DEST_CS);
        sortingSignature.Build(renderSystem, &allocator);

        renderSystem.RegisterPSOLoader(
            SVO_Voxelize,
            {
                &voxelizeSignature,
                [&](RenderSystem* RS)
                {
                    return CreateVoxelizerPSO(RS);
                }
            });

        renderSystem.RegisterPSOLoader(
            SVO_SortSingleBlock,
            {
                &sortingSignature,
                [&](RenderSystem* RS)
                {
                    return CreateSortPSO(RS);
                }
            });

        renderSystem.RegisterPSOLoader(
            SVO_GatherArguments,
            {
                &sortingSignature,
                [&](RenderSystem* RS)
                {
                    return CreateGatherArgsPSO(RS);
                }
            });

        /*
        renderSystem.QueuePSOLoad(SVO_Voxelize);
        renderSystem.QueuePSOLoad(SVO_SortSingleBlock);
        renderSystem.QueuePSOLoad(SVO_SortMultiBlock);
        renderSystem.QueuePSOLoad(SVO_GatherArguments);
        */

        dispatch = renderSystem.CreateIndirectLayout(
            {
                {
                    ILE_RootDescriptorUINT,
                    IndirectDrawDescription::Constant{
                        .rootParameterIdx   = 2,
                        .destinationOffset  = 0,
                        .numValues          = 4 }},
                { ILE_DispatchCall }
            },
            &allocator,
            &sortingSignature
        );
    }


    /************************************************************************************************/


    void StaticVoxelizer::GatherArgs(FrameResourceHandle argBuffer, FrameResourceHandle sampleBuffer, ResourceHandler& resources, Context& ctx)
    {
        static const auto gatherArgs = resources.GetPipelineState(SVO_GatherArguments);
        ctx.SetComputeRootSignature(sortingSignature);
        ctx.SetPipelineState(gatherArgs);

        ctx.SetComputeUnorderedAccessView(0, resources.UAV(argBuffer, ctx));
        ctx.SetComputeUnorderedAccessView(1, resources.UAV(sampleBuffer, ctx));

        ctx.Dispatch( uint3{ 1, 1, 1 } );
    }


    /************************************************************************************************/


    StaticVoxelizer::VoxelizePass& StaticVoxelizer::VoxelizeScene(FrameGraph& frameGraph, Scene& scene, uint3 XYZ, GatherPassesTask& passes, ReserveConstantBufferFunction reserveCB)
    {
        return frameGraph.AddNode<VoxelizePass>(
            VoxelizePass{
                passes,
                reserveCB
            },
            [](auto& builder, VoxelizePass& data)
            {
                data.sampleBuffer   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DRS_UAV);
                data.sortedBuffer   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DRS_UAV);
                data.argBuffer      = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4096), DRS_UAV);
            },
            [&scene, this](VoxelizePass& data, ResourceHandler& resources, Context& ctx, iAllocator& TL_allocator)
            {
                return;
                auto voxelize   = resources.GetPipelineState(SVO_Voxelize);
                auto sort       = resources.GetPipelineState(SVO_SortSingleBlock);
                auto multiSort  = resources.GetPipelineState(SVO_SortMultiBlock);

                auto& visabilityComponent = SceneVisibilityComponent::GetComponent();
                auto& brushComponent = BrushComponent::GetComponent();

                Vector<BrushHandle> brushes{ &TL_allocator };
                for (auto& entityHandle : scene.sceneEntities)
                {
                    Apply(*visabilityComponent[entityHandle].entity,
                        [&](MaterialComponentView& material,
                            BrushView& brush)
                        {
                            auto passes = material.GetPasses();

                            if (std::find(passes.begin(), passes.end(), GetCRCGUID(VXGI_STATIC)))
                                brushes.push_back(brush.brush);
                        });
                }

                CBPushBuffer constantBuffer{ data.reserveCB(AlignedSize<float4x4>() * brushes.size()) };

                ctx.BeginEvent_DEBUG("Static Voxelizer");

                ctx.ClearUAVBufferRange(resources.UAV(data.sampleBuffer, ctx), 0, 1024);
                ctx.AddUAVBarrier(resources.GetResource(data.sampleBuffer));

                ctx.BeginEvent_DEBUG("Voxel Pass");

                ctx.SetRootSignature(voxelizeSignature);
                ctx.SetPipelineState(voxelize);

                float4 values[] = {
                    { 2048, 2048, 2048, 0 },
                    { 2048, 2048, 2048, 0 },
                    { -1024, -1024, -1024, 0 },
                };

                ctx.SetViewports({ D3D12_VIEWPORT{ 0, 0, 2048, 2048, 0, 1.0f } });
                ctx.SetScissorRects({ D3D12_RECT{ 0, 0, 2048, 2048 } });
                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
                ctx.SetGraphicsConstantValue(0, 12, values);
                
                DescriptorHeap heap;
                heap.Init(ctx, voxelizeSignature.GetDescHeap(0), &TL_allocator);
                heap.SetUAVStructured(ctx, 0, resources.UAV(data.sampleBuffer, ctx), resources.UAV(data.sampleBuffer, ctx), 40, 0);
                ctx.SetGraphicsDescriptorTable(2, heap);


                for (const auto& brush : brushes)
                {
                    auto* const triMesh = GetMeshResource(brushComponent[brush].MeshHandle);
                    auto lod            = triMesh->GetHighestLoadedLodIdx();

                    const float4x4 WT = GetWT(brushComponent[brush].Node);
                    const ConstantBufferDataSet entityConstants{ WT, constantBuffer };

                    ctx.SetGraphicsConstantBufferView(1, entityConstants);

                    ctx.AddIndexBuffer(triMesh, lod);
                    ctx.AddVertexBuffers(triMesh,
                        lod,
                        {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV });

                    ctx.DrawIndexed(triMesh->lods[lod].GetIndexCount());
                }

                ctx.EndEvent_DEBUG();

                ctx.BeginEvent_DEBUG("Sort");

                ctx.AddUAVBarrier(resources.GetResource(data.argBuffer));

                GatherArgs(data.argBuffer, data.sampleBuffer, resources, ctx);

                ctx.SetComputeRootSignature(sortingSignature);

                ctx.SetComputeUnorderedAccessView(0, resources.GetResource(data.sampleBuffer), 4096);

                ctx.SetPipelineState(sort);
                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.argBuffer, ctx),
                    dispatch);

                ctx.AddUAVBarrier(resources.GetResource(data.argBuffer));

                ctx.SetPipelineState(multiSort);
                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.argBuffer, ctx),
                    dispatch);

                ctx.AddUAVBarrier(resources.GetResource(data.argBuffer));

                ctx.EndEvent_DEBUG();
                ctx.EndEvent_DEBUG();
            });
    }


}   /************************************************************************************************/
