#include <RenderSystemInterface.hpp>
#include <ModifiableShape.hpp>
#include <MeshUtilities.hpp>
#include <TriMeshResource.hpp>

#include "OBJLoader.hpp"

using namespace FlexKit;

TriMeshHandle LoadObj(std::filesystem::path p)
{
	if (!std::filesystem::exists(p))
		FK_LOG_ERROR(std::format("File Not Found! {}", p.string()).c_str());

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

	Vector<uint32_t>	indexes{ SystemAllocator };
	Vector<float3>		points{ SystemAllocator };
	Vector<float3>		normals{ SystemAllocator };
	Vector<float2>		uvs{ SystemAllocator };

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
			optimized.PushTri(kdbTree.mesh.tris[I], context);

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
