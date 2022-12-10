#include "SVOGI.h"

namespace FlexKit
{   /************************************************************************************************/


	GILightingEngine::GILightingEngine(RenderSystem& IN_renderSystem, iAllocator& allocator) :
			renderSystem    { IN_renderSystem },

			octreeBuffer    { renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(768 * MEGABYTE)) },

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
		return staticVoxelizer.VoxelizeScene(frameGraph, scene, octreeBuffer, { 64, 64, 64 }, passes, reserveCB);
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
				data.counters           = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8192), DASUAV);
				data.indirectArgs       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DASUAV);
				data.sampleBuffer       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(8 * MEGABYTE), DASUAV);

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


	SVO_RayTrace& GILightingEngine::RayTrace(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				ResourceHandle                  depthTarget, 
				FrameResourceHandle             renderTarget,
				GBuffer&                        gbuffer,
				ReserveConstantBufferFunction   reserveCB,
				iAllocator*                     allocator,
				uint32_t                        mipOffset)
	{
		return frameGraph.AddNode<SVO_RayTrace>(
			SVO_RayTrace{
				reserveCB, camera
			},
			[&](FrameGraphNodeBuilder& builder, SVO_RayTrace& data)
			{
				data.depthTarget    = builder.DepthRead(depthTarget);
				data.renderTarget   = builder.WriteTransition(renderTarget, DASRenderTarget);
				data.normals        = builder.PixelShaderResource(gbuffer.normal);
				data.albedo         = builder.PixelShaderResource(gbuffer.albedo);
				data.indirectArgs   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DASUAV);
				data.octree         = builder.NonPixelShaderResource(octreeBuffer);
			},
			[mipOffset](SVO_RayTrace& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ctx.BeginEvent_DEBUG("VXGI_DrawVolume");

				auto debugVis               = resources.GetPipelineState(VXGI_DRAWVOLUMEVISUALIZATION);
				auto& rootSig               = resources.renderSystem().Library.RSDefault;

				struct debugConstants
				{
					uint32_t mipOffset;
				} constantValues{
					.mipOffset = mipOffset,
				};

				CBPushBuffer constantBuffer = 
					data.reserveCB(
						AlignedSize<Camera::ConstantBuffer>() +
						AlignedSize<debugConstants>());

				const auto cameraConstantValues = GetCameraConstants(data.camera);
				const ConstantBufferDataSet cameraConstants { cameraConstantValues, constantBuffer };
				const ConstantBufferDataSet passConstants   { constantValues, constantBuffer };

				DescriptorHeap resourceHeap;
				resourceHeap.Init2(ctx, rootSig.GetDescHeap(0), 4, &allocator);
				resourceHeap.SetStructuredResource(ctx, 0, resources.PixelShaderResource(data.octree, ctx), sizeof(OctTreeNode), 4096 / sizeof(OctTreeNode));
				resourceHeap.SetSRV(ctx, 1, resources.PixelShaderResource(data.depthTarget, ctx), DeviceFormat::R32_FLOAT);
				resourceHeap.SetSRV(ctx, 2, resources.PixelShaderResource(data.normals, ctx));
				resourceHeap.SetSRV(ctx, 3, resources.PixelShaderResource(data.albedo, ctx));


				ctx.SetRootSignature(rootSig);
				ctx.SetPipelineState(debugVis);

				ctx.SetGraphicsConstantBufferView(0, cameraConstants);
				ctx.SetGraphicsConstantBufferView(1, passConstants);
				ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

				ctx.SetScissorAndViewports({
						resources.GetRenderTarget(data.renderTarget),
					});

				ctx.SetRenderTargets(
					{
						resources.GetRenderTarget(data.renderTarget),
					},
					false,
					resources.GetResource(data.depthTarget));

				ctx.SetGraphicsDescriptorTable(4, resourceHeap);
				ctx.Draw(6);

				ctx.EndEvent_DEBUG();
			});
	}


	/************************************************************************************************/


	ID3D12PipelineState* GILightingEngine::CreateMarkErasePSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("MarkEraseNodes", "cs_6_5", R"(assets\shaders\VXGI_Erase.hlsl)");

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
		Shader computeShader = RS->LoadShader("ReleaseNodes", "cs_6_5", R"(assets\shaders\VXGI_Remove.hlsl)");

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
		Shader computeShader = RS->LoadShader("TransferNodes", "cs_6_5", R"(assets\shaders\VXGI_Remove.hlsl)");

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
		Shader computeShader = RS->LoadShader("Injection", "cs_6_5", R"(assets\shaders\VXGI.hlsl)");

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
		Shader computeShader = RS->LoadShader("CreateIndirectArgs", "cs_6_5", R"(assets\shaders\VXGI_DispatchArgs.hlsl)");

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
		Shader computeShader = RS->LoadShader("CreateRemoveArgs", "cs_6_5", R"(assets\shaders\VXGI_RemoveArgs.hlsl)");

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
		Shader computeShader = RS->LoadShader("CreateIndirectArgs", "cs_6_5", R"(assets\shaders\VXGI_DecrementCounter.hlsl)");

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
		Shader computeShader = RS->LoadShader("CreateDrawArgs", "cs_6_5", R"(assets\shaders\VXGI_DrawArgs.hlsl)");

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
		Shader computeShader = RS->LoadShader("GatherSubdivionRequests", "cs_6_5", R"(assets\shaders\VXGI.hlsl)");

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
		Shader computeShader = RS->LoadShader("ProcessSubdivionRquests", "cs_6_5", R"(assets\shaders\VXGI.hlsl)");

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
		Shader computeShader = RS->LoadShader("Init", "cs_6_5", R"(assets\shaders\VXGI_InitOctree.hlsl)");

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
		auto VShader = RS->LoadShader("FullScreenQuad_VS",  "vs_6_5", "assets\\shaders\\VoxelDebugVis.hlsl");
		auto PShader = RS->LoadShader("VoxelDebug_PS",      "ps_6_5", "assets\\shaders\\VoxelDebugVis.hlsl");

		CD3DX12_BLEND_DESC blendMode = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendMode.RenderTarget[0].BlendEnable   = true;
		blendMode.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

		blendMode.RenderTarget[0].DestBlend     = D3D12_BLEND::D3D12_BLEND_ONE;
		blendMode.RenderTarget[0].SrcBlend      = D3D12_BLEND::D3D12_BLEND_ONE;

		blendMode.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
		blendMode.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable                  = false;
		Depth_Desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = blendMode;
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			//PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
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
		auto VShader = RS->LoadShader("voxelize_VS", "vs_6_5", "assets\\shaders\\Voxelizer.hlsl");
		auto GShader = RS->LoadShader("voxelize_GS", "gs_6_5", "assets\\shaders\\Voxelizer.hlsl");
		auto PShader = RS->LoadShader("voxelize_PS", "ps_6_5", "assets\\shaders\\Voxelizer.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.ConservativeRaster            = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		Rast_Desc.CullMode                      = D3D12_CULL_MODE_NONE;

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


	/************************************************************************************************/


	ID3D12PipelineState* StaticVoxelizer::CreateGatherArgsPSO(RenderSystem* RS)
	{
		return LoadComputeShader(
			RS->LoadShader("Main", "cs_6_5", R"(assets\shaders\SVO_VoxelGatherArgs.hlsl)"),
			markSignature,
			*RS);
	}


	/************************************************************************************************/


	ID3D12PipelineState* StaticVoxelizer::CreateMarkNodesPSO(RenderSystem* RS)
	{
		return LoadComputeShader(
			RS->LoadShader("MarkNodes", "cs_6_5", R"(assets\shaders\Voxelizer_MarkNodes.hlsl)"),
			markSignature,
			*RS);
	}


	/************************************************************************************************/


	ID3D12PipelineState* StaticVoxelizer::CreateExpandNodesPSO(RenderSystem* RS)
	{
		return LoadComputeShader(
			RS->LoadShader("ExpandNodes", "cs_6_5", R"(assets\shaders\Voxelizer_ExpandNodes.hlsl)"),
			markSignature,
			*RS);
	}


	/************************************************************************************************/


	ID3D12PipelineState* StaticVoxelizer::CreateFillAttributesPSO(RenderSystem* RS)
	{
		return LoadComputeShader(
			RS->LoadShader("FillNodes", "cs_6_5", R"(assets\shaders\Voxelizer_BuildHighestMipLevel.hlsl)"),
			markSignature,
			*RS);
	}


	/************************************************************************************************/


	ID3D12PipelineState* StaticVoxelizer::CreateBuildMIPLevelPSO(RenderSystem* RS)
	{
		return LoadComputeShader(
			RS->LoadShader("BuildLevel", "cs_6_5", R"(assets\shaders\Voxelizer_BuildMIPLevel.hlsl)"),
			markSignature,
			*RS);
	}


	/************************************************************************************************/


	StaticVoxelizer::StaticVoxelizer(RenderSystem& renderSystem, iAllocator& allocator) :
		voxelizeSignature   { &allocator },
		markSignature       { &allocator }
	{
		voxelizeSignature.AllowIA = true;
		voxelizeSignature.AllowSO = false;

		DesciptorHeapLayout layout{};
		layout.SetParameterAsShaderUAV(0, 0, 1, 0);

		voxelizeSignature.SetParameterAsUINT(0, 12, 0, 0, PIPELINE_DEST_ALL);
		voxelizeSignature.SetParameterAsCBV(1, 1, 0, PIPELINE_DEST_ALL);
		voxelizeSignature.SetParameterAsDescriptorTable(2, layout, -1, PIPELINE_DEST_PS);
		voxelizeSignature.Build(renderSystem, &allocator);

		DesciptorHeapLayout UAVLayout{};
		UAVLayout.SetParameterAsShaderUAV(0, 0, 3, 0);

		markSignature.AllowIA = false;
		markSignature.AllowSO = false;

		markSignature.SetParameterAsDescriptorTable(0, UAVLayout);
		markSignature.SetParameterAsSRV(1, 0);
		markSignature.SetParameterAsUINT(2, 4, 0, 0);
		markSignature.SetParameterAsSRV(3, 1);
		markSignature.Build(renderSystem, &allocator);

		renderSystem.RegisterPSOLoader(
			SVO_GatherArguments,
			{
				&voxelizeSignature,
				[&](RenderSystem* RS)
				{
					return CreateGatherArgsPSO(RS);
				}
			});

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
			SVO_GATHERSUBDIVISIONREQUESTS,
			{
				&markSignature,
				[&](RenderSystem* RS)
				{
					return CreateMarkNodesPSO(RS);
				}
			});

		renderSystem.RegisterPSOLoader(
			SVO_EXPANDNODES,
			{
				&markSignature,
				[&](RenderSystem* RS)
				{
					return CreateExpandNodesPSO(RS);
				}
			});

		renderSystem.RegisterPSOLoader(
			SVO_FILLNODES,
			{
				&markSignature,
				[&](RenderSystem* RS)
				{
					return CreateFillAttributesPSO(RS);
				}
			});

		renderSystem.RegisterPSOLoader(
			SVO_BUILDMIPLEVEL,
			{
				&markSignature,
				[&](RenderSystem* RS)
				{
					return CreateBuildMIPLevelPSO(RS);
				}
			});

		dispatch = renderSystem.CreateIndirectLayout(
			{   {   ILE_RootDescriptorUINT,
					IndirectDrawDescription::Constant{
						.rootParameterIdx   = 2,
						.destinationOffset  = 0,
						.numValues          = 4 }},
				{ ILE_DispatchCall }
			},
			&allocator,
			&markSignature
		);
	}


	/************************************************************************************************/


	void StaticVoxelizer::GatherArgs(FrameResourceHandle argBuffer, FrameResourceHandle sampleBuffer, ResourceHandler& resources, Context& ctx, iAllocator& TL_allocator, const size_t offset)
	{
		static const auto gatherArgs = resources.GetPipelineState(SVO_GatherArguments);
		ctx.SetComputeRootSignature(markSignature);
		ctx.SetPipelineState(gatherArgs);

		DescriptorHeap heap;
		heap.Init2(ctx, markSignature.GetDescHeap(0), 2, &TL_allocator);
		heap.SetUAVStructured(ctx, 0, resources.UAV(argBuffer, ctx), 32, offset);
		heap.NullFill(ctx, 2);

		ctx.SetComputeDescriptorTable(0, heap);
		ctx.SetComputeShaderResourceView(1, resources.NonPixelShaderResource(sampleBuffer, ctx));

		ctx.Dispatch( uint3{ 1, 1, 1 } );
	}


	/************************************************************************************************/


	StaticVoxelizer::VoxelizePass& StaticVoxelizer::VoxelizeScene(FrameGraph& frameGraph, Scene& scene, ResourceHandle octreeBuffer, uint3 XYZ, GatherPassesTask& passes, ReserveConstantBufferFunction reserveCB)
	{
		return frameGraph.AddNode<VoxelizePass>(
			VoxelizePass{
				passes,
				reserveCB
			},
			[&](FrameGraphNodeBuilder& builder, VoxelizePass& data)
			{
				data.sampleBuffer   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource((256 + 128)* MEGABYTE ), DASUAV);
				data.tempBuffer     = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(32 * MEGABYTE), DASUAV);
				data.argBuffer      = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4096), DASUAV);
				data.parentBuffer   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(32 * MEGABYTE), DASUAV);
				data.octree         = builder.UnorderedAccess(octreeBuffer);
			},
			[&scene, this](VoxelizePass& data, ResourceHandler& resources, Context& ctx, iAllocator& TL_allocator)
			{
				ctx.BeginEvent_DEBUG("Static Voxelizer");

				const ConstantBufferDataSet cameraConstants;

				CommonResources voxelizerResources = {
					.cameraConstants    = cameraConstants,
					.octree             = data.octree,
					.parentBuffer       = data.parentBuffer,
					.sampleBuffer       = data.sampleBuffer,
					.argBuffer          = data.argBuffer,
					.tempBuffer         = data.tempBuffer,
					.reserveCB          = data.reserveCB,
				};

				GatherSamples(
					voxelizerResources,
					scene,
					resources,
					ctx,
					TL_allocator);

				BuildTree(
					voxelizerResources,
					resources,
					ctx,
					TL_allocator);

				BuildMIPLevels(
					voxelizerResources,
					resources,
					ctx,
					TL_allocator);

				ctx.EndEvent_DEBUG();
			});
	}


	/************************************************************************************************/


	void StaticVoxelizer::GatherSamples(CommonResources& resources, Scene& scene, ResourceHandler& resourcesHandler, Context& ctx, iAllocator& TL_allocator)
	{
		auto voxelize               = resourcesHandler.GetPipelineState(SVO_Voxelize);
		auto& visabilityComponent   = SceneVisibilityComponent::GetComponent();
		auto& brushComponent        = BrushComponent::GetComponent();

		Vector<BrushHandle> brushes{ &TL_allocator };
		for (auto& entityHandle : scene.sceneEntities)
		{
			Apply(*visabilityComponent[entityHandle].entity,
				[&](MaterialView& material,
					BrushView& brush)
				{
					auto passes = material.GetPasses();

					if (std::find(passes.begin(), passes.end(), GetCRCGUID(VXGI_STATIC)))
						brushes.push_back(brush.brush);
				});
		}

		CBPushBuffer constantBuffer{ resources.reserveCB(AlignedSize<float4x4>() * brushes.size()) };

		ctx.BeginEvent_DEBUG("Voxel Pass");

		ctx.ClearUAVBufferRange(resourcesHandler.UAV(resources.sampleBuffer, ctx), 0, 1024);
		ctx.AddUAVBarrier(resourcesHandler.GetResource(resources.sampleBuffer));

		ctx.SetRootSignature(voxelizeSignature);
		ctx.SetPipelineState(voxelize);

		constexpr int32_t VoxelEdgeLength = 256;
		constexpr int32_t VoxelResolution = 2048;

		float4 values[] = {
			{ VoxelEdgeLength, VoxelEdgeLength, VoxelEdgeLength, VoxelEdgeLength },
			{ VoxelResolution, VoxelResolution, VoxelResolution, 0 },
			{ VoxelEdgeLength / -2, VoxelEdgeLength / -2, VoxelEdgeLength / -2, 0 },
		};

		ctx.SetViewports({ D3D12_VIEWPORT{ 0, 0, VoxelResolution, VoxelResolution, 0, 1.0f } });
		ctx.SetScissorRects({ D3D12_RECT{ 0, 0, VoxelResolution, VoxelResolution} });
		ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
		ctx.SetGraphicsConstantValue(0, 12, values);
				
		DescriptorHeap heap;
		heap.Init(ctx, voxelizeSignature.GetDescHeap(0), &TL_allocator);
		heap.SetUAVStructured(ctx, 0,
			resourcesHandler.UAV(resources.sampleBuffer, ctx),
			resourcesHandler.UAV(resources.sampleBuffer, ctx), 16, 0);

		ctx.SetGraphicsDescriptorTable(2, heap);

		for (const auto& brush : brushes)
		{
			auto meshes = brushComponent[brush].meshes;

			for(auto mesh : meshes)
			{
				auto* const triMesh = GetMeshResource(mesh);
				const auto lod      = triMesh->GetHighestLoadedLodIdx();

				const float4x4  WT      = GetWT(brushComponent[brush].Node).Transpose();
				const float4    albedo  = GetMaterialProperty<float4>(brushComponent[brush].material, GetCRCGUID(PBR_ALBEDO)).value_or(float4{ 1.0f, 0.0f, 1.0f, 0.0f });

				struct sVoxelConstants
				{
					float4x4    WT;
					float4      albedo;
				};

				const ConstantBufferDataSet entityConstants{ sVoxelConstants{ WT, albedo }, constantBuffer };

				ctx.SetGraphicsConstantBufferView(1, entityConstants);

				ctx.AddIndexBuffer(triMesh, lod);
				ctx.AddVertexBuffers(triMesh,
					lod,
					{   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV });

				ctx.DrawIndexed(triMesh->lods[lod].GetIndexCount());
			}
		}

		ctx.EndEvent_DEBUG();
	}


	/************************************************************************************************/


	void StaticVoxelizer::BuildTree(CommonResources& resources, ResourceHandler& resourceHandler, Context& ctx, iAllocator& TL_allocator)
	{
		const auto gatherSubDRequests = resourceHandler.GetPipelineState(SVO_GATHERSUBDIVISIONREQUESTS);
		const auto expandNodes        = resourceHandler.GetPipelineState(SVO_EXPANDNODES);
		const auto fillNodes          = resourceHandler.GetPipelineState(SVO_FILLNODES);

		ctx.BeginEvent_DEBUG("Build");

		ctx.DiscardResource(resourceHandler.GetResource(resources.argBuffer));
		ctx.ClearUAVBuffer(resourceHandler.GetResource(resources.argBuffer));

		ctx.AddUAVBarrier(resourceHandler.GetResource(resources.argBuffer));

		GatherArgs(resources.argBuffer, resources.sampleBuffer, resourceHandler, ctx, TL_allocator, 1);

		ctx.AddUAVBarrier(resourceHandler.GetResource(resources.argBuffer));

		for(size_t I = 0; I < 11; I++)
		{   // Mark nodes
			ctx.BeginEvent_DEBUG("Mark and expand Pass");

			ctx.ClearUAVBufferRange(resourceHandler.UAV(resources.tempBuffer, ctx), 0, 4096);
			ctx.AddUAVBarrier(resourceHandler.UAV(resources.tempBuffer, ctx));

			DescriptorHeap UAVHeap{};
			UAVHeap.Init2(ctx, markSignature.GetDescHeap(0), 2, &TL_allocator);
			UAVHeap.SetUAVStructured(ctx, 0, resourceHandler.UAV(resources.tempBuffer, ctx), resourceHandler.UAV(resources.tempBuffer, ctx), 4, 0);
			UAVHeap.SetUAVBuffer(ctx, 1, resourceHandler.UAV(resources.octree, ctx), 4096 / 4);

			ctx.SetComputeRootSignature(markSignature);
			ctx.SetPipelineState(gatherSubDRequests);
			ctx.SetComputeDescriptorTable(0, UAVHeap);
			ctx.SetComputeShaderResourceView(1, resourceHandler.NonPixelShaderResource(resources.sampleBuffer, ctx));

			ctx.ExecuteIndirect(resourceHandler.IndirectArgs(resources.argBuffer, ctx), dispatch, 32);

			// Expand Nodes
			GatherArgs(resources.argBuffer, resources.tempBuffer, resourceHandler, ctx, TL_allocator);

			DescriptorHeap UAVHeap2{};
			UAVHeap2.Init2(ctx, markSignature.GetDescHeap(0), 3, &TL_allocator);
			UAVHeap2.SetUAVBuffer(ctx, 0, resourceHandler.UAV(resources.octree, ctx), 4096 / 4);
			UAVHeap2.SetUAVBuffer(ctx, 1, resourceHandler.UAV(resources.octree, ctx), 0);
			UAVHeap2.SetUAVBuffer(ctx, 2, resourceHandler.UAV(resources.parentBuffer, ctx), 0);

			ctx.SetPipelineState(expandNodes);
			ctx.SetComputeDescriptorTable(0, UAVHeap2);
			ctx.SetComputeShaderResourceView(1, resourceHandler.NonPixelShaderResource(resources.tempBuffer, ctx), 4096);

			ctx.ExecuteIndirect(resourceHandler.IndirectArgs(resources.argBuffer, ctx), dispatch);

			ctx.EndEvent_DEBUG();
		}

		ctx.BeginEvent_DEBUG("Fill Highest Level");

		ctx.ClearUAVBufferRange(resourceHandler.UAV(resources.tempBuffer, ctx), 0, 4096);
		ctx.AddUAVBarrier(resourceHandler.UAV(resources.tempBuffer, ctx));
		ctx.AddUAVBarrier(resourceHandler.UAV(resources.octree, ctx));
		ctx.AddUAVBarrier(resourceHandler.UAV(resources.parentBuffer, ctx));

		// Fill Highest level
		DescriptorHeap UAVHeap{};
		UAVHeap.Init2(ctx, markSignature.GetDescHeap(0), 3, &TL_allocator);
		UAVHeap.SetUAVStructured(ctx, 0, resourceHandler.UAV(resources.tempBuffer, ctx), resourceHandler.UAV(resources.tempBuffer, ctx), 4, 0);
		UAVHeap.SetUAVBuffer(ctx, 1, resourceHandler.UAV(resources.octree, ctx), 4096 / 4);
		UAVHeap.SetUAVBuffer(ctx, 2, resourceHandler.UAV(resources.parentBuffer, ctx));

		ctx.SetComputeRootSignature(markSignature);
		ctx.SetComputeDescriptorTable(0, UAVHeap);
		ctx.SetComputeShaderResourceView(1, resourceHandler.NonPixelShaderResource(resources.sampleBuffer, ctx));
		ctx.SetPipelineState(fillNodes);

		ctx.ExecuteIndirect(resourceHandler.IndirectArgs(resources.argBuffer, ctx), dispatch, 32);

		ctx.EndEvent_DEBUG();
		ctx.EndEvent_DEBUG();
	}


	/************************************************************************************************/


	void StaticVoxelizer::BuildMIPLevels(CommonResources& resources, ResourceHandler& resourceHandler, Context& ctx, iAllocator& TL_allocator)
	{
		const auto buildLevel = resourceHandler.GetPipelineState(SVO_BUILDMIPLEVEL);

		FrameResourceHandle tempBuffers[2] = {
			resources.tempBuffer,
			resources.sampleBuffer
		};

		ctx.BeginEvent_DEBUG("Build MIP Levels");

		for(size_t I = 0; I < 11; I++)
		{   // Mark nodes
			ctx.BeginEvent_DEBUG("Build Level");

			const auto source = tempBuffers[(I + 0u) % 2u];
			const auto target = tempBuffers[(I + 1u) % 2u];

			GatherArgs(resources.argBuffer, source, resourceHandler, ctx, TL_allocator);

			ctx.ClearUAVBufferRange(resourceHandler.UAV(target, ctx), 0, 4096);
			ctx.AddUAVBarrier(resourceHandler.UAV(target, ctx));

			DescriptorHeap UAVHeap{};
			UAVHeap.Init2(ctx, markSignature.GetDescHeap(0), 3, &TL_allocator);
			UAVHeap.SetUAVStructured(ctx, 0,                                                                            // dirtyParents : register u0
				resourceHandler.UAV(target, ctx),
				resourceHandler.UAV(target, ctx), 4, 0);
			UAVHeap.SetUAVBuffer(ctx, 1, resourceHandler.UAV(resources.octree, ctx), 4096 / 4);                         // octree  : register(u1)

			ctx.SetComputeRootSignature(markSignature);
			ctx.SetComputeDescriptorTable(0, UAVHeap);
			ctx.SetComputeShaderResourceView(1, resourceHandler.NonPixelShaderResource(source, ctx), 4096);             // dirtyNodes   : register(t0)
			ctx.SetComputeShaderResourceView(3, resourceHandler.NonPixelShaderResource(resources.parentBuffer, ctx));   // parents      : register(t1)
			ctx.SetPipelineState(buildLevel);

			ctx.ExecuteIndirect(resourceHandler.IndirectArgs(resources.argBuffer, ctx), dispatch);

			ctx.EndEvent_DEBUG();
		}

		ctx.EndEvent_DEBUG();
	}


}   /************************************************************************************************/
