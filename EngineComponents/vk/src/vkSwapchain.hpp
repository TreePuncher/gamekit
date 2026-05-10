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
		VkSemaphore&	GetAcquireWait();
		VkSemaphore&	GetSubmitSignal();
		VkFence&		GetFrameFence();


        VkSwapchainKHR	swapchain = nullptr;
		VkSemaphore		acquireWait[10];
		VkSemaphore		renderingSignal[10];

	    ResourceHandle	resource				= InvalidHandle;
		VkImage			images[10]				= { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		VkFence			frameFences[10]			= { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		VkImageView		views[10]				= { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		DeviceLayout	layout[10]				= { DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined, DeviceLayout::Undefined };
		uint32_t		imageIndex				= 0;
		uint32_t		frameIndex				= 0;
		uint32_t		swapchainCount			= 0;
		uint64_t		frameSubmissionIDs[10]	= { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
		VkSemaphore		current[2];

		Extra_SignalBlock	signaling;
    };
}   // namespace VK_internal
