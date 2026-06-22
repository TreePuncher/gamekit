#include "RTExperiment.hpp"
#include "OBJLoader.hpp"
#include <print>

using namespace FlexKit;


RTExperimentState::RTExperimentState(GameFramework& IN_framework) :
	FrameworkState{ IN_framework },

	brushes{ GetAllocator() },
	cameras{ GetAllocator() },
	lights{ GetAllocator() },
	materials{ GetRenderSystem(), GetAllocator() },
	visibility{ GetAllocator() },
	scene{ GetAllocator() },
	triggers{ GetAllocator(), GetAllocator() },
	gameObjects{ GetAllocator(), 1024 },

	sbt{ GetAllocator() },
	depthBuffer{ GetRenderSystem(), { 800, 600 } },
	persistent{ 256, GetAllocator() },
	gpuAllocator{ 64 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::UAVTextures | DeviceHeapFlags::UAVBuffer, framework.core.GetBlockMemory() }
{
	InitiateSceneNodeBuffer(GetAllocator());

	Win32RenderWindowDesc windowDesc = DefaultWindowDesc({ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT);

	DescriptorSetLayout layout{ GetTempAllocator() };
	layout.AddUAVs(1);
	layout.AddSRVs(1);

	PipelineInterfaceBuilder builder{ GetTempAllocator() };
	builder.SetParameterAsUINT(0, 20);
	builder.SetParameterAsDescriptorSet(1, layout);

	globalInterface = builder.Build(GetAllocator());
	renderWindow = CreateWin32RenderWindow(GetRenderSystem(), windowDesc);

	library			= LoadShaderLibrary("assets/shaders/rtLibrary.hlsl", globalInterface);
	raygenID		= library->FindShaderFunction("raygen_main");
	missID			= library->FindShaderFunction("miss_main");
	defaultMaterial = library->FindShaderFunction("DefaultMaterial");
	lightMaterial	= library->FindShaderFunction("LightMaterial");

	GetRenderSystem().RegisterPSOLoader(GetTypeGUID(Trangle),
		[&](IRenderSystem& renderSystem, iAllocator& allocator)
		{
			PipelineBuilder builder(renderSystem, allocator);
			builder.AddInputLayout({
				.inputs = {
					{
						.name			= "POSITION",
						.format			= DeviceFormat::R32G32B32_FLOAT,
						.inputSlotClass = EInputClassification::PerVertex,
					},
					{
						.name			= "NORMAL",
						.format			= DeviceFormat::R32G32B32_FLOAT,
						.slot			= 1,
						.inputSlotClass = EInputClassification::PerVertex,
					},
				},
				.count = 2
				});
			//builder.AddVertexShader("VMain");
			//builder.AddPixelShader("PMain");
			builder.AddVertexShader("VMain", "assets/shaders/TestShader.hlsl",	ShaderOptions{});
			builder.AddPixelShader("PMain", "assets/shaders/TestShader.hlsl",	ShaderOptions{});
			builder.AddRasterizerState({
				.CullMode = ECullMode::BACK,
			});
			builder.AddRenderTargetState({
					.targetCount	= 1,
					.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
				});
			builder.AddDepthStencilFormat(DeviceFormat::D32_FLOAT);
			builder.AddDepthStencilState({
				.depthEnable = true,
			});

			return builder.Build(renderSystem, GetAllocatorMT());
		});

	GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

	vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
	cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);

	lightMesh = LoadObj("light.obj");
	suzanneMesh = LoadObj("suzanne.obj");
	roomMesh = LoadObj("room.obj");

	auto& suzanneObj = gameObjects.Allocate();
	auto& lightObj = gameObjects.Allocate();
	auto& roomObj = gameObjects.Allocate();
	cameraObj = &gameObjects.Allocate();

	suzanneObj.AddView<TriggerView>();
	lightObj.AddView<TriggerView>();
	roomObj.AddView<TriggerView>();

	auto& suzanneMaterial = suzanneObj.AddView<MaterialView>();
	auto& lightMaterial = lightObj.AddView<MaterialView>();
	auto& roomMaterial = roomObj.AddView<MaterialView>();

	lightMaterial.Add2Pass(RTPass);
	roomMaterial.Add2Pass(RTPass);
	suzanneMaterial.Add2Pass(RTPass);

	lightMaterial.SetProperty(LightIrradiance, 1.0f);
	roomMaterial.SetProperty(DiffuseColor, float3{ 0.5f, 0.5f, 0.5 });
	suzanneMaterial.SetProperty(DiffuseColor, float3{ 0.5f, 0.0f, 0.5 });

	auto& cameraView = cameraObj->AddView<CameraView>();
	auto& cameraNode = cameraObj->AddView<SceneNodeView>(GetZeroedNode());
	activeCamera = cameraView.camera;
	cameraView.SetCameraNode(cameraNode.node);
	cameraView.SetCameraFOV(pi / 4);
	cameraView.SetCameraAspectRatio(800.0f / 600.0f);
	cameraView.SetCameraFar(100.0f);
	cameraNode.Yaw(pi / -2.0f);
	cameraNode.TranslateLocal({ 7, 2, 0 });

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
	hitTable		= persistent.AllocBlocks(10, GetRenderSystem().GetCurrentCounter()).value();
	missTable		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
	rayGenerator	= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
	sceneInstances	= persistent.AllocBlocks(4, GetRenderSystem().GetCurrentCounter()).value();
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
	auto& sceneUpdate		= scene.UpdateSceneBVH(dispatcher, transformUpdate, GetTempAllocatorMT());
	auto& passes			= GatherScene(dispatcher, &scene, activeCamera, GetTempAllocatorMT());

	passes.AddInput(sceneUpdate);
	cameraUpdate.AddInput(transformUpdate);

	auto renderTarget = renderWindow->GetBackBuffer();
	frameGraph.AddOutput(renderTarget);
	frameGraph.AddOutput(persistent.resource);
	frameGraph.AddMemoryPool(gpuAllocator);
	frameGraph.AddConstantBuffer(cBuffer);

	ClearBackBuffer(frameGraph, renderTarget);
	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	auto& sbtUpdate = UpdateSBT(frameGraph, passes);
	if (!trace)
	{
		ForwardPass(frameGraph, renderTarget, passes, cameraUpdate);

		PresentBackBuffer(frameGraph, *renderWindow);
	}
	else
	{
		PathTracePass(frameGraph, passes, cameraUpdate, sbtUpdate, renderTarget);
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
				.renderTarget	= builder.RenderTarget(renderTarget),
				.depthTarget	= builder.DepthTarget(depthBuffer.Get()),
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

			auto cameraConstants = GetCameraConstants(activeCamera);
			CBPushBuffer			cbPushBuffer{ resources.ReserveCB(sizeof(cameraConstants)) };
			ConstantBufferDataSet	cameraCB{ cameraConstants, cbPushBuffer};

			const IPipelineInterface* pipelineInterface = resources.GetPipelineState(GetTypeGUID(Trangle), threadLocalAllocator)->GetInterface();

			ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), threadLocalAllocator);
			ctx.SetInputPrimitive(EInputPrimitive::INPUTPRIMITIVETRIANGLELIST);
			ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
			ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) }, true, resources.GetResource(data.depthTarget));
			ctx.SetGraphicsConstantBufferView(0, cameraCB);

			auto brushes = passes.GetData().GetPass(RTPass);

			for (auto& brushDraw : brushes)
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

