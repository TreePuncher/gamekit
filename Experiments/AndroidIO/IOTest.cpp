
#include <Events.hpp>
#include <Application.hpp>
#include <FrameGraph.hpp>
#include <RenderSystemInterface.hpp>
#include <ModifiableShape.hpp>
#include <MeshUtilities.hpp>
#include <TriMeshResource.hpp>

#include <jni.h>
#include <android/log.h>

#include <cstdint>
#include <chrono>
#include <memory>
#include <thread>
#include <filesystem>

#include <AndroidIOHelpers.hpp>

using std::filesystem::path;

using namespace FlexKit;
using namespace std::chrono_literals;

TriMeshHandle LoadObj(std::filesystem::path p)
{
	using namespace FlexKit;

    if(!std::filesystem::exists(p))
		FK_LOG_ERROR("LoadObj: Path doesn't exist!\n");

    const size_t fileSize = std::filesystem::file_size(p);

    auto message = std::format("File: {}, Size: {}", p.string(), fileSize);
    FK_LOG_INFO(message.c_str());

    if(fileSize == 0)
    {
		FK_LOG_ERROR("LoadObj: File Size is zero!\n");
        return InvalidHandle;
    }

    auto ptr = SystemAllocator.malloc(fileSize);

    Vector<char> buffer{ SystemAllocator };

    auto message2 = std::format("d1: P{}, d2: P{}, allocator: {}, test: {}", buffer.size(), buffer.Max, (uint64_t)buffer.Allocator, ptr);
    FK_LOG_INFO(message2.c_str());

	buffer.resize(fileSize);

	bool Loaded = FlexKit::LoadFileIntoBuffer(p.string().c_str(), (std::byte*)buffer.data(), buffer.size());// TODO: Make Thread Safe
	if (!Loaded)
	{
		FK_LOG_ERROR("LoadObj: Failed To Load Obj\n");
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

struct TestState final : public FrameworkState
{
    TestState(GameFramework& IN_framework, IRenderWindow* IN_renderWindow, const path& p) : 
        FrameworkState  { IN_framework      },
        assetPath       { p },
        renderWindow    { IN_renderWindow   } 
        {
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
							}},
						.count = 1
						});
					builder.AddVertexShader("VMain");
					builder.AddPixelShader("PMain");
					builder.AddRasterizerState();
					builder.AddRenderTargetState({
							.targetCount = 1,
							.targetFormats = { DeviceFormat::R8G8B8A8_UNORM },
						});

					return builder.Build(renderSystem, allocator);
				});

			meshResource = LoadObj(path{ p.string() + "/suzanne.obj" });
			AddAssetFile((p.string() + "/shaderpack.gameres").c_str());

			vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
			cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);
        }


    UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT) final 
    { 
        std::this_thread::sleep_for(1ms);

        return nullptr; 
    }


    UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph) final
    { 
        static bool toggle = false;
		auto renderTarget = renderWindow->GetBackBuffer();
        frameGraph.AddOutput(renderTarget);

		float g = (float)sinf(t) / 2.0f + 0.5f;

        ClearBackBuffer(
            frameGraph, 
			renderTarget, 
            float4{ 
                g,
                g,
                g,
                0.0f });

		struct DrawTrangle
		{
			FrameResourceHandle renderTarget;
		};

		frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& builder) -> DrawTrangle
			{
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

				const IPipelineInterface* pipelineInterface = resources.GetPipelineState(GetTypeGUID(Trangle), threadLocalAllocator)->GetInterface();
				DescriptorSet descriptorSet{ ctx, pipelineInterface->GetDescHeap(0), threadLocalAllocator };
				descriptorSet.SetCBV(ctx, 0, cb0Set);

				ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), threadLocalAllocator);

				auto mesh = GetMeshResource(meshResource);
				auto& lod = mesh->lods[0];

				ctx.AddVertexBuffers(mesh, 0, { VERTEXBUFFER_TYPE::POSITION });
				ctx.AddIndexBuffer(mesh, 0);

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				ctx.SetGraphicsDescriptorTable(0, descriptorSet);

				ctx.DrawIndexed(lod.GetIndexCount());
			});

		PresentBackBuffer(frameGraph, renderWindow->GetBackBuffer());

        toggle = !toggle;

        return nullptr;
    }


    void PostDrawUpdate(EngineCore& core, double dT) final 
    {
        renderWindow->Present(1, 0);
		core.RenderSystem->ResetVertexBuffer(vBuffer);
        t += dT;
    }


    bool EventHandler(Event evt) final 
    { 
        if(evt.mType == Event::System && evt.InputSource == Event::E_SystemEvent && evt.Action == Event::Exit)
        {
            FK_LOG_0("Quit Event Received");

            framework.PopState();    
        }
        return true;	
    }

    path					assetPath;
    TriMeshHandle			meshResource;
    IRenderWindow*			renderWindow	= nullptr;
	VertexBufferHandle		vBuffer			= InvalidHandle;
	ConstantBufferHandle	cBuffer			= InvalidHandle;
    double					t = 0.0;
};

void SetupApplication(FKApplication& app, IRenderWindow* renderWindow, struct android_app *pApp)
{
    __android_log_write(ANDROID_LOG_VERBOSE, "FlexKit", "SetupApplication(): Created Application!");

    auto assetPath = GetGameAssetPath(pApp);
    app.PushState<TestState>(renderWindow, assetPath);
}
