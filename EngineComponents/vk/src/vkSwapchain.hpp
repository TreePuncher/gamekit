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
		
		ResourceHandle Resource() const noexcept;

	private:
		VkSemaphore& GetAcquireWait();
		VkSemaphore& GetSubmitSignal();


        VkSwapchainKHR	swapchain = nullptr;
		VkSemaphore		acquireWait[3];
		VkSemaphore		renderingSignal[3];

	    ResourceHandle	resource				= InvalidHandle;
		VkImage			images[3]				= { nullptr, nullptr, nullptr };
		VkFence			windowFences[3]			= { nullptr, nullptr, nullptr };
		VkImageView		views[3]				= { nullptr, nullptr, nullptr };
		DeviceLayout	layout[3]				= { DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined };
		uint32_t		imageIndex				= 0;
		uint32_t		frameIndex				= 0;
		uint64_t		frameSubmissionIDs[3]	= { 0u, 0u, 0u };
		VkSemaphore		current[2];

		Extra_SignalBlock	signaling;
    };
}   // namespace VK_internal
