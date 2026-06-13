#include "RTExperiment.hpp"
#include "OBJLoader.hpp"

using namespace FlexKit;


RTExperimentState::RTExperimentState(GameFramework& IN_framework) :
	FrameworkState{ IN_framework },

	brushes		{ GetAllocator() },
	cameras		{ GetAllocator() },
	lights		{ GetAllocator() },
	materials	{ GetRenderSystem(), GetAllocator() },
	visibility	{ GetAllocator() },
	scene		{ GetAllocator() },
	triggers	{ GetAllocator(), GetAllocator() },
	gameObjects	{ GetAllocator(), 1024 },

	depthBuffer	{ GetRenderSystem(), { 800, 600 } },
	persistent	{ 64, GetAllocator() },
	gpuAllocator{ 64 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::UAVTextures | DeviceHeapFlags::UAVBuffer, framework.core.GetBlockMemory() }
{
	Win32RenderWindowDesc windowDesc = DefaultWindowDesc({ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT);

	DescriptorSetLayout layout{ GetAllocator() };
	layout.SetParameterAsUAV(0, 0, 1, 0);
	layout.SetParameterAsSRV(1, 1, 1, 0);

	PipelineInterfaceBuilder builder{ GetTempAllocator() };
	builder.SetParameterAsUINT(0, 16, 0, 0);
	builder.SetParameterAsDescriptorSet(1, layout);

	InitiateSceneNodeBuffer(GetAllocator());

	globalInterface = builder.Build(GetTempAllocator());
	renderWindow = CreateWin32RenderWindow(GetRenderSystem(), windowDesc);

	library = LoadShaderLibrary("assets/shaders/rtLibrary.hlsl", globalInterface);
	raygenID = library->FindShaderFunction("raygen_main");
	missID = library->FindShaderFunction("miss_main");
	defaultGroup1 = library->FindShaderFunction("defaultHitGroup");

	GetRenderSystem().RegisterPSOLoader(GetTypeGUID(Trangle),
		[](IRenderSystem& renderSystem, iAllocator& allocator)
		{
			PipelineBuilder builder(renderSystem, allocator);
			builder.AddInputLayout({
				.inputs = {
					{
						.name			= "POSITION",
						.index			= 0,
						.format			= DeviceFormat::R32G32B32_FLOAT,
						.inputSlotClass = EInputClassification::PerVertex,
					},
					{
						.name			= "NORMAL",
						.index			= 1,
						.format			= DeviceFormat::R32G32B32_FLOAT,
						.inputSlotClass = EInputClassification::PerVertex,
					},
				},
				.count = 1
				});
			//builder.AddVertexShader("VMain");
			//builder.AddPixelShader("PMain");
			builder.AddVertexShader("VMain", "assets/shaders/TestShader.hlsl", ShaderOptions{ .enableDebug = true });
			builder.AddPixelShader("PMain", "assets/shaders/TestShader.hlsl", ShaderOptions{ .enableDebug = true });
			builder.AddRasterizerState({
				.CullMode = ECullMode::BACK
			});
			builder.AddRenderTargetState({
					.targetCount = 1,
					.targetFormats = { DeviceFormat::R16G16B16A16_FLOAT },
				});

			return builder.Build(renderSystem, allocator);
		});

	GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

	vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
	cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);

	suzanneMesh = LoadObj("suzanne.obj");
	lightMesh = LoadObj("light.obj");
	roomMesh = LoadObj("room.obj");

	auto& suzanneObj = gameObjects.Allocate();
	auto& lightObj = gameObjects.Allocate();
	auto& roomObj = gameObjects.Allocate();
	auto& cameraObj = gameObjects.Allocate();

	suzanneObj.AddView<TriggerView>();
	lightObj.AddView<TriggerView>();
	roomObj.AddView<TriggerView>();

	MaterialHandle lightMaterial = materials.CreateMaterial();
	MaterialHandle roomMaterial = materials.CreateMaterial();
	MaterialHandle suzanneMaterial = materials.CreateMaterial();

	materials.Add2Pass(lightMaterial, RTPass);
	materials.Add2Pass(roomMaterial, RTPass);
	materials.Add2Pass(suzanneMaterial, RTPass);

	auto& cameraView = cameraObj.AddView<CameraView>();
	activeCamera = cameraView.camera;
	cameraView.SetCameraNode(GetZeroedNode());
	cameraView.SetCameraFOV(pi / 4);

	auto& suzanneBrush = suzanneObj.AddView<BrushView>(suzanneMesh);
	auto& lightBrush = lightObj.AddView<BrushView>(lightMesh);
	auto& roomBrush = roomObj.AddView<BrushView>(roomMesh);

	suzanneObj.AddView<SceneNodeView>(GetZeroedNode());
	lightObj.AddView<SceneNodeView>(GetZeroedNode());
	roomObj.AddView<SceneNodeView>(GetZeroedNode());

	suzanneBrush.SetMaterial(suzanneMaterial);
	lightBrush.SetMaterial(lightMaterial);
	roomBrush.SetMaterial(roomMaterial);

	scene.OwnGameObject(suzanneObj);
	scene.OwnGameObject(lightObj);
	scene.OwnGameObject(roomObj);

	SBTMemory		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
	hitTable		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
	missTable		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
	rayGenerator	= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
}


