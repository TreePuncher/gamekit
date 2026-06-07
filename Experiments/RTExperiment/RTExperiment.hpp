#include <Application.hpp>

#include <RenderSystemInterface.hpp>
#include <FrameGraph.hpp>
#include <filesystem>
#include <ModifiableShape.hpp>
#include <MeshUtilities.hpp>
#include <PersistentGPUAllocator.hpp>
#include <TriMeshResource.hpp>
#include <Type.hpp>

#include <Win32Graphics.hpp>
#include <dxContext.hpp>
#include <dxRenderSystem.hpp>
#include "ShaderBindingTable.hpp"

using namespace FlexKit;

TriMeshHandle LoadObj(std::filesystem::path p)
{
	using namespace FlexKit;

	Vector<char> buffer{ SystemAllocator };
	buffer.resize(std::filesystem::file_size(p));

	bool Loaded = LoadFileIntoBuffer(p.string().c_str(), (std::byte*)buffer.data(), buffer.size());// TODO: Make Thread Safe
	if (!Loaded)
	{
		printf("Failed To Load Obj\n");
		return InvalidHandle;
	}

	MeshUtilityFunctions::OBJ_Tools::LoaderState	S;
	MeshUtilityFunctions::TokenList					TL{ SystemAllocator };

	char	current_line[512];
	uint32_t pos = 0;
	uint32_t line_pos = 0;
	auto size = buffer.size();
	while (pos < size)
	{
		if (buffer[pos] != '\n')
		{
			current_line[line_pos++] = buffer[pos];
		}
		else
		{
			size_t LineLength = line_pos;
			current_line[LineLength] = '\0';
			CStrToToken(MeshUtilityFunctions::ScrubLine(current_line, LineLength), LineLength, TL, S);
			line_pos = 0;
		}
		pos++;
	}

	struct Point {
		float xyz[3];
	};

	Vector<uint32_t>	indexes	{ SystemAllocator };
	Vector<float3>		points	{ SystemAllocator };
	Vector<float3>		normals	{ SystemAllocator };
	Vector<float2>		uvs		{ SystemAllocator };

	auto meshHandle = CreateMesh(123456789);
	auto& mesh = *GetMeshResource(meshHandle);
	TriMesh::LOD_Runtime lod0;
	lod0.bufferSet = &IRenderSystem::GetInstance().CreateVertexBufferSet();

	AABB aabb;

	for (const MeshToken& token : TL)
	{
		std::visit(
			Overloaded{
			[&](const PointToken& point)
			{
				Point p;
				p.xyz[0] = point.xyz.x;
				p.xyz[1] = point.xyz.y;
				p.xyz[2] = point.xyz.z;
				points.push_back(point.xyz);

				aabb += point.xyz;
			},
			[](auto&&) {} },
			token);
	}

	auto kdbTree = MeshUtilityFunctions::MeshKDBTree{ TL };

	MeshUtilityFunctions::OptimizedMesh       optimized;
	MeshUtilityFunctions::LocalBlockContext   context{ kdbTree.mesh };

	for (const auto& leaf : kdbTree)
		for (auto I = leaf->begin; I < leaf->end; I++)
			optimized.PushTri(kdbTree.mesh.tris[I], context, true);

	MeshUtilityFunctions::OptimizedBuffer optimizedBuffer{ optimized };

	size_t VertexBufferSize = optimizedBuffer.points.ByteSize() + sizeof(VertexBufferView);// pos
	size_t IndexBufferSize = optimizedBuffer.indexes.ByteSize() + sizeof(VertexBufferView);// index

	lod0.views[0] = CreateVertexBufferView(SystemAllocator, VertexBufferSize);
	lod0.views[1] = CreateVertexBufferView(SystemAllocator, IndexBufferSize);


	lod0.views[0]->Begin(VERTEXBUFFER_TYPE::POSITION, VERTEXBUFFER_FORMAT::R32G32B32);
	lod0.views[1]->Begin(VERTEXBUFFER_TYPE::INDEX, VERTEXBUFFER_FORMAT::R32);


	memcpy(lod0.views[0]->GetBuffer(), optimizedBuffer.points.data(), optimizedBuffer.points.ByteSize());
	memcpy(lod0.views[1]->GetBuffer(), optimizedBuffer.indexes.data(), optimizedBuffer.indexes.ByteSize());

	lod0.views[0]->MarkFull();
	lod0.views[1]->MarkFull();

	lod0.views[0]->End();
	lod0.views[1]->End();

	lod0.bufferSet->CreateBuffer(VERTEXBUFFER_TYPE::POSITION, VERTEXBUFFER_FORMAT::R32G32B32, optimizedBuffer.points.ByteSize());
	lod0.bufferSet->CreateBuffer(VERTEXBUFFER_TYPE::INDEX, VERTEXBUFFER_FORMAT::R32, optimizedBuffer.indexes.ByteSize());

	auto copyCtx = IRenderSystem::GetInstance().GetImmediateCopyQueue();

	UploadVertexBuffer(copyCtx, lod0.bufferSet->At(0), *lod0.views[0]);
	UploadVertexBuffer(copyCtx, lod0.bufferSet->At(1), *lod0.views[1]);

	if (optimizedBuffer.normals.size())
	{
		size_t NormalBufferSize = optimizedBuffer.normals.ByteSize() + sizeof(VertexBufferView);// Normal
		lod0.views[2] = CreateVertexBufferView(SystemAllocator, NormalBufferSize);
		lod0.views[2]->Begin(VERTEXBUFFER_TYPE::NORMAL, VERTEXBUFFER_FORMAT::R32G32B32);
		memcpy(lod0.views[2]->GetBuffer(), optimizedBuffer.normals.data(), optimizedBuffer.normals.ByteSize());
		lod0.views[2]->MarkFull();
		lod0.views[2]->End();
		lod0.bufferSet->CreateBuffer(VERTEXBUFFER_TYPE::NORMAL, VERTEXBUFFER_FORMAT::R32G32B32, optimizedBuffer.normals.ByteSize());
		UploadVertexBuffer(copyCtx, lod0.bufferSet->At(2), *lod0.views[2]);
	}

	mesh.aabb = aabb;
	mesh.bs = float4{ aabb.MidPoint(), aabb.Span().magnitude() / 2.0f };

	SubMesh sm{
		.BaseIndex = 0,
		.IndexCount = (uint32_t)optimizedBuffer.IndexCount(),
		.materialIndex = 0,
		.aabb = aabb,
	};

	lod0.subMeshes.push_back(sm);
	mesh.lods.push_back(lod0);
	lod0.state = TriMesh::LOD_Runtime::LOD_State::Loaded;

	return meshHandle;
}