#if 0
	struct SharedPassData
	{
	    
	};


	DataDrivenMultiPassDescription<SharedPassData, GatherPassesTask&, BrushEntry> passDescription
	{
		.sharedData = SharedPassData{},
		.getPVS	= [](auto& source) -> std::span<BrushEntry>
		{
			return {};
		},

		.getPasses =[] (iAllocator&)->Vector<BrushEntry>
		{
			return {};
		}
	};

	frameGraph.AddDataDrivenMultiPass(
		passDescription,
		[](auto& builder, SharedPassData& shared)
	    {
	    
	    },[](auto& begin, auto& end, auto& pvs, IDirectContext& ctx, iAllocator& local) {});

#endif

	struct TracableScene
	{
		Scene* scene = nullptr;
		const ResourceAllocation* resourceAllocation = nullptr;
	};

	PassDrivenResourceAllocation allocation
	{
		.getPass =
			[&]() -> std::span<const BrushEntry>
			{
				return passes.GetData().GetPass(PassHandle{ RTPass });
			},
		.initializeResources =
			[&](std::span<const BrushEntry> objects, std::span<FrameResourceHandle> frameHandles, auto& resourceCtx, iAllocator& allocator)
			{
				ProfileFunction();

				SceneVisibilityComponent& sceneVis = SceneVisibilityComponent::GetComponent();
				Vector<TriMeshHandle> handles{ &allocator };

				for (auto& mesh : scene.sceneEntities)
				{
					auto meshes = GetTriMesh(*sceneVis[mesh].entity);

					for (auto& mesh : meshes)
						handles.push_back(mesh);
				}

				std::ranges::sort(handles.begin(), handles.end());
				auto uniqueEnd = std::ranges::unique(handles.begin(), handles.end());

				handles.erase(uniqueEnd.begin(), uniqueEnd.end());

				auto range = std::ranges::remove_if(
					handles,
					[](TriMeshHandle& mesh)
					{
						auto meshResource = GetMeshResource(mesh);

						auto& lod = meshResource->GetLowestLoadedLod();

						return (lod.blAS != -1);
					});

				handles.erase(range.begin(), range.end());

				for (auto [meshHandle, frameHandle] : zip(handles, frameHandles))
				{
					auto	meshResource = GetMeshResource(meshHandle);
					auto& lod = meshResource->GetLowestLoadedLod();

					resourceCtx.BuildBLAS(frameHandle, lod);
				}
			},
		.layout		= DeviceLayout::Unknown,
		.access		= DeviceAccessState::DASACCELERATIONSTRUCTURE_READ,
		.max		= 16,
		.pool		= &gpuAllocator,
		.dependency = &passes,
	};



	const auto& allocationRes = frameGraph.AllocateResourceSet(allocation);
	auto& traceableScene = GetAllocator().allocate<TracableScene>();

	traceableScene.resourceAllocation = &allocationRes;
	traceableScene.scene = &scene;

	auto& sbtUpdate = frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder) -> UpdateSBTData
		{
			builder.Requires(GetTypeGUID(Trangle));
			builder.AddDataDependency(passes);
			builder.AddNodeDependency(traceableScene.resourceAllocation->node);

			return UpdateSBTData{
				.node		= builder.GetNodeHandle(),
				.sbtBuffer	= builder.CopyDest(persistent.resource),
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
						float4 rgba = { 218.0f/256.0f, 110.0f / 256.0f, 214.0f / 256.0f, 1 };
					} missGeneratorRecord;

					auto rayGeneratorUpload = ctx.ReserveDirectUploadSpace(64, sizeof(rayGeneratorRecord));
					auto missShaderUpload	= ctx.ReserveDirectUploadSpace(64, sizeof(missGeneratorRecord));

					memcpy(rayGeneratorRecord.programID, (void*)raygenID.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(missGeneratorRecord.programID, (void*)missID.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(rayGeneratorUpload.buffer, &rayGeneratorRecord, sizeof(rayGeneratorRecord));
					memcpy(missShaderUpload.buffer, &missGeneratorRecord, sizeof(missGeneratorRecord));

					ctx.CopyBuffer(rayGeneratorUpload, rayGenerator.resource, rayGenerator.offset);
					ctx.CopyBuffer(missShaderUpload, missTable.resource, missTable.offset);

					return true;
				}();


			auto rtPass = passes.GetData().GetPass(RTPass);

			struct alignas(128) HitGroupData
			{
				uint8_t		programID[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
				float4		color;
				uint32_t	test[12] = { 3, 0, 0, 3, 5 };
				uint64_t	index;
				uint64_t	position;
				uint64_t	normal;
			};

			Vector<HitGroupData>	hitGroupData{ threadLocalAllocator };
			Vector<uint32_t>		destination{ threadLocalAllocator };

			auto res = scene.Query(threadLocalAllocator, MaterialPassQuery{ RTPass }, BrushReq{}, GameObjectReq{});
			for (auto obj : res)
			{
				auto&& [materialView, brushView, gameObject] = obj;

				const auto brushID = brushView->brushID;

				if (auto res = sbt.objectMappings.find(brushID); res != nullptr)
				{
					// Check for stale information
				}
				else
				{
					// insert into binding table
					const uint32_t idx = (sbt.freeList.size()) ? sbt.freeList.pop_back() : sbt.GetIdx();
					destination.push_back(idx);

					auto meshHandle = brushView->meshes[0];
					auto meshResource = GetMeshResource(meshHandle);

					auto& lod = meshResource->GetLowestLoadedLod();
					DevicePointer indexBuffer		= lod.bufferSet->GetBufferPointer(VERTEXBUFFER_TYPE::INDEX);
					DevicePointer positionBuffer	= lod.bufferSet->GetBufferPointer(VERTEXBUFFER_TYPE::POSITION);
					DevicePointer normalBuffer		= lod.bufferSet->GetBufferPointer(VERTEXBUFFER_TYPE::NORMAL);

					HitGroupData data;
					memset(&data, 0, sizeof(HitGroupData));

					auto property = materialView.GetProperty<float>(LightIrradiance);
					if (property)
						memcpy(data.programID, (void*)lightMaterial.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					else
				        memcpy(data.programID, (void*)defaultMaterial.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

					float4 colors[] = {
						float4(1, 0, 0, 0),
						float4(0, 1, 0, 0),
						float4(0, 0, 1, 0),
						float4(1, 1, 0, 0),
					};

					if (property)
						data.color = float4(1, 1, 1, 1);
					else
					{
						data.color		= colors[idx % 4];
						data.index		= indexBuffer;
						data.normal		= normalBuffer;
						data.position	= positionBuffer;
					}

					hitGroupData.push_back(data);

					sbt.objectMappings.insert(brushID, HitGroupAllocation{ .idx = idx });
				}
			}

			if (hitGroupData.empty())
				return;

			auto hitGroupsUpload = ctx.ReserveDirectUploadSpace(hitGroupData.ByteSize(), 64);
			const uint32_t stride = sizeof(HitGroupData);

			memcpy(
				hitGroupsUpload.buffer,
				hitGroupData.data(), hitGroupData.ByteSize());

			for (auto [idx, src, destination] : zip(iota(0), hitGroupData, destination))
			{
				auto size = sizeof(HitGroupData);
	            ctx.CopyBufferRegion(
					hitTable.resource,
					hitGroupsUpload.resource,
					size,
					hitTable.offset + stride * destination,
					hitGroupsUpload.offset + stride * destination);
			}

			auto& dxCtx = static_cast<dx_Internal::dxDirectContext&>(ctx);
			
			static const auto buildHLAS = [&]() -> bool
				{
					Vector<D3D12_RAYTRACING_INSTANCE_DESC> instances(threadLocalAllocator);
					for (auto obj : enumerate(res))
					{
						auto&& [idx, data] = obj;
						auto&& [materialView, brushView, gameObject] = data;

						auto mesh = brushView->meshes.front();
						auto blas = GetMeshResource(mesh)->GetHighestLoadedLod().blAS;

						uint32_t flag = 0;
						auto property = materialView.GetProperty<float>(LightIrradiance);
						if (property)
							flag = 0x02;
						else 
						    flag = 0x01;

						D3D12_RAYTRACING_INSTANCE_DESC instance{
							.Transform = {
								{1, 0, 0, 0},
								{0, 1, 0, 0},
								{0, 0, 1, 0} },
							.InstanceID		= (UINT)idx,
							.InstanceMask	= flag,
							.InstanceContributionToHitGroupIndex = (uint32_t)idx,
							.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE,
							.AccelerationStructure = resources.GetDevicePointer(blas),
						};

						instances.push_back(instance);
					};

					auto upload = dxCtx.ReserveDirectUploadSpace(instances.ByteSize());
					memcpy(upload.buffer, instances.data(), instances.ByteSize());

					dxCtx.CopyBufferRegion(persistent.resource, upload.resource, instances.ByteSize(), sceneInstances.offset, upload.offset);

					return true;
				}();
			
		});

	return sbtUpdate;
}


TracePassData& RTExperimentState::PathTracePass(FrameGraph& frameGraph, GatherPassesTask& passes, CameraUpdateTask& cameraUpdate, UpdateSBTData& sbtUpdate, ResourceHandle renderTarget)
{
	struct UpdateTLAS
	{
		FrameGraphNodeHandle	nodeHandle;
		FrameResourceHandle		tlas;
		FrameResourceHandle		scratchPad;
	};

	UpdateTLAS& updateTLAS = frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder) -> UpdateTLAS
		{
			builder.AddNodeDependency(sbtUpdate.node),
			builder.AddDataDependency(cameraUpdate);

			return {
				.nodeHandle = builder.GetNodeHandle(),
			    .tlas = builder.AcquireVirtualResource(GPUResourceDesc::RayTracingStructure(64 * KILOBYTE), DeviceAccessState::DASACCELERATIONSTRUCTURE_WRITE, VirtualResourceScope::Frame),
				.scratchPad = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(64 * KILOBYTE), DeviceAccessState::DASUAV, VirtualResourceScope::Frame),
			};
		},
		[=, this, &passes](const UpdateTLAS& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{
						.DestAccelerationStructureData = resources.GetDevicePointer(data.tlas),
						.Inputs{
							.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE::D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
							.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS::D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
							.NumDescs = (UINT)passes.GetData().GetPass(RTPass).size(),
							.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
							.InstanceDescs = sceneInstances
						},
						.ScratchAccelerationStructureData = resources.GetDevicePointer(data.scratchPad),
			};

			auto& dxCtx = static_cast<dx_Internal::dxDirectContext&>(ctx);
			dxCtx.FlushBarriers();
			dxCtx.DeviceContext->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
		});

	auto& tracePass = frameGraph.AddNode2(
		[&](FrameGraphNodeBuilder& builder) -> TracePassData
		{
			builder.Requires(GetTypeGUID(Trangle));
			builder.AddDataDependency(cameraUpdate);
			builder.AddNodeDependency(updateTLAS.nodeHandle);

			return TracePassData{
				.sbtBuffer = builder.ReadTransition(
								sbtUpdate.sbtBuffer, DeviceAccessState::DASNonPixelShaderResource,
								{ DeviceSyncPoint::Sync_Copy, DeviceSyncPoint::Sync_Raytracing }),
				.tlas = builder.ReadTransition(
					            updateTLAS.tlas, DeviceAccessState::DASACCELERATIONSTRUCTURE_READ,
					{ DeviceSyncPoint::Sync_BuildRaytracingAccelerationStructure, DeviceSyncPoint::Sync_Raytracing }),
				.traceBuffer = builder.AcquireVirtualResource(
								GPUResourceDesc::UAVTexture(
								{ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT, false),	DeviceAccessState::DASUAV),

				.renderTarget = builder.CopyDest(renderTarget),

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
			auto* set0Layout = &globalInterface->GetDescriptorSetLayout(0);
			DescriptorSet set(ctx, *set0Layout, threadLocalAllocator);
			set.SetUAVTexture(ctx, 0, resources.GetResource(data.traceBuffer));
			set.SetSRV(ctx, 1, resources.GetResource(data.tlas));
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
					hitTable.devicePtr, 128 * sbt.count, 128,
			};
			rayDesc.MissShaderTable =
				D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
					missTable.devicePtr, 64, 64,
			};

			rayDesc.Width	= 800;
			rayDesc.Height	= 600;
			rayDesc.Depth	= 1;

			auto constants = GetCameraConstants(activeCamera);
			float3 pos = constants.WPOS;

			ctx.DiscardResource(resources.GetResource(data.traceBuffer));
			ctx.ClearUAVTextureFloat(resources.GetResource(data.traceBuffer));
			ctx.AddUAVBarrier(
				resources.GetResource(data.traceBuffer),
				-1, DeviceLayout::UnorderedAccess,
				DeviceSyncPoint::Sync_ClearUAV, DeviceSyncPoint::Sync_Raytracing);

			auto& dxCtx = static_cast<dx_Internal::dxDirectContext&>(ctx);

			struct 
			{
				float4x4_GPU	PVI;
				float			cameraPOS[3];
				uint32_t		seed;
			} viewportPoints
		    {
				.PVI		= constants.PVI,
				.cameraPOS	= { pos.x, pos.y, pos.z },
				.seed		= (uint32_t)rand()
		    };

			dxCtx.FlushBarriers();
			dxCtx.SetComputeRootSignature(globalInterface);
			dxCtx.SetComputeConstantValue(0, 20, &viewportPoints);
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
	core.RenderSystem->ResetConstantBuffer(cBuffer);
}
