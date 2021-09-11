#include "SVOGI.h"

namespace FlexKit
{
    GILightingEngine::GILightingEngine(RenderSystem& IN_renderSystem, iAllocator& allocator) :
            renderSystem    { IN_renderSystem },

            voxelBuffer     { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(64 * MEGABYTE))   },
            octreeBuffer    { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(512 * MEGABYTE))  },
            freeList        { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(32 * MEGABYTE))   },

            gatherSignature     { &allocator },
            dispatchSignature   { &allocator },
            testSignature       { &allocator }
    {
        testSignature.SetParameterAsUAV(0, 0, 0, PIPELINE_DEST_CS);
        testSignature.SetParameterAsUAV(1, 1, 0, PIPELINE_DEST_CS);
        testSignature.SetParameterAsUAV(2, 2, 0, PIPELINE_DEST_CS);
        testSignature.SetParameterAsUINT(3, 4, 0, 0, PIPELINE_DEST_CS);
        testSignature.Build(&renderSystem, &allocator);

        gather = renderSystem.CreateIndirectLayout({
            { ILE_RootDescriptorUINT, IndirectDrawDescription::Constant{.rootParameterIdx = 3, .destinationOffset = 0, .numValues = 4 } },
            { ILE_DispatchCall } },
            &allocator,
            &testSignature);

        renderSystem.SetDebugName(voxelBuffer, "VoxelBuffer");
        renderSystem.SetDebugName(octreeBuffer, "Octree");

        renderSystem.RegisterPSOLoader(VXGI_CLEANUPVOXELVOLUMES,       { &renderSystem.Library.RSDefault, CreateUpdateVoxelVolumesPSO });
        renderSystem.RegisterPSOLoader(VXGI_DRAWVOLUMEVISUALIZATION,   { &renderSystem.Library.RSDefault, CreateUpdateVolumeVisualizationPSO});
        renderSystem.RegisterPSOLoader(VXGI_SAMPLEINJECTION,           { &renderSystem.Library.RSDefault, CreateInjectVoxelSamplesPSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERARGS1,               { &renderSystem.Library.RSDefault, CreateVXGIGatherArgs1PSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERARGS2,               { &renderSystem.Library.RSDefault, CreateVXGIGatherArgs2PSO });
        renderSystem.RegisterPSOLoader(VXGI_GATHERSUBDIVISIONREQUESTS, { &renderSystem.Library.RSDefault, CreateVXGIGatherSubDRequestsPSO });
        renderSystem.RegisterPSOLoader(VXGI_PROCESSSUBDREQUESTS,       { &renderSystem.Library.RSDefault, CreateVXGIProcessSubDRequestsPSO });
        renderSystem.RegisterPSOLoader(VXGI_INITOCTREE,                { &renderSystem.Library.RSDefault, CreateVXGI_InitOctree });
        renderSystem.RegisterPSOLoader(VXGI_FREENODES,                 { &renderSystem.Library.RSDefault, CreateReleaseVoxelNodesPSO });
    }


    /************************************************************************************************/


    GILightingEngine::~GILightingEngine()
    {

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
                data.octree     = builder.UnorderedAccess(octreeBuffer);
                data.freeList   = builder.UnorderedAccess(freeList);
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

                //ctx.ClearUAVBuffer(resources.GetResource(data.freeList));
                //ctx.ClearUAVBuffer(resources.GetResource(data.octree));

                ctx.AddUAVBarrier(resources.GetResource(data.freeList));
                ctx.AddUAVBarrier(resources.GetResource(data.octree));

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


    UpdateVoxelVolume& GILightingEngine::UpdateVoxelVolumes(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
			    ResourceHandle                  depthTarget,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator)
    {
        struct alignas(64) OctTreeNode
        {
            uint    nodes[8];
            uint4   volumeCord;
            uint    data;
            uint    flags;
        };

        return frameGraph.AddNode<UpdateVoxelVolume>(
            UpdateVoxelVolume{
                reserveCB, camera
            },
			[&](FrameGraphNodeBuilder& builder, UpdateVoxelVolume& data)
			{
                data.depthTarget        = builder.NonPixelShaderResource(depthTarget);
                data.voxelBuffer        = builder.UnorderedAccess(voxelBuffer);
                data.octree             = builder.UnorderedAccess(octreeBuffer);
                data.counters           = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8192), DRS_UAV);
                data.indirectArgs       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);
                data.freeList           = builder.UnorderedAccess(freeList);

                builder.SetDebugName(data.counters,     "counters");
                builder.SetDebugName(data.indirectArgs, "IndirectArgs");
            },
            [&dispatch = dispatch](UpdateVoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ctx.BeginEvent_DEBUG("VXGI");

                const auto WH = resources.GetTextureWH(data.depthTarget);

                auto updateVolumes          = resources.GetPipelineState(VXGI_CLEANUPVOXELVOLUMES);
                auto freeNodes              = resources.GetPipelineState(VXGI_FREENODES);
                auto sampleInjection        = resources.GetPipelineState(VXGI_SAMPLEINJECTION);
                auto gatherDispatchArgs     = resources.GetPipelineState(VXGI_GATHERARGS1);
                auto gatherSubDRequests     = resources.GetPipelineState(VXGI_GATHERSUBDIVISIONREQUESTS);
                auto processSubDRequests    = resources.GetPipelineState(VXGI_PROCESSSUBDREQUESTS);

                const uint32_t blockSize = 8;
                auto& rootSig = resources.renderSystem().Library.RSDefault;

                CBPushBuffer constantBuffer{ data.reserveCB(AlignedSize<Camera::ConstantBuffer>()) };

                const auto cameraConstantValues = GetCameraConstants(data.camera);
                const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };

                ctx.DiscardResource(resources.GetResource(data.counters));
                ctx.ClearUAVBuffer(resources.GetResource(data.counters), uint4{ 0, 0, 312, 213 });
                

                ctx.SetComputeRootSignature(rootSig);
                ctx.SetComputeConstantBufferView(0, cameraConstants);

                DescriptorHeap resourceHeap;
                resourceHeap.Init2(ctx, rootSig.GetDescHeap(0), 1, &allocator);
                resourceHeap.SetSRV(ctx, 0, resources.GetResource(data.depthTarget), DeviceFormat::R32_FLOAT);

                ctx.SetComputeDescriptorTable(4, resourceHeap);

                ctx.BeginEvent_DEBUG("SampleCleanup");


                // Gather arguements 
                DescriptorHeap uavHeap0;
                uavHeap0.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
                uavHeap0.SetUAVStructured(ctx, 0, resources.UAV(data.octree, ctx), 4);
                uavHeap0.SetUAVStructured(ctx, 1, resources.UAV(data.indirectArgs, ctx), 16, 0);

                ctx.SetComputeDescriptorTable(5, uavHeap0);
                if(0)
                ctx.Dispatch(gatherDispatchArgs, { 1, 1, 1 });

                DescriptorHeap uavHeap1;
                uavHeap1.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
                uavHeap1.SetUAVStructured(ctx, 0, resources.UAV(data.voxelBuffer, ctx), resources.GetResource(data.counters), 32, 0);
                uavHeap1.SetUAVStructured(ctx, 1, resources.UAV(data.octree, ctx),      resources.GetResource(data.octree),     sizeof(OctTreeNode), 0);
                uavHeap1.SetUAVStructured(ctx, 2, resources.UAV(data.freeList, ctx),    resources.GetResource(data.freeList),   sizeof(uint32_t), 0);

                ctx.SetComputeDescriptorTable(5, uavHeap1);
                ctx.SetPipelineState(updateVolumes);

                if(0)
                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.indirectArgs, ctx),
                    dispatch);

                ctx.AddUAVBarrier(resources.GetResource(data.octree));

                // Gather arguements 
                {
                    DescriptorHeap uavHeap;
                    uavHeap.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
                    uavHeap.SetUAVStructured(ctx, 0, resources.UAV(data.freeList,      ctx), 4);
                    uavHeap.SetUAVStructured(ctx, 1, resources.UAV(data.indirectArgs,  ctx), 16, 0);

                    //ctx.Dispatch(gatherDispatchArgs, { 1, 1, 1 });
                }

                ctx.SetComputeDescriptorTable(5, uavHeap1);
                ctx.SetPipelineState(freeNodes);

                if(0)
                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.indirectArgs, ctx),
                    dispatch);

                ctx.AddUAVBarrier(resources.GetResource(data.octree));

                ctx.EndEvent_DEBUG();

                ctx.BeginEvent_DEBUG("SampleInjection");

                DescriptorHeap uavHeap2;
                uavHeap2.Init2(ctx, rootSig.GetDescHeap(1), 4, &allocator);
                uavHeap2.SetUAVStructured(ctx, 0, resources.UAV(data.voxelBuffer, ctx),     resources.GetResource(data.counters),   32, 0);
                uavHeap2.SetUAVStructured(ctx, 1, resources.UAV(data.octree, ctx),          resources.GetResource(data.octree),     sizeof(OctTreeNode), 0);
                uavHeap2.SetUAVStructured(ctx, 2, resources.UAV(data.freeList, ctx),        resources.GetResource(data.freeList),   sizeof(uint32_t), 0);

                ctx.SetComputeDescriptorTable(5, uavHeap2);
                ctx.AddUAVBarrier(resources.GetResource(data.counters));

                const auto XY = uint2{ float2{  WH[0] / 128.0f, WH[1] / 128.0f }.ceil() };
                ctx.Dispatch(sampleInjection, { XY, 1 });

                // Create Dispatch args
                ctx.DiscardResource(resources.GetResource(data.indirectArgs));
                ctx.ClearUAVBuffer(resources.GetResource(data.indirectArgs));

                ctx.AddUAVBarrier(resources.GetResource(data.counters));
                ctx.AddUAVBarrier(resources.GetResource(data.indirectArgs));

                DescriptorHeap uavHeap3;
                uavHeap3.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
                uavHeap3.SetUAVStructured(ctx, 0, resources.GetResource(data.counters), 4, 0);
                uavHeap3.SetUAVStructured(ctx, 1, resources.GetResource(data.indirectArgs), 16, 0);

                ctx.SetComputeDescriptorTable(5, uavHeap3);
                ctx.Dispatch(gatherDispatchArgs, { 1, 1, 1 });

                ctx.SetPipelineState(gatherSubDRequests);
                ctx.SetComputeConstantBufferView(0, cameraConstants);
                ctx.SetComputeDescriptorTable(4, resourceHeap);
                ctx.SetComputeDescriptorTable(5, uavHeap2);

                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.indirectArgs, ctx),
                    dispatch);

                ctx.AddUAVBarrier(resources.GetResource(data.octree));

                DescriptorHeap uavHeap4;
                uavHeap4.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);
                uavHeap4.SetUAVStructured(ctx, 0, resources.UAV(data.octree, ctx), 4);
                uavHeap4.SetUAVStructured(ctx, 1, resources.UAV(data.indirectArgs, ctx), 16, 0);

                ctx.SetComputeDescriptorTable(5, uavHeap4);
                ctx.Dispatch(gatherDispatchArgs, { 1, 1, 1 });

                ctx.SetPipelineState(processSubDRequests);
                ctx.SetComputeConstantBufferView(0, cameraConstants);
                ctx.SetComputeDescriptorTable(4, resourceHeap);
                ctx.SetComputeDescriptorTable(5, uavHeap2);

                ctx.ExecuteIndirect(
                    resources.IndirectArgs(data.indirectArgs, ctx),
                    dispatch);

                ctx.EndEvent_DEBUG();
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
                data.depthTarget    = target.depthTarget;

                data.indirectArgs   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);
                data.octree         = builder.NonPixelShaderResource(octreeBuffer);
            },
			[&draw = draw](DEBUGVIS_VoxelVolume& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ctx.BeginEvent_DEBUG("VXGI_DrawVolume");

                auto state                  = resources.GetPipelineState(VXGI_DRAWVOLUMEVISUALIZATION);
                auto gatherDrawArgs         = resources.GetPipelineState(VXGI_GATHERARGS2);

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

                struct alignas(64) OctTreeNode
                {
                    uint    nodes[8];
                    uint4   volumeCord;
                    uint    data;
                    uint    flags;
                };


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


    ID3D12PipelineState* GILightingEngine::CreateUpdateVoxelVolumesPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("MarkEraseNodes", "cs_6_0", R"(assets\shaders\VXGI.hlsl)");

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


    ID3D12PipelineState* GILightingEngine::CreateReleaseVoxelNodesPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("ReleaseNodes", "cs_6_0", R"(assets\shaders\VXGI.hlsl)");

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


    ID3D12PipelineState* GILightingEngine::CreateVXGIGatherArgs1PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateIndirectArgs", "cs_6_0", R"(assets\shaders\VXGI_DispatchArgs.hlsl)");

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


    ID3D12PipelineState* GILightingEngine::CreateVXGIGatherArgs2PSO(RenderSystem* RS)
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
