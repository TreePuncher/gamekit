#include <RenderSystemInterface.hpp>

namespace VK_internal
{
    using namespace FlexKit;

	struct vkCopyContext : public ICopyContext
	{
		void                Barrier(ID3D12Resource* destination, DeviceAccessState before, DeviceAccessState after) final;

		UploadReservation	Reserve(size_t byteSize, uint32_t alignment) final;
		void				CopyBuffer(ResourceHandle source, size_t dstOffset, UploadReservation) final;
		void                CopyBuffer(GPURange dest, void* source_ptr, uint64_t size) final;
		void                CopyTextureRegion(ResourceHandle, size_t subResourceIdx, uint3 XYZ, UploadReservation source, uint2 WH, DeviceFormat format) final;
		void                CopyTile(ResourceHandle dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src) final;

		bool				IsSubResourceTiled(ResourceHandle Resource, const size_t level) const final;

		IRenderSystem&		GetRenderSystem() noexcept final;
	};

    
}
