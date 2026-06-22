#include <TriMeshResource.hpp>
#include <RenderSystemInterface.hpp>
#include <directx/d3d12.h>

namespace dx_Internal
{
    using namespace FlexKit;

	DeviceResource_ptr GetBuffer(TriMesh* Mesh, size_t lod, size_t Buffer);
	DeviceResource_ptr FindBuffer(TriMesh* Mesh, size_t lod, VERTEXBUFFER_TYPE type);


	typedef static_vector<D3D12_INPUT_ELEMENT_DESC, 16> InputDescription;

	struct TriangleMeshMetaData
	{
		D3D12_INPUT_ELEMENT_DESC	InputLayout[16];
		uint32_t					InputElementCount;
		uint32_t					IndexBuffer_Index;
	};


	struct dxVertexBufferSet final : public IVertexBufferSet
	{
		struct BuffEntry
		{
			struct ID3D12Resource*	apiResource;
			uint32_t				bufferSizeInBytes;
			uint32_t				bufferStride;
			VERTEXBUFFER_TYPE		type;
			
			size_t Size() const noexcept { return bufferSizeInBytes / bufferStride; }

			DevicePointer		GetDevicePointer()	const noexcept { return { apiResource->GetGPUVirtualAddress() }; }
			DeviceResource_ptr	GetAPIPointer()		const noexcept { return { apiResource }; }

			operator bool() const noexcept { return apiResource != nullptr; }
			operator DeviceResource_ptr const () const noexcept { return apiResource; }
		};

		virtual ~dxVertexBufferSet() override { Release(); }

		virtual void						Clear() override;
		virtual std::optional<VertexBuffer> Find(const VERTEXBUFFER_TYPE type) const override;
		virtual std::optional<uint32_t>		FindIdx(VERTEXBUFFER_TYPE) const override;

		virtual const VertexBuffer			operator []	(uint8_t idx) const override;
		virtual uint8_t						GetIndexBufferIndex() const override;

		virtual DevicePointer				GetBufferPointer(VERTEXBUFFER_TYPE) const override;

		virtual void CreateBuffer(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT, size_t byteSize) override;
		virtual void ReleaseBuffer(VERTEXBUFFER_TYPE) override;

		virtual void Release() override;


		static_vector<BuffEntry, 16>	buffers;
		TriangleMeshMetaData			MD{
			.InputElementCount = 0,
			.IndexBuffer_Index = 0xffffffff,
		};
	};

}
