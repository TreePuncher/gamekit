#include <TriMeshResource.hpp>
#include <RenderSystemInterface.hpp>

namespace FlexKit
{}

namespace dx_Internal
{
    using namespace FlexKit;

	DeviceResource_ptr GetBuffer(TriMesh* Mesh, size_t lod, size_t Buffer);
	DeviceResource_ptr FindBuffer(TriMesh* Mesh, size_t lod, VERTEXBUFFER_TYPE type);
}
