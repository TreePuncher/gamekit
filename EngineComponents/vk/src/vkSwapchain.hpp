#pragma once
#include "vkRenderSystem.hpp"

namespace VK_internal
{
    struct vkSwapchain
    {
        vkSwapchain(VkSurfaceKHR surface, uint2 WH, DeviceFormat format);

        void UpdateBufferIdx();

		uint2 GetWH() const;

		bool Present(const uint32_t syncInternal = 0, const uint32_t flags = 0);
		void Resize(const uint2 WH);
		void Release();
		

		VkSemaphore& GetSemaphore();
		VkSemaphore& GetNextSemaphore();


        VkSwapchainKHR	swapchain = nullptr;
		VkSemaphore		semaphores[3];
		VkSemaphore		presentSemaphores[3];

	    ResourceHandle	resource	= InvalidHandle;
		VkImage			images[3];
		VkFence			windowFences[3];
		VkImageView		views[3]	= { nullptr, nullptr, nullptr };
		DeviceLayout	layout[3]	= { DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined };
		uint32_t		imageIndex	= 0;
		VkSemaphore		current[2];
    };
}   // namespace VK_internal