constexpr GUID_t VertexShaderAssetID	= GetCRCGUID(VertexShader);
constexpr GUID_t PixelShaderAssetID		= GetCRCGUID(PixelShader);

struct RTExperimentState : FrameworkState
{
	RTExperimentState(GameFramework& IN_framework) : 
        FrameworkState(IN_framework),
		persistent{ 64, framework.core.GetBlockMemory() },
		gpuAllocator{ 64 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::UAVTextures | DeviceHeapFlags::UAVBuffer, framework.core.GetBlockMemory() }
	{
		Win32RenderWindowDesc windowDesc = DefaultWindowDesc({ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT);

		DescriptorSetLayout layout{ framework.core.GetBlockMemory() };
		layout.SetParameterAsUAV(0, 0, 1, 0);
		layout.SetParameterAsSRV(1, 1, 1, 0);

		PipelineInterfaceBuilder builder{ framework.core.GetBlockMemory() };
		builder.SetParameterAsUINT(0, 16, 0, 0);
		builder.SetParameterAsDescriptorSet(1, layout);
		
		globalInterface = builder.Build(framework.core.GetTempMemory());

		renderWindow = CreateWin32RenderWindow(GetRenderSystem(), windowDesc);

		library			= LoadShaderLibrary("assets/shaders/rtLibrary.hlsl", globalInterface);
		raygenID		= library->FindShaderFunction("raygen_main");
		missID			= library->FindShaderFunction("miss_main");
		defaultGroup1	= library->FindShaderFunction("defaultHitGroup");

		GetRenderSystem().RegisterPSOLoader(GetTypeGUID(Trangle),
			[](IRenderSystem& renderSystem, iAllocator& allocator)
			{
				PipelineBuilder builder(renderSystem, allocator);
				builder.AddInputLayout({
					.inputs = {
						{
							.name			= "POSITION",
							.index			= 0,
							.format			= DeviceFormat::R16G16B16A16_FLOAT,
							.inputSlotClass = EInputClassification::PerVertex,
						}},
					.count = 1
					});
				//builder.AddVertexShader("VMain");
				//builder.AddPixelShader("PMain");
				builder.AddVertexShader("VMain", "assets/shaders/TestShader.hlsl", ShaderOptions{ .enableDebug = true });
				builder.AddPixelShader("PMain", "assets/shaders/TestShader.hlsl", ShaderOptions{ .enableDebug = true });
				builder.AddRasterizerState();
				builder.AddRenderTargetState({
						.targetCount = 1,
						.targetFormats = { DeviceFormat::R16G16B16A16_FLOAT },
					});

				return builder.Build(renderSystem, allocator);
			});

		GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

		vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
		cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);

		shape = LoadObj("Test.obj");

		SBTMemory		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
		hitTable		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
		missTable		= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
		rayGenerator	= persistent.AllocBlocks(2, GetRenderSystem().GetCurrentCounter()).value();
	}