RTExperimentState::~RTExperimentState()
{
	GetRenderSystem().ReleaseVB(vBuffer);
	GetRenderSystem().ReleaseCB(cBuffer);
}


UpdateTask* RTExperimentState::Update(EngineCore&, UpdateDispatcher&, double dT)
{
	t += dT;

	Win32UpdateInput();
	return nullptr;
}


UpdateTask* RTExperimentState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);
	auto& transformUpdate	= QueueTransformUpdateTask(dispatcher);
	auto& sceneUpdate		= scene.UpdateSceneBVH(dispatcher, transformUpdate, GetAllocatorMT());
	auto& passes			= GatherScene(dispatcher, &scene, activeCamera, GetAllocatorMT());

	passes.AddInput(sceneUpdate);
	cameraUpdate.AddOutput(transformUpdate);

	auto renderTarget = renderWindow->GetBackBuffer();
	frameGraph.AddOutput(renderTarget);
	frameGraph.AddOutput(persistent.resource);
	frameGraph.AddMemoryPool(gpuAllocator);
	frameGraph.AddConstantBuffer(cBuffer);

	ClearBackBuffer(frameGraph, renderTarget, { sinf(t) * 0.5f + 0.5f, cosf(t * 5.0f) * 0.5f + 0.5f, tanf(t * 10.0f) * 0.5f + 0.5f, 1 });
	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);


	auto& sbtUpdate = UpdateSBT(frameGraph, passes);
	if (!trace)
	{
		ForwardPass(frameGraph, renderTarget, passes, cameraUpdate);

		PresentBackBuffer(frameGraph, *renderWindow);
	}
	else
	{
		PathTracePass(frameGraph, cameraUpdate, sbtUpdate, renderTarget);
	}

	return nullptr;
}


ForwardPassData& RTExperimentState::ForwardPass(FrameGraph& frameGraph, ResourceHandle renderTarget, GatherPassesTask& passes, CameraUpdateTask& cameraUpdate)
{
	return frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder) -> ForwardPassData
		{
			builder.Requires(GetTypeGUID(Trangle));
			builder.AddDataDependency(cameraUpdate);
			builder.AddDataDependency(passes);

			return ForwardPassData{
				.renderTarget = builder.RenderTarget(renderTarget),
			};
		},
		[=, this, &passes](const ForwardPassData& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
		{
			float fTime = (float)t;

			struct
			{
				float time;
			} constants0{
				.time = fTime,
			};

			const IPipelineInterface* pipelineInterface = resources.GetPipelineState(GetTypeGUID(Trangle), threadLocalAllocator)->GetInterface();

			ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), threadLocalAllocator);
			ctx.SetInputPrimitive(EInputPrimitive::INPUTPRIMITIVETRIANGLELIST);
			ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
			ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });


			for (auto& brushDraw : passes.GetData().GetPass(RTPass))
			{
				ctx.SetGraphicsConstantValue(0, 1, &constants0);

				for (auto& mesh : brushDraw.brush->meshes)
				{
					auto meshResource = GetMeshResource(mesh);
					auto& lod = meshResource->lods[0];

					ctx.AddVertexBuffers(meshResource, 0, { VERTEXBUFFER_TYPE::POSITION, VERTEXBUFFER_TYPE::NORMAL });
					ctx.AddIndexBuffer(meshResource, 0);

					ctx.DrawIndexed(lod.GetIndexCount());
				}
			}
		});
}


