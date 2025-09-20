#include "vkCopyContext.hpp"

namespace VK_internal
{
	using namespace FlexKit;

	void vkCopyContext::Barrier(ID3D12Resource* destination, DeviceAccessState before, DeviceAccessState after)
	{}

	UploadReservation vkCopyContext::Reserve(size_t byteSize, uint32_t alignment)
	{
		return {};
	}

	void vkCopyContext::CopyBuffer(ResourceHandle source, size_t dstOffset, UploadReservation)
    {}

	void vkCopyContext::CopyBuffer(GPURange dest, void* source_ptr, uint64_t size)
    {}

	void vkCopyContext::CopyTextureRegion(ResourceHandle, size_t subResourceIdx, uint3 XYZ, UploadReservation source, uint2 WH, DeviceFormat format)
	{}

	void vkCopyContext::CopyTile(ResourceHandle dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
	{}

	bool vkCopyContext::IsSubResourceTiled(ResourceHandle Resource, const size_t level) const
	{
		return false;
	}

	IRenderSystem& vkCopyContext::GetRenderSystem() noexcept
	{
		return IRenderSystem::GetInstance();
	}
}
