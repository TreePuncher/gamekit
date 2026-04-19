#include "vkCopyContext.hpp"
#include <MathUtilities.hpp>
#include <vkRenderSystem.hpp>
#include <vkUploadBuffer.hpp>

namespace VK_internal
{
	using namespace FlexKit;

	vkCopyContext::vkCopyContext()
	{
		auto& renderSystem = (vkRenderSystem&)vkRenderSystem::GetInstance();

		const static uint32_t queueIndex = renderSystem.device.get_queue_index(vkb::QueueType::transfer).has_value();

		VkCommandPoolCreateInfo createPoolDesc{
				.sType				= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext				= nullptr,
				.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex	= queueIndex
		};

		if (auto res = vkCreateCommandPool(renderSystem.device, &createPoolDesc, nullptr, &commandPool); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to create command pool!" };

		VkCommandBufferAllocateInfo createCommandBuffer{
			.sType			= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext			= nullptr,
			.commandPool	= commandPool,
			.level			= VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		if (auto res = vkAllocateCommandBuffers(renderSystem.device, &createCommandBuffer, &cmdBuffer); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to create command buffer" };

		VkCommandBufferBeginInfo beginInfo{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext				= nullptr,
			.flags				= VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo	= nullptr,
		};

		if (auto res = vkBeginCommandBuffer(cmdBuffer, &beginInfo); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to begin copy command buffer" };
	}

	void vkCopyContext::Barrier(ResourceHandle destination, DeviceAccessState before, DeviceAccessState after)
	{}

	UploadReservation vkCopyContext::Reserve(size_t byteSize, uint32_t alignment)
	{
		auto reservation = copyBuffer.Reserve(byteSize, 256);

		FK_ASSERT(reservation.has_value());

		return reservation.value();
	}

	void vkCopyContext::CopyBuffer(ResourceHandle source, size_t dstOffset, UploadReservation)
    {
	}

	void vkCopyContext::CopyBuffer(GPURange dest, void* source_ptr, uint64_t size)
    {
		auto uploadSize		= FlexKit::Min(dest.size, size);
		auto uploadSpace	= Reserve(uploadSize, 256);

		memcpy(uploadSpace.buffer, source_ptr, uploadSize);

		VkBufferCopy copy{
			.srcOffset	= uploadSpace.offset,
			.dstOffset	= dest.offset,
			.size		= uploadSize
		};

		vkCmdCopyBuffer(cmdBuffer,
			uploadSpace.resource.As_ptr<VkBuffer>(),
			(VkBuffer)dest.devicePtr,
			1,
			&copy);
	}

	void vkCopyContext::CopyTextureRegion(ResourceHandle target, size_t subResourceIdx, uint3 XYZ, UploadReservation upload, uint2 WH)
	{
		auto& vkRS = GetVkRenderSystem();
		auto [apiObject, layout, dxFormat] = vkRS.resources.Get<ResourceFieldID::APIHandle, ResourceFieldID::Layout, ResourceFieldID::Format>(target);

		FK_ASSERT(apiObject.type == vkResourceEntry::Type::Image);

		const auto format		= FormatToVK(dxFormat);
		const auto formatSize	= GetFormatElementSize(format);
		const auto rowSize		= AlignedSize(formatSize * WH[0]);

		VkBufferImageCopy region{
			.bufferOffset		= upload.offset,
			.bufferRowLength	= (uint32_t)rowSize,
			.bufferImageHeight	= WH[1],
			.imageSubresource{
				.aspectMask		= VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel		= (uint32_t)subResourceIdx,
				.baseArrayLayer	= 0,
				.layerCount		= 1,
			},
			.imageOffset		= VkOffset3D{ (int32_t)XYZ[0], (int32_t)XYZ[1], (int32_t)XYZ[2] },
			.imageExtent		= VkExtent3D{ WH[0], WH[1], 1 }
		};

		vkCmdCopyBufferToImage(
			cmdBuffer,
			upload.resource.As_ptr<VkBuffer>(),
			apiObject.image,
			VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			1, &region);
	}

	void vkCopyContext::CopyTile(ResourceHandle dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
	{
	}

	bool vkCopyContext::IsSubResourceTiled(ResourceHandle Resource, const size_t level) const
	{
		return false;
	}

	void vkCopyContext::Close() noexcept
	{
		vkEndCommandBuffer(cmdBuffer);
	}

	IRenderSystem& vkCopyContext::GetRenderSystem() noexcept
	{
		return IRenderSystem::GetInstance();
	}

	vkRenderSystem& vkCopyContext::GetVkRenderSystem() noexcept
	{
		return static_cast<VK_internal::vkRenderSystem&>(GetRenderSystem());
	}

}
