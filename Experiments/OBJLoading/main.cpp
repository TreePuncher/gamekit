#include <Application.hpp>
#include <vkBackend.hpp>
#include <vkSurface.hpp>

#include <RenderSystemInterface.hpp>
#include <FrameGraph.hpp>
#include <filesystem>
#include <ModifiableShape.hpp>
#include <MeshUtilities.hpp>
#include <TriMeshResource.hpp>

#define USEVK 1

#if !USEVK
#include <Win32Graphics.hpp>
#include <dxBackend.hpp>
#endif

using namespace FlexKit;

FlexKit::TriMeshHandle LoadObj(std::filesystem::path p)
{
	using namespace FlexKit;

	Vector<char> buffer{ SystemAllocator };
	buffer.resize(std::filesystem::file_size(p));

	bool Loaded = FlexKit::LoadFileIntoBuffer(p.string().c_str(), (std::byte*)buffer.data(), buffer.size());// TODO: Make Thread Safe
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

	Vector<uint32_t>	indexes{ SystemAllocator };
	Vector<float3>		points{ SystemAllocator };
	Vector<float3>		normals{ SystemAllocator };
	Vector<float2>		uvs{ SystemAllocator };

	auto meshHandle = FlexKit::CreateMesh(123456789);
	auto& mesh		= *FlexKit::GetMeshResource(meshHandle);
	TriMesh::LOD_Runtime lod0;
	lod0.bufferSet = &FlexKit::IRenderSystem::GetInstance().CreateVertexBufferSet();

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
	size_t NormalBufferSize = optimizedBuffer.normals.ByteSize() + sizeof(VertexBufferView);// Normal
	size_t IndexBufferSize	= optimizedBuffer.indexes.ByteSize() + sizeof(VertexBufferView);// index

	lod0.views[0] = CreateVertexBufferView(SystemAllocator, VertexBufferSize);
	lod0.views[1] = CreateVertexBufferView(SystemAllocator, IndexBufferSize);
	lod0.views[2] = CreateVertexBufferView(SystemAllocator, NormalBufferSize);

	lod0.views[0]->Begin(VERTEXBUFFER_TYPE::POSITION, VERTEXBUFFER_FORMAT::R32G32B32);
	lod0.views[1]->Begin(VERTEXBUFFER_TYPE::INDEX, VERTEXBUFFER_FORMAT::R32);
	lod0.views[2]->Begin(VERTEXBUFFER_TYPE::NORMAL, VERTEXBUFFER_FORMAT::R32G32B32);

	memcpy(lod0.views[0]->GetBuffer(), optimizedBuffer.points.data(), optimizedBuffer.points.ByteSize());
	memcpy(lod0.views[1]->GetBuffer(), optimizedBuffer.indexes.data(), optimizedBuffer.indexes.ByteSize());
	memcpy(lod0.views[2]->GetBuffer(), optimizedBuffer.normals.data(), optimizedBuffer.normals.ByteSize());

	lod0.views[0]->MarkFull();
	lod0.views[1]->MarkFull();
	lod0.views[2]->MarkFull();

	lod0.views[0]->End();
	lod0.views[1]->End();
	lod0.views[2]->End();

	lod0.bufferSet->CreateBuffer(VERTEXBUFFER_TYPE::POSITION,	VERTEXBUFFER_FORMAT::R32G32B32,	optimizedBuffer.points.ByteSize());
	lod0.bufferSet->CreateBuffer(VERTEXBUFFER_TYPE::INDEX,		VERTEXBUFFER_FORMAT::R32,		optimizedBuffer.indexes.ByteSize());
	lod0.bufferSet->CreateBuffer(VERTEXBUFFER_TYPE::NORMAL,		VERTEXBUFFER_FORMAT::R32G32B32,	optimizedBuffer.normals.ByteSize());

	mesh.aabb	= aabb;
	mesh.bs		= float4{ aabb.MidPoint(), aabb.Span().magnitude() / 2.0f };

	auto copyCtx = IRenderSystem::GetInstance().GetImmediateCopyQueue();

	UploadVertexBuffer(copyCtx, lod0.bufferSet->At(0), *lod0.views[0]);
	UploadVertexBuffer(copyCtx, lod0.bufferSet->At(1), *lod0.views[1]);
	UploadVertexBuffer(copyCtx, lod0.bufferSet->At(2), *lod0.views[2]);

	SubMesh sm{
		.BaseIndex		= 0,
		.IndexCount		= (uint32_t)optimizedBuffer.IndexCount(),
		.materialIndex	= 0,
		.aabb			= aabb,
	};

	lod0.subMeshes.push_back(sm);
	mesh.lods.push_back(lod0);
	lod0.state = TriMesh::LOD_Runtime::LOD_State::Loaded;

	return meshHandle;
}

