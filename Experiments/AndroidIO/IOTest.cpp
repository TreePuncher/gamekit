
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


void Test_PushBack(FlexKit::Vector<char>& vector, char val)
{
    FK_LOG_INFO("1");

    if (vector.Size + 1 > vector.Max)
    {// Increase Size
#ifdef _DEBUG
        FK_ASSERT(vector.Allocator);
#endif			
        auto NewSize = ((vector.Max < 1) ? 2 : (2 * vector.Max));
        char* NewMem = (char*)vector.Allocator->_aligned_malloc(sizeof(char) * NewSize);
        {
            std::string msg = std::format("NewSize: {}, NewMem1: {}", NewSize, (uint64_t)(NewMem));
            FK_LOG_INFO(msg.c_str());
        }

        const size_t End = vector.Size;
        for (size_t itr = 0; itr < End; ++itr)
            new(NewMem + itr) char();

#ifdef _DEBUG
        FK_ASSERT(NewMem != nullptr);
        if (vector.Size)
            FK_ASSERT(NewMem != A);
#endif

        FK_LOG_INFO("2");

        if (vector.A)
        {
            size_t itr = 0;
            size_t End = vector.Size;
            for (; itr < End; ++itr)
                NewMem[itr] = std::move(vector.A[itr]);

            if(vector.A != vector.internalBuffer.GetBuffer())
                vector.Allocator->_aligned_free(vector.A);
        }

        FK_LOG_INFO("3");

        {
            std::string msg = std::format("NewMem2: {}", (uint64_t)(NewMem));
            FK_LOG_INFO(msg.c_str());
        }

        vector.A   = NewMem;
        vector.Max = NewSize;
    }

    FK_LOG_INFO("4");

	const size_t idx = vector.Size++;
    std::string msg = std::format("vector.A: {}", (uint64_t)(vector.A + idx));
    FK_LOG_INFO(msg.c_str());

    std::construct_at<char>(vector.A + idx, val);
    FK_LOG_INFO("5");
}

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
            LoadObj( path{ p.string() + "/suzanne.obj" });
        }


    UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT) final 
    { 
        std::this_thread::sleep_for(1ms);

        return nullptr; 
    }


    UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph) final
    { 
        static bool toggle = false;
        frameGraph.AddOutput(renderWindow->GetBackBuffer());
        
        ClearBackBuffer(
            frameGraph, renderWindow->GetBackBuffer(), 
            float4{ 
                (float)sinf(t) / 2.0f + 0.5f, 
                (float)sinf(t * 3.0f) / 2.0f + 0.5f, 
                (float)sinf(t * 7.0f) / 2.0f + 0.5f, 
                0.0f });

        toggle = !toggle;

        return nullptr;
    }


    void PostDrawUpdate(EngineCore&, double dT) final 
    {
        renderWindow->Present(1, 0);
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

    path            assetPath;
    TriMeshHandle   mesh;
    IRenderWindow*  renderWindow = nullptr;
    double          t = 0.0;
};

void SetupApplication(FKApplication& app, IRenderWindow* renderWindow, struct android_app *pApp)
{
    __android_log_write(ANDROID_LOG_VERBOSE, "FlexKit", "SetupApplication(): Created Application!");

    auto assetPath = GetGameAssetPath(pApp);
    
    app.PushState<TestState>(renderWindow, assetPath);
}