	~RTExperimentState()
	{
		GetRenderSystem().ReleaseVB(vBuffer);
		GetRenderSystem().ReleaseCB(cBuffer);
	}

	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT)
	{
		t += dT;

		Win32UpdateInput();
		return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
	{
		struct
		{
			float xyz[3];
		} triangle[3] = {
			{.xyz = { -1.0f, -1.0f,  0.0f }},
			{.xyz = {  0.0f,  1.0f,  0.0f }},
			{.xyz = {  1.0f, -1.0f,  0.0f }},
		};

		GetRenderSystem().VertexBufferPush(vBuffer, triangle, sizeof(triangle));

		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);
		frameGraph.AddOutput(persistent.resource);
		frameGraph.AddMemoryPool(gpuAllocator);

		ClearBackBuffer(frameGraph, renderTarget, { sinf(t) * 0.5f + 0.5f, cosf(t * 5.0f) * 0.5f + 0.5f, tanf(t * 10.0f) * 0.5f + 0.5f, 1 });

		struct DrawTrangle
		{
			FrameResourceHandle renderTarget;
		};

		struct UpdateSBT
		{
			FrameResourceHandle sbtBuffer;
		};

		struct TracePass
		{
			FrameResourceHandle sbtBuffer;
			FrameResourceHandle traceBuffer;
			FrameResourceHandle renderTarget;
		};

		frameGraph.AddConstantBuffer(cBuffer);

		auto sbtUpdate = frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& builder) -> UpdateSBT
			{
				builder.Requires(GetTypeGUID(Trangle));

				return UpdateSBT{
					.sbtBuffer = builder.CopyDest(persistent.resource)
				};
			},
			[=, this](const UpdateSBT& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
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

						auto rayGeneratorUpload		= ctx.ReserveDirectUploadSpace(64, sizeof(rayGeneratorRecord));
						auto missShaderUpload		= ctx.ReserveDirectUploadSpace(64, sizeof(missGeneratorRecord));
						auto hitGroupsUpload		= ctx.ReserveDirectUploadSpace(64, sizeof(hitGroups));

						memcpy(rayGeneratorRecord.programID, (void*)raygenID.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
						memcpy(missGeneratorRecord.programID, (void*)missID.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
						memcpy(hitGroups[0].programID, (void*)defaultGroup1.id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
						memcpy(rayGeneratorUpload.buffer, &rayGeneratorRecord, sizeof(rayGeneratorRecord));
						memcpy(missShaderUpload.buffer, &missGeneratorRecord, sizeof(missGeneratorRecord));
						memcpy(hitGroupsUpload.buffer, &hitGroups, sizeof(hitGroups));

						ctx.CopyBuffer(rayGeneratorUpload, rayGenerator.resource, rayGenerator.offset);
						ctx.CopyBuffer(missShaderUpload, missTable.resource, missTable.offset);
						ctx.CopyBuffer(hitGroupsUpload, hitTable.resource, hitTable.offset);
						return true;
					}();
			});

		auto drawPass = frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& builder) -> DrawTrangle
			{
				builder.Requires(GetTypeGUID(Trangle));

				return DrawTrangle{
					.renderTarget		= builder.RenderTarget(renderTarget), 
				};
			},
			[=, this](const DrawTrangle& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
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

				auto mesh = GetMeshResource(shape);
				auto& lod = mesh->lods[0];

				ctx.SetInputPrimitive(EInputPrimitive::INPUTPRIMITIVETRIANGLELIST);
				ctx.AddVertexBuffers(mesh, 0, { VERTEXBUFFER_TYPE::POSITION });
				ctx.AddIndexBuffer(mesh, 0);

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				ctx.SetGraphicsConstantValue(0, 1, &constants0);

				ctx.DrawIndexed(lod.GetIndexCount());
			});

		auto tracePass = frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& builder) -> TracePass
			{
				builder.Requires(GetTypeGUID(Trangle));

				return TracePass{
                    .sbtBuffer = builder.ReadTransition(sbtUpdate.sbtBuffer, DeviceAccessState::DASNonPixelShaderResource,
											{ DeviceSyncPoint::Sync_Copy, DeviceSyncPoint::Sync_Raytracing }),
				    .traceBuffer = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture({ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT, false),DeviceAccessState::DASUAV),
					.renderTarget = builder.WriteTransition(
						            drawPass.renderTarget,
						        DeviceAccessState::DASCopyDest,
						        { DeviceSyncPoint::Sync_Draw, DeviceSyncPoint::Sync_Copy }),
				};
			},
			[=, this](const TracePass& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
			{
				auto& set0Layout = globalInterface->GetDescriptorSetLayout(0);
				FlexKit::DescriptorSet set(ctx, set0Layout, threadLocalAllocator);
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
				rayDesc.Width	= 800;
				rayDesc.Height	= 600;
				rayDesc.Depth	= 1;

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
		return nullptr;
	}


	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
	{
		renderWindow->Present();
		core.RenderSystem->ResetVertexBuffer(vBuffer);
	}

	double					t				= 0.0;
	size_t					vertexCount		= 0;
	IRenderWindow*			renderWindow	= nullptr;
	VertexBufferHandle		vBuffer			= InvalidHandle;
	ConstantBufferHandle	cBuffer			= InvalidHandle;
	UniqueResourceHandle	testTexture;
	TriMeshHandle			shape			= InvalidHandle;

	dx_Internal::MemoryPoolAllocator gpuAllocator;

	PersistentAllocator		persistent;
	IPipelineInterface*		globalInterface = nullptr;
    IPipelineStateLibrary*	library			= nullptr;
	ShaderBindingTable		sbt;
	
	ShaderID raygenID;
	ShaderID missID;
	ShaderID defaultGroup1;

	using DeviceAddressRange = FlexKit::DeviceAddressRange;
	GPURange SBTMemory;
	GPURange hitTable;
	GPURange missTable;
	GPURange rayGenerator;
};


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
