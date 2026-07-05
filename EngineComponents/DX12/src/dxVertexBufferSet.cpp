#include "dxVertexBufferSet.hpp"
#include "dxRenderSystem.hpp"
#include <TriMeshResource.hpp>

namespace dx_Internal
{
	DeviceResource_ptr GetBuffer(TriMesh* mesh, size_t lod, size_t Buffer)
	{
		return static_cast<dxVertexBufferSet*>(mesh->lods[lod].bufferSet)->At(Buffer).resource;
	}


	DeviceResource_ptr FindBuffer(TriMesh* mesh, size_t lod, VERTEXBUFFER_TYPE type)
	{
		if (auto res = mesh->lods[lod].bufferSet->Find(type); res.has_value())
			return res.value().resource;
		else
			return nullptr;
	}


	std::optional<VertexBuffer> dxVertexBufferSet::Find(const VERTEXBUFFER_TYPE type) const
	{
		auto res = std::find_if(
			buffers.begin(),
			buffers.end(),
			[&](auto& buffer)
			{
				return buffer.type == type;
			});

		if (res != buffers.end())
		{
			VertexBuffer out{
				.byteSize	= res->bufferSizeInBytes,
				.byteStride = res->bufferStride,
				.resource	= res->apiResource,
				.type		= res->type
			};

			return out;
		}

		return {};
	}

	std::optional<uint32_t>	dxVertexBufferSet::FindIdx(VERTEXBUFFER_TYPE) const
	{
		return {};
	}


	uint8_t	dxVertexBufferSet::GetIndexBufferIndex() const
	{
		return MD.IndexBuffer_Index;
	}

	DevicePointer dxVertexBufferSet::GetBufferPointer(VERTEXBUFFER_TYPE type) const
	{
		if (auto res = Find(type); res)
		    return DevicePointer{ res.value().resource.As<ID3D12Resource>()->GetGPUVirtualAddress() };
		else 
		    return {};
	}

	void dxVertexBufferSet::CreateBuffer(VERTEXBUFFER_TYPE type, VERTEXBUFFER_FORMAT format, size_t byteSize)
	{
		auto& dxRS = static_cast<dxRenderSystem&>(dxRenderSystem::GetInstance());
		ID3D12Resource* apiResource = dxRS._CreateVertexBufferDeviceResource(byteSize, true);

		SETDEBUGNAME(apiResource, "VertexBuffer");

		auto idx = buffers.emplace_back(
			BuffEntry{
			    .apiResource		= apiResource,
			    .bufferSizeInBytes	= (uint32_t)Min(byteSize, std::numeric_limits<uint32_t>::max()),
			    .bufferStride		= (uint32_t)format,
			    .type				= type
			}
		);

		if (type == VERTEXBUFFER_TYPE::INDEX)
			MD.IndexBuffer_Index = idx;
	}

	void dxVertexBufferSet::ReleaseBuffer(VERTEXBUFFER_TYPE type)
	{
		auto res = std::ranges::find_if(
			buffers,
			[type](const BuffEntry& buffer)
			{
				return buffer.type == type;
			});

		if (res != buffers.end())
		{
			res->apiResource->Release();
			buffers.remove_unstable(res);

			auto idx = FindIdx(VERTEXBUFFER_TYPE::INDEX);
			MD.IndexBuffer_Index = idx.value_or(0xffffffffffffffff);
		}
	}

	void dxVertexBufferSet::Release()
	{
		auto& dxRS = static_cast<dxRenderSystem&>(dxRenderSystem::GetInstance());
		
		dxRS.LockedResourceOperation(
			[&]
			{
				for (auto& buf : buffers)
					dxRS._PushDelayReleasedResource(buf.apiResource);
		        
			    buffers.clear();
			});

		dxRS.allocator->free(this);
	}

	const VertexBuffer dxVertexBufferSet::operator []	(uint8_t idx) const
	{
		auto& buffer = buffers[idx];
		return {
			.byteSize	= buffer.bufferSizeInBytes,
			.byteStride = buffer.bufferStride,
			.resource	= buffer.apiResource,
			.type		= buffer.type
		};
	}

	void dxVertexBufferSet::Clear()
	{
		buffers.clear();
	}
}