struct TestState : FrameworkState
{
	TestState(GameFramework& IN_framework) : FrameworkState(IN_framework)
	{
#if USEVK
#if WIN32
		renderWindow	= CreateWin32VKSurface(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
#else
		//renderWindow	= CreateWaylandSurface(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
#endif
#else
		renderWindow	= CreateWin32RenderWindow(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
#endif

		//testTexture		= GetRenderSystem().CreateGPUResource(GPUResourceDesc::ShaderResource({ 1024, 1024 }, DeviceFormat::R8G8B8A8_UNORM));

		GetRenderSystem().RegisterPSOLoader(GetTypeGUID(Trangle),
			[](IRenderSystem& renderSystem, iAllocator& allocator)
			{
				PipelineBuilder builder(renderSystem, allocator);
				builder.AddInputLayout({
					.inputs = {
						{
							.name	= "POSITION",
							.index	= 0,
							.format = DeviceFormat::R32G32B32_FLOAT,
							.inputSlotClass = EInputClassification::PerVertex,
						}},
					.count = 1
					});
				builder.AddVertexShader("VMain", "assets/shaders/TestShader.hlsl");
				builder.AddPixelShader("PMain", "assets/shaders/TestShader.hlsl");
				builder.AddRasterizerState();
				builder.AddRenderTargetState({
						.targetCount	= 1,
						.targetFormats	= { DeviceFormat::R8G8B8A8_UNORM },
					});

				return builder.Build(renderSystem, allocator);
			});

		GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

		vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
		cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);

		shape = LoadObj("Test.obj");
	}

	~TestState()
	{
		GetRenderSystem().ReleaseVB(vBuffer);
		GetRenderSystem().ReleaseCB(cBuffer);
	}

	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT)
	{
		t += dT;

#ifdef WIN32
#if USEVK
	    vkWin32UpdateInput();
#else
		Win32UpdateInput();
#endif
#else
		//ProcessEvents(*renderWindow);
#endif
	    return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
	{
		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);

		struct
		{
			float xyz[3];
		} triangle[3] = {
			{.xyz = { -1.0f, -1.0f,  0.0f }},
			{.xyz = {  0.0f,  1.0f,  0.0f }},
			{.xyz = {  1.0f, -1.0f,  0.0f }},
		};

		GetRenderSystem().VertexBufferPush(vBuffer, triangle, sizeof(triangle));

		ClearBackBuffer(frameGraph, renderTarget, { 0, 0, 0, 1 });

		struct DrawTrangle
		{
			FrameResourceHandle renderTarget;
		};

		frameGraph.AddConstantBuffer(cBuffer);

		auto node = frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& ){},
			[](const ResourceHandler&, IDirectContext&, iAllocator&){});

		frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& builder) -> DrawTrangle
			{
				builder.AddNodeDependency(node);
			    return DrawTrangle{
				    .renderTarget = builder.RenderTarget(renderTarget)
			    };
			},
			[=, this](const DrawTrangle& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
			{
				float fTime = (float)t;
				
				auto cb = resources.ReserveCB(512);
				
				struct
				{
					float time;
				} constants0{
					.time = fTime,
				};

			    struct
				{
					float4 xyz;
					float4 uvw;
				} constants1{
					.xyz = float4{ 0.0f, 1.0f, 0.0f, 0.0f },
					.uvw = float4{ 1.0f, 0.0f, 0.0f, 0.0f },
				};

				const auto cb0Set = ConstantBufferDataSet{ constants0, cb };
				//const auto cb1Set = ConstantBufferDataSet{ constants1, cb };

				const IPipelineInterface* pipelineInterface = resources.GetPipelineState(GetTypeGUID(Trangle), threadLocalAllocator)->GetInterface();
				DescriptorSet descriptorSet{ ctx, pipelineInterface->GetDescHeap(0), threadLocalAllocator};
				descriptorSet.SetCBV(ctx, 0, cb0Set);

				ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), threadLocalAllocator);
				//ctx.SetVertexBuffers(static_vector<VertexBufferEntry, 1>{ VertexBufferEntry
				//	                    {
				//		                    .VertexBuffer	= vBuffer,
		        //                            .Stride			= 36,
		        //                            .Offset			= 0, 
				//                        } });

				auto mesh = GetMeshResource(shape);
				auto& lod = mesh->lods[0];

				ctx.AddVertexBuffers(mesh, 0, { VERTEXBUFFER_TYPE::POSITION });
				ctx.AddIndexBuffer(mesh, 0);

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				//ctx.SetGraphicsConstantValue(0, 8, &constants0);
				//ctx.SetGraphicsConstantBufferView(0, cb1Set);
				ctx.SetGraphicsDescriptorTable(0, descriptorSet);

				//ctx.SetGraphicsConstantBufferView(0, cBuffer, 0);

				ctx.DrawIndexed(lod.GetIndexCount());
			});

		PresentBackBuffer(frameGraph, *renderWindow);
	    return nullptr;
	}


	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
	{
		bool res = renderWindow->Present();
		core.RenderSystem->ResetVertexBuffer(vBuffer);
	}

	double					t				= 0.0;
	size_t					vertexCount		= 0;
	IRenderWindow*			renderWindow	= nullptr;
	VertexBufferHandle		vBuffer			= InvalidHandle;
	ConstantBufferHandle	cBuffer			= InvalidHandle;
	UniqueResourceHandle	testTexture;
	TriMeshHandle			shape			= InvalidHandle;
};

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator,
			FlexKit::CoreOptions{
				.GPUdebugMode		= true,
				.GPUValidation		= true,
				.GPUSyncQueues		= true,
#if USEVK
				.CreateRenderSystem = CreateVK,
#else
				.CreateRenderSystem = CreateDX,
#endif
			}, FlexKit::FrameworkOptions{
				.integrateIMGUI		= false
			});

		app->PushState<TestState>();
		app->GetCore().FPSLimit		= 144;
		app->GetCore().FrameLock	= true;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (std::runtime_error runtimeError)
	{
		FK_LOG_ERROR("Exception Caught!\n%s", runtimeError.what());
	}
	catch (...)
	{
		FK_LOG_ERROR("Exception Caught!");
		return -1;
	}
	return 0;
}


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