UpdateSBTData& RTExperimentState::UpdateSBT(FrameGraph& frameGraph, GatherPassesTask& passes)
{
	auto& sbtUpdate = frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder) -> UpdateSBTData
		{
			builder.Requires(GetTypeGUID(Trangle));
			builder.AddDataDependency(passes);

			return UpdateSBTData{
				.sbtBuffer = builder.CopyDest(persistent.resource)
			};
		},
		[=, &passes, this](const UpdateSBTData& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
		{
			static bool initRayGenerator = [&]()
				{
					struct
					{
						uint8_t programID[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
					} rayGeneratorRecord;

					struct
					{
						uint8_t programID[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
						uint8_t rgba;
					} missGeneratorRecord;

					struct
					{
						uint8_t programID[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
					} hitGroups[1] = {

					};

					auto rayGeneratorUpload = ctx.ReserveDirectUploadSpace(64, sizeof(rayGeneratorRecord));
					auto missShaderUpload = ctx.ReserveDirectUploadSpace(64, sizeof(missGeneratorRecord));
					auto hitGroupsUpload = ctx.ReserveDirectUploadSpace(64, sizeof(hitGroups));

					memcpy(rayGeneratorRecord.programID, (void*)raygenID.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(missGeneratorRecord.programID, (void*)missID.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(hitGroups[0].programID, (void*)defaultGroup1.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(rayGeneratorUpload.buffer, &rayGeneratorRecord, sizeof(rayGeneratorRecord));
					memcpy(missShaderUpload.buffer, &missGeneratorRecord, sizeof(missGeneratorRecord));
					memcpy(hitGroupsUpload.buffer, &hitGroups, sizeof(hitGroups));

					ctx.CopyBuffer(rayGeneratorUpload,	rayGenerator.resource,	rayGenerator.offset);
					ctx.CopyBuffer(missShaderUpload,	missTable.resource,		missTable.offset);
					ctx.CopyBuffer(hitGroupsUpload,		hitTable.resource,		hitTable.offset);

					return true;
				}();

			auto rtPass = passes.GetData().GetPass(RTPass);

			for (auto& brush : rtPass)
			{

			}
		});

	return sbtUpdate;
}


TracePassData& RTExperimentState::PathTracePass(FrameGraph& frameGraph, CameraUpdateTask& cameraUpdate, UpdateSBTData& sbtUpdate, ResourceHandle renderTarget)
{
	auto& tracePass = frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder) -> TracePassData
		{
			builder.Requires(GetTypeGUID(Trangle));
			builder.AddDataDependency(cameraUpdate);

			return TracePassData{
				.sbtBuffer = builder.ReadTransition(
								sbtUpdate.sbtBuffer, DeviceAccessState::DASNonPixelShaderResource,
								{ DeviceSyncPoint::Sync_Copy, DeviceSyncPoint::Sync_Raytracing }),

				.traceBuffer = builder.AcquireVirtualResource(
								GPUResourceDesc::UAVTexture(
								{ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT, false),	DeviceAccessState::DASUAV),

				.renderTarget = builder.RenderTarget(renderTarget),

#if 0
						.renderTarget = builder.WriteTransition(
										drawPass.renderTarget,
										DeviceAccessState::DASCopyDest,
										{ DeviceSyncPoint::Sync_Draw, DeviceSyncPoint::Sync_Copy }),
#endif
			};
		},
		[=, this](const TracePassData& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
		{
			auto& set0Layout = globalInterface->GetDescriptorSetLayout(0);
			DescriptorSet set(ctx, set0Layout, threadLocalAllocator);
			set.SetUAVTexture(ctx, 0, resources.GetResource(data.traceBuffer));
			set.NullFill(ctx);
			ctx.SetRTStateObject(library->FindShaderFunction("raygen_main"), library);

			// Update SBT
			D3D12_DISPATCH_RAYS_DESC rayDesc{};
			rayDesc.RayGenerationShaderRecord =
				D3D12_GPU_VIRTUAL_ADDRESS_RANGE{
					rayGenerator.devicePtr, rayGenerator.size
			};
			rayDesc.HitGroupTable =
				D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
					hitTable.devicePtr, 32, 32,
			};
			rayDesc.MissShaderTable =
				D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
					missTable.devicePtr, 32, 32,
			};

			rayDesc.Width = 800;
			rayDesc.Height = 600;
			rayDesc.Depth = 1;

			ctx.DiscardResource(resources.GetResource(data.traceBuffer));
			ctx.ClearUAVTextureFloat(resources.GetResource(data.traceBuffer));
			ctx.AddUAVBarrier(
				resources.GetResource(data.traceBuffer),
				-1, DeviceLayout::UnorderedAccess,
				DeviceSyncPoint::Sync_ClearUAV, DeviceSyncPoint::Sync_Raytracing);

			auto& dxCtx = static_cast<dx_Internal::dxDirectContext&>(ctx);
			dxCtx.FlushBarriers();
			dxCtx.SetComputeRootSignature(globalInterface);
			dxCtx.SetComputeDescriptorSet(0, set);
			dxCtx.DeviceContext->DispatchRays(&rayDesc);

			ctx.CopyResource(
				resources.GetResource(data.renderTarget),
				resources.CopySrc(data.traceBuffer, ctx, DeviceSyncPoint::Sync_Raytracing, DeviceSyncPoint::Sync_Copy));

		});

	frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder)
		{
			builder.ReadTransition(tracePass.renderTarget, DeviceAccessState::DASPresent, { DeviceSyncPoint::Sync_Copy, DeviceSyncPoint::Sync_All });
		},
		[=, this](const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.FlushBarriers();
		});

	return tracePass;
}


void RTExperimentState::PostDrawUpdate(FlexKit::EngineCore& core, double dT)
{
	renderWindow->Present();
	depthBuffer.Increment();
	core.RenderSystem->ResetVertexBuffer(vBuffer);
}
