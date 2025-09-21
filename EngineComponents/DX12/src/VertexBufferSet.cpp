#include "src/VertexBufferSet.hpp"
#include "dxRenderSystem.hpp"
#include <TriMeshResource.hpp>

namespace dx_Internal
{
	DeviceResource_ptr GetBuffer(TriMesh* mesh, size_t lod, size_t Buffer)
	{
		return static_cast<VertexBufferSet*>(mesh->lods[lod].bufferSet)->At(Buffer).resource;
	}


	DeviceResource_ptr FindBuffer(TriMesh* mesh, size_t lod, VERTEXBUFFER_TYPE type)
	{
		if (auto res = mesh->lods[lod].bufferSet->Find(type); res.has_value())
			return res.value().resource;
		else
			return nullptr;
	}
}
