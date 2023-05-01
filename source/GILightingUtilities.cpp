#include "GILightingUtilities.h"
#include "SVOGI.h"
#include <range/v3/all.hpp>

namespace FlexKit
{   /************************************************************************************************/


	class Null_Technqiue : public GITechniqueInterface
	{
	public:
		Null_Technqiue(RenderSystem& renderSystem, iAllocator& allocator) {}


		void Init(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB) {}


		BuildSceneRes BuildScene(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			ReserveConstantBufferFunction	reserveCB,
			iAllocator&						allocator) { return {nullptr}; }


		void RayTrace(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			const CameraHandle				camera,
			GatherPassesTask&				passes,
			BuildSceneRes					res,
			ResourceHandle					depthTarget,
			FrameResourceHandle				renderTarget,
			GBuffer&						gbuffer,
			ReserveConstantBufferFunction	reserveCB,
			iAllocator*						allocator) {}
	};


	/************************************************************************************************/


	struct ShaderTableHandle
	{
		DevicePointer   devicePointer;
		ResourceHandle  resource;
		uint32_t        byteSize;
		uint32_t        stride;

		operator DeviceAddressRangeStride()
		{
			return { devicePointer, byteSize, stride };
		}
	};


	struct RayGeneratorHandle
	{
		DevicePointer   devicePointer;
		ResourceHandle  resource;

		operator DeviceAddressRange()
		{
			return { devicePointer, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
		}
	};


	/************************************************************************************************/


	template<size_t TableSize = 4096>
	struct FrameAllocator
	{
		struct Identifier
		{
			uint8_t buffer[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];

			operator void* () { return buffer; }
		};

		FrameAllocator(RenderSystem& IN_renderSystem, iAllocator& IN_allocator) :
			allocator		{ IN_allocator },
			renderSystem	{ IN_renderSystem },
			frameBegin		{ buffer.begin() }
		{
			resource	= renderSystem.CreateConstantBuffer(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * TableSize, false);
			gpuBuffer	= { renderSystem.GetDeviceResource(resource)->GetGPUVirtualAddress() };
		}

