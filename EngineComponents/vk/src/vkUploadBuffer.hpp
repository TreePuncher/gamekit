#pragma once
#include <vulkan/vulkan.hpp>
#include <RenderSystemInterface.hpp>

namespace VK_internal
{
	using namespace FlexKit;

	struct vkUploadBuffer
	{
		vkUploadBuffer();

		vkUploadBuffer(vkUploadBuffer&&);
		vkUploadBuffer& operator = (vkUploadBuffer&&) noexcept;

		vkUploadBuffer(const vkUploadBuffer&) = delete;
		vkUploadBuffer& operator =	(const vkUploadBuffer&) = delete;

		~vkUploadBuffer();

		void Release();

		std::expected<UploadReservation, ReserveErrors> Reserve(const size_t size, const size_t reserveAlignement);

		VkBuffer Resize(const size_t size); // Returns old resource

		VkBuffer		deviceBuffer	= nullptr;
		VkDeviceMemory  memory			= nullptr;

		size_t			position		= 0;
		size_t			last			= 0;
		size_t			size			= 0;
		char*			buffer			= nullptr;
	};

}