		DevicePointer Push(void* identifier_ptr)
		{
			Identifier identifier;
			memcpy(&identifier, identifier_ptr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES );

			auto idx = buffer.emplace_back(identifier);
			tableSize++;

			return { gpuBuffer.address + idx * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
		}

		DevicePointer Begin()
		{
			if (buffer.size() % 2 != 0)
				buffer.emplace_back();

			tableSize = 0;

			return { gpuBuffer + (buffer.size() - 1) * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
		}

		DeviceAddressRangeStride End()
		{
			return { gpuBuffer.address + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * (buffer.size() - tableSize), tableSize, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES};
		}

		template<typename TY>
		void Move(TY destination)
		{
			FK_ASSERT(sizeof(TY) >= buffer.size(), "Destination too small!");
		}

		void BeginFrame()
		{
			frameBegin = buffer.begin();
		}

		DevicePointer EndFrame()
		{
			return {};
		}

		size_t Size() { return buffer.size() * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; }

		DevicePointer			gpuBuffer	= { 0 };
		ConstantBufferHandle	resource	= InvalidHandle;
		uint32_t				tableSize	= 0;

		iAllocator&				allocator;
		RenderSystem&			renderSystem;

		using IdentifierBuffer = CircularBuffer<Identifier, TableSize>;

		struct FrameRange
		{
			uint32_t begin;
			uint32_t end;
		};

		CircularBuffer<FrameRange, 3>				frameRanges;
		typename IdentifierBuffer::CircularIterator	frameBegin;
		IdentifierBuffer							buffer;
	};


	struct Allocation
	{
		void* begin;
		void* end;
	};

	struct PersistentAllocator
	{
		PersistentAllocator(uint32_t blockSize, RenderSystem& IN_renderSystem, iAllocator& IN_allocator) :
			allocations	{ &IN_allocator },
			freeList	{ &IN_allocator }{}

		struct AllocatedRange
		{
			size_t		frameID;
			uint32_t	begin;
			uint32_t	end;

			uint32_t size() const { return end - begin; }

			bool operator < (const AllocatedRange& rhs) noexcept
			{
				return size() < rhs.size();
			}
		};

		std::optional<Allocation> Alloc(uint32_t allocationSize, size_t frameID)
		{
			for (int I = 0; I < freeList.size(); I++)
			{
				static const int BlockSize = 64;

				if (I % BlockSize == 0)
				{
					std::partial_sort(
						freeList.begin() + I + 0,
						freeList.begin() + I + BlockSize,
						freeList.end(),
						std::less{});
				}

				auto& element = freeList[I];

				if (element.size() >= allocationSize && element.frameID < frameID - 3)
				{
					Allocation alloc{
						.begin  = buffer + element.begin,
						.end    = buffer + element.begin + allocationSize,
					};

					if (element.size() > allocationSize)
						element.begin = element.begin + allocationSize;
					else
						freeList.remove_unstable(freeList.begin() + I);

					return alloc;
				}
			}

			return {};
		}

		void Free(Allocation allocation)
		{
			auto res = std::find_if(
				allocations.begin(), allocations.end(),
				[](auto& e)
				{
					return false;
				});

			if (res != allocations.end())
			{
				allocations.remove_unstable(res);
				freeList.emplace_back(*res);
			}

		}

		ID3D12DescriptorHeap*	heap;
		char*					buffer;
		Vector<AllocatedRange>	allocations;
		Vector<AllocatedRange>	freeList;
	};


	/************************************************************************************************/


	constexpr PSOHandle InlineTest{ GetTypeGUID(InlineTest) };

	class NOOBs_First_RTX_Technqiue : public GITechniqueInterface
	{
	public:
		NOOBs_First_RTX_Technqiue(RenderSystem& renderSystem, iAllocator& IN_allocator) :
			globalRootSig	{ &IN_allocator },
			persistent		{ 4 * MEGABYTE, renderSystem, IN_allocator },
			temporary		{ renderSystem, IN_allocator },
			rayGenTable		{ renderSystem.CreateUAVBufferResource(4096, false) },
			missShaderTable	{ renderSystem.CreateUAVBufferResource(4096, false) },
			hitShaderTable	{ renderSystem.CreateUAVBufferResource(4096, false) },
			ASpool			{ renderSystem,   512 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::UAVBuffer, IN_allocator },
			allocator		{ IN_allocator }
		{
			renderSystem.SetDebugName(rayGenTable,		"rayGenTable");
			renderSystem.SetDebugName(missShaderTable,	"missShaderTable");
			renderSystem.SetDebugName(hitShaderTable,	"hitShaderTable");
			renderSystem.SetDebugName(ASpool.heap,		"Accelleration Structure Pool");

			renderSystem.RegisterPSOLoader(InlineTest,
				{	&globalRootSig,
					[&](RenderSystem* renderSystem) { return CreateInlineTest(*renderSystem); }
				});

			CreatePipelineLibrary(renderSystem);
		}


		~NOOBs_First_RTX_Technqiue()
		{
			stateObject->Release();
		}

		void CreatePipelineLibrary(RenderSystem& renderSystem)
		{
			const char awesomeRaytracingCode[] = R"(assets\shaders\RTX\MyFirstRTX.hlsl)";

			D3D12_SHADER_BYTECODE shaderByteCode = renderSystem.LoadShader(nullptr, "lib_6_5", awesomeRaytracingCode);

			/*
			LPCWSTR Name;
			LPCWSTR ExportToRename;
			D3D12_EXPORT_FLAGS Flags;
			*/

			D3D12_EXPORT_DESC exports[] =
			{
				{
					L"AnyHit",
					L"AnyHit",
					D3D12_EXPORT_FLAGS::D3D12_EXPORT_FLAG_NONE
				},
				{
					L"RayGenerator",
					L"RayGenerator",
					D3D12_EXPORT_FLAGS::D3D12_EXPORT_FLAG_NONE
				},
				{
					L"Miss",
					L"Miss",
					D3D12_EXPORT_FLAGS::D3D12_EXPORT_FLAG_NONE
				},
				{
					L"ClosestHit",
					L"ClosestHit",
					D3D12_EXPORT_FLAGS::D3D12_EXPORT_FLAG_NONE
				}
			};

			DesciptorHeapLayout UAV_layout{};
			UAV_layout.SetParameterAsShaderUAV(0, 1, 1);
			UAV_layout.SetParameterAsSRV(1, 1, 2);

			globalRootSig.AllowIA = true;
			globalRootSig.AllowSO = true;
			globalRootSig.SetParameterAsSRV(0, 0);
			globalRootSig.SetParameterAsCBV(1, 0, 0, PIPELINE_DEST_ALL);
			globalRootSig.SetParameterAsDescriptorTable(2, UAV_layout);
			globalRootSig.Build(&renderSystem, &allocator);

			D3D12_GLOBAL_ROOT_SIGNATURE signature =
			{
				.pGlobalRootSignature = globalRootSig
			};

			D3D12_DXIL_LIBRARY_DESC dxil_desc[1] =
			{
				{
					.DXILLibrary	= shaderByteCode,
					.NumExports		= 4,
					.pExports		= exports,
				}
			};

			D3D12_RAYTRACING_SHADER_CONFIG shader_Config[1] =
			{
				{
					.MaxPayloadSizeInBytes		= 16,
					.MaxAttributeSizeInBytes	= 16,
				}
			};

			D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig[] = {
				{
					.MaxTraceRecursionDepth = 4,
				}
			};

			D3D12_HIT_GROUP_DESC hitGroupDesc = {
				.HitGroupExport				= L"DefaultHitGroup",
				.Type						= D3D12_HIT_GROUP_TYPE_TRIANGLES,
				.AnyHitShaderImport			= L"AnyHit",
				.ClosestHitShaderImport		= L"ClosestHit",
				.IntersectionShaderImport	= nullptr
			};

			D3D12_STATE_SUBOBJECT subObjects[] =
			{
				{
					.Type	= D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
					.pDesc	= &signature,
				},
				{
					.Type	= D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
					.pDesc	= dxil_desc,
				},
				{
					.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
					.pDesc	= shader_Config
				},
				{
					.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
					.pDesc	= pipelineConfig
				},
				{
					.Type	= D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
					.pDesc	= &hitGroupDesc
				}
			};

			D3D12_STATE_OBJECT_DESC descs[] = {
				{
					.Type			= D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
					.NumSubobjects	= sizeof(subObjects) / sizeof(subObjects[0]),
					.pSubobjects	= subObjects,
				}
			};

			if (FAILED(renderSystem.pDevice10->CreateStateObject(descs, IID_PPV_ARGS(&stateObject))))
				FK_LOG_ERROR("Failed to create State Object");
		}


		void Init(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB)
		{
			struct Resources
			{
				FrameResourceHandle	rayGenTable;
				FrameResourceHandle	hitShaderTable;
				FrameResourceHandle	missShaderTable;
			};

			auto& resources = frameGraph.AddNode<Resources>(
				{},
				[&](FrameGraphNodeBuilder& builder, Resources& data)
				{
					data.rayGenTable		= builder.CopyDest(rayGenTable);
					data.hitShaderTable		= builder.CopyDest(hitShaderTable);
					data.missShaderTable	= builder.CopyDest(missShaderTable);
				},
				[&](Resources& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
				{
					ID3D12StateObjectProperties* stateObjectProperties;
					if (FAILED(stateObject->QueryInterface(&stateObjectProperties)))
						FK_LOG_ERROR("Failed to query ID3D12StateObjectProperties");

					const auto rayGenIdentifier		= stateObjectProperties->GetShaderIdentifier(L"RayGenerator");
					const auto missIdentifier		= stateObjectProperties->GetShaderIdentifier(L"Miss");
					const auto hitTableIdentifier	= stateObjectProperties->GetShaderIdentifier(L"DefaultHitGroup");

					struct ShaderTableLayout
					{
						uint8_t		rayGen[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
						uint32_t	beef = 0xbeefbeef;
					} rayGen;

					struct hitTableLayout
					{
						uint8_t		anyhit_main[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
						uint32_t	beef = 0xbeefbeef;

					} hitTable;

					struct missTableLayout
					{
						uint8_t		miss[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
						uint32_t	beef = 0xbeefbeef;
					} missTable;

					memcpy(&rayGen.rayGen,			rayGenIdentifier,	D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(&hitTable.anyhit_main,	hitTableIdentifier,	D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
					memcpy(&missTable.miss,			missIdentifier,		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

					auto rayGenUpload		= ctx.ReserveDirectUploadSpace(sizeof(rayGen));
					auto hitTableUpload		= ctx.ReserveDirectUploadSpace(sizeof(hitTable));
					auto missTableUpload	= ctx.ReserveDirectUploadSpace(sizeof(missTable));

					memcpy(rayGenUpload.buffer, &rayGen, sizeof(rayGen));
					memcpy(hitTableUpload.buffer, &hitTable, sizeof(hitTable));
					memcpy(missTableUpload.buffer, &missTable, sizeof(missTable));

					ctx.CopyBuffer(rayGenUpload, rayGenTable);
					ctx.CopyBuffer(hitTableUpload, hitShaderTable);
					ctx.CopyBuffer(missTableUpload, missShaderTable);
				});
		}


		BuildSceneRes BuildScene(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			ReserveConstantBufferFunction	reserveCB,
			iAllocator&						allocator)
		{
			PassDrivenResourceAllocation allocation
			{
				.getPass =
					[&]() -> std::span<const PVEntry>
					{
						return passes.GetData().GetPass(PassHandle{ GBufferPassID });
					},
				.initializeResources =
					[&](std::span<const PVEntry> objects, std::span<FrameResourceHandle> frameHandles, auto& resourceCtx, iAllocator& allocator)
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
							auto	meshResource	= GetMeshResource(meshHandle);
							auto&	lod				= meshResource->GetLowestLoadedLod();
							
							resourceCtx.BuildBLAS(frameHandle, lod);
						}
					},
				.layout		= DeviceLayout::DeviceLayout_Unknown,
				.access		= DeviceAccessState::DASACCELERATIONSTRUCTURE_READ,
				.max		= 256,
				.pool		= &ASpool,
				.dependency	= &passes,
			};

			const auto& allocationRes = frameGraph.AllocateResourceSet(allocation);

			return { (void*)&allocationRes };
		}


		void RayTrace(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			const CameraHandle				camera,
			GatherPassesTask&				passes,
			BuildSceneRes					bvh,
			ResourceHandle					depthTarget,
			FrameResourceHandle				renderTarget,
			GBuffer&						gbuffer,
			ReserveConstantBufferFunction	reserveCB,
			iAllocator*						allocator)
		    {
			struct raytrace_data
			{
				ReserveConstantBufferFunction	reserveCB;
				FrameResourceHandle				tlAS;
				FrameResourceHandle				scratchPad;
				FrameResourceHandle				temporary;
				FrameResourceHandle				target2D;

				FrameResourceHandle				depthBuffer;
				FrameResourceHandle				albedo;

				CameraHandle					camera;
			};
			frameGraph.AddNode<raytrace_data>(
				raytrace_data{
					.reserveCB	= reserveCB,
					.camera		= camera
				},
				[&](FrameGraphNodeBuilder& builder, raytrace_data& data)
				{
					builder.NonPixelShaderResource(hitShaderTable);
					builder.NonPixelShaderResource(missShaderTable);
					builder.NonPixelShaderResource(rayGenTable);

					data.tlAS		= builder.AcquireVirtualResource(GPUResourceDesc::RayTracingStructure(1 * MEGABYTE), DASACCELERATIONSTRUCTURE_WRITE, &ASpool);
					data.scratchPad	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1 * MEGABYTE), DASUAV);
					data.temporary	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1 * MEGABYTE), DASCopyDest);
					data.target2D	= builder.WriteTransition(renderTarget, DASUAV, { FlexKit::Sync_All, FlexKit::Sync_Raytracing });

					data.depthBuffer	= builder.NonPixelShaderResource(depthTarget);
					data.albedo			= builder.NonPixelShaderResource(gbuffer.albedo);

					builder.AddNodeDependency(static_cast<const ResourceAllocation*>(bvh._ptr)->node);
				},
				[&](raytrace_data& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
				{
					ctx.BeginEvent_DEBUG("RT Experiment");
					ctx.BeginEvent_DEBUG("Update Top Level Accelleration Struction");

					SETDEBUGNAME(resources.GetDeviceResource(data.tlAS), "TLAS");
					SETDEBUGNAME(resources.GetDeviceResource(data.scratchPad), "scraptch");

					auto UpdateAS = [&]()
					{
						Vector<D3D12_RAYTRACING_INSTANCE_DESC> sceneElements{ &allocator };

						for (auto& pvs : passes.GetData().GetPass(PassHandle{ GBufferPassID }))
						{
							for(auto mesh : pvs.brush->meshes)
							{
								auto& lod = GetMeshResource(mesh)->GetLowestLoadedLod();

								if (lod.blAS == -1)
									continue;

								D3D12_RAYTRACING_INSTANCE_DESC instance;
								instance.AccelerationStructure					= lod.blAS;
								instance.Flags									= D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
								instance.InstanceContributionToHitGroupIndex	= 0;
								instance.InstanceID								= pvs.submissionID;
								instance.InstanceMask							= 0x01;

								const auto WT = GetWT(pvs.brush->Node);

								for (size_t row = 0; row < 3; row++)
									for (size_t col = 0; col < 4; col++)
										instance.Transform[row][col] = WT[row][col];

								sceneElements.emplace_back(instance);
							}
						}

						const uint32_t arraySize = (uint32_t)(FlexKit::AlignedSize(sceneElements.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)));

						UploadReservation upload = ctx.ReserveDirectUploadSpace(arraySize);
						memcpy(upload.buffer, sceneElements.data(), sceneElements.ByteSize());

						ctx.CopyBuffer(upload, resources.CopyDest(data.temporary, ctx));
						SETDEBUGNAME(upload.resource, "copy buffer");


						D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildAS = {
							.DestAccelerationStructureData   = resources.GetDevicePointer(resources.GetResource(data.tlAS)),
							.Inputs = {
								.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
								.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
								.NumDescs		= static_cast<UINT>(sceneElements.size()),
								.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY,
								.InstanceDescs	= resources.GetDevicePointer(resources.NonPixelShaderResource(data.temporary, ctx))
							},
							.ScratchAccelerationStructureData = resources.GetDevicePointer(resources.UAV(data.scratchPad, ctx)),
						};

						ctx.FlushBarriers();
						ctx.DeviceContext->BuildRaytracingAccelerationStructure(&buildAS, 0, nullptr);

						ctx.AddBufferBarrier(resources.GetResource(data.tlAS), DASACCELERATIONSTRUCTURE_WRITE, DASACCELERATIONSTRUCTURE_READ, Sync_BuildRaytracingAccellerationStructure, Sync_Raytracing);
					};

					UpdateAS();

					ctx.EndEvent_DEBUG();
					ctx.BeginEvent_DEBUG("Cast Rays");


					const DispatchDesc desc = {
						.hitGroupTable =
						{
							.rangeStride = {
								.StartAddress	= resources.GetDevicePointer(hitShaderTable),
								.SizeInBytes	= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
								.StrideInBytes	= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
							}
						},
						.missTable =
						{
							.rangeStride = {
								.StartAddress	= resources.GetDevicePointer(missShaderTable),
								.SizeInBytes	= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
								.StrideInBytes	= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
							}
						},
						.rayGenerationRecord =
						{
							.range = {
								.StartAddress	= resources.GetDevicePointer(rayGenTable),
								.SizeInBytes	= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
							}
						},
					};


					const uint32_t bufferSize = (uint32_t)FlexKit::AlignedSize<Camera::ConstantBuffer>();
					auto constantBuffer = data.reserveCB(bufferSize);
					ConstantBufferDataSet cameraConstants{ GetCameraConstants(data.camera), constantBuffer };

					DescriptorHeap heap{};
					heap.Init(ctx, globalRootSig.GetDescHeap(0), &allocator);
					heap.SetUAVTexture(ctx, 0, resources.UAV(data.target2D, ctx));
					heap.SetSRV(ctx, 1, resources.NonPixelShaderResource(data.depthBuffer, ctx), DeviceFormat::R32_FLOAT);
					heap.SetSRV(ctx, 2, resources.NonPixelShaderResource(data.albedo, ctx));

					ctx.SetComputeRootSignature(globalRootSig);
					ctx.SetComputeShaderResourceView(0, resources.GetResource(data.tlAS), 0);
					ctx.SetComputeConstantBufferView(1, cameraConstants);
					ctx.SetComputeDescriptorTable(2, heap);

					//ctx.Dispatch(resources.GetPipelineState(InlineTest), { 1920 / 32 + 1, 1080 / 32 + 1, 1 });
					ctx.DeviceContext->SetPipelineState1(stateObject);
					ctx.DispatchRays({ 1920, 1080, 1 }, desc);

					ctx.EndEvent_DEBUG();
					ctx.EndEvent_DEBUG();
				});
		}


		ID3D12PipelineState* CreateInlineTest(RenderSystem& renderSystem)
		{
			const char file[] = R"(assets\shaders\RTX\InlineTracingTest.hlsl)";

			D3D12_SHADER_BYTECODE shaderByteCode = renderSystem.LoadShader("main", "cs_6_5", file);

			D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
				globalRootSig,
				shaderByteCode
			};

			ID3D12PipelineState* PSO = nullptr;
			auto HR = renderSystem.pDevice10->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

			FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

			return PSO;
		}


		DeviceAddressRangeStride	hitShader;
		DeviceAddressRangeStride	missShader;
		DevicePointer				rayGenShader;

		ResourceHandle		hitShaderTable;
		ResourceHandle		missShaderTable;
		ResourceHandle		rayGenTable;

		ID3D12StateObject*		stateObject = nullptr;
		RootSignature			globalRootSig;

		MemoryPoolAllocator	ASpool;

		iAllocator&			allocator;

		PersistentAllocator	persistent;
		FrameAllocator<>	temporary;
	};


	/************************************************************************************************/


	class VXGI_Technique : public GITechniqueInterface
	{
	public:
		VXGI_Technique(RenderSystem& renderSystem, iAllocator& allocator) :
			lightingEngine{ renderSystem, allocator } {}


		void Init(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB)
		{
			lightingEngine.InitializeOctree(frameGraph);
		}

		BuildSceneRes BuildScene(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			ReserveConstantBufferFunction	reserveCB,
			iAllocator&						allocator)
		{
			lightingEngine.VoxelizeScene(
				frameGraph,
				scene,
				reserveCB,
				passes);

			return { nullptr };
		}

		void RayTrace(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			const CameraHandle				camera,
			GatherPassesTask&				passes,
			BuildSceneRes					voxelize,
			ResourceHandle					depthTarget,
			FrameResourceHandle				renderTarget,
			GBuffer&						gbuffer,
			ReserveConstantBufferFunction	reserveCB,
			iAllocator*						allocator)
		{
			lightingEngine.RayTrace(
				dispatcher,
				frameGraph,
				camera,
				depthTarget,
				renderTarget,
				gbuffer,
				reserveCB,
				allocator);
		}

		GILightingEngine lightingEngine;
	};


	/************************************************************************************************/


	GlobalIlluminationEngine::GlobalIlluminationEngine(RenderSystem& renderSystem, iAllocator& IN_allocator, EGITECHNIQUE techniqueEnum) :
		allocator{ IN_allocator }
	{
		if (techniqueEnum == EGITECHNIQUE::AUTOMATIC)
		{
			switch (renderSystem.features.RT_Level)
			{
			case AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE:
				techniqueEnum = EGITECHNIQUE::VXGL;
				break;
			case AvailableFeatures::Raytracing::RT_FeatureLevel_1:
			case AvailableFeatures::Raytracing::RT_FeatureLevel_1_1:
				techniqueEnum = EGITECHNIQUE::RTGI;
				break;
			}
		}

		switch (techniqueEnum)
		{
		case EGITECHNIQUE::RTGI:
			FK_LOG_INFO("RTGI Enabled");
			technique = &allocator.allocate<NOOBs_First_RTX_Technqiue>(renderSystem, allocator);
			break;
		case EGITECHNIQUE::VXGL:
			FK_LOG_INFO("VXGL Enabled");
			technique = &allocator.allocate<VXGI_Technique>(renderSystem, allocator);
			break;
		case EGITECHNIQUE::DISABLE:
		default:
			FK_LOG_INFO("GI Disabled");
			technique = &allocator.allocate<Null_Technqiue>(renderSystem, allocator);
			break;
			// do nothing;
		}

	}


	/************************************************************************************************/


	GlobalIlluminationEngine::~GlobalIlluminationEngine()
	{
		if (technique)
			allocator.release(*technique);
	}


	/************************************************************************************************/


	void GlobalIlluminationEngine::Init(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB)
	{
		if(technique)
			technique->Init(frameGraph, reserveCB);
	}


	/************************************************************************************************/


	BuildSceneRes GlobalIlluminationEngine::BuildScene(
		FrameGraph&						frameGraph,
		Scene&							scene,
		GatherPassesTask&				passes,
		ReserveConstantBufferFunction	reserveCB,
		iAllocator&						allocator)
	{
		if (technique)
			return technique->BuildScene(
				frameGraph,
				scene,
				passes,
				reserveCB,
				allocator);
		else
			return { nullptr };
	}


	/************************************************************************************************/


	void GlobalIlluminationEngine::RayTrace(
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		const CameraHandle				camera,
		GatherPassesTask&				passes,
		BuildSceneRes					bvh,
		ResourceHandle					depthTarget,
		FrameResourceHandle				renderTarget,
		GBuffer&						gbuffer,
		ReserveConstantBufferFunction	reserveCB,
		iAllocator*						allocator)
	{
		if (technique)
			technique->RayTrace(
				dispatcher,
				frameGraph,
				camera,
				passes,
				bvh,
				depthTarget,
				renderTarget,
				gbuffer,
				reserveCB,
				allocator);
	}


}   /************************************************************************************************/

