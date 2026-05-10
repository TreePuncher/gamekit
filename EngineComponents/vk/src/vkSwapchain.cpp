#include "vkSwapchain.hpp"
#include <exception>

namespace VK_internal
{
    vkSwapchain::vkSwapchain(VkSurfaceKHR surface, uint2 IN_WH, DeviceFormat IN_format)
    {
        auto& vkRS = static_cast<vkRenderSystem&>(vkRenderSystem::GetInstance());

        FK_LOG_9("VK: Querying surface capabilities!");

        VkSurfaceCapabilitiesKHR capabilities;
		VkSurfaceFormatKHR formats[128];
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkRS.device.physical_device, surface, &capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkRS.device.physical_device, surface, &swapchainCount, nullptr);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkRS.device.physical_device, surface, &swapchainCount, formats);

        auto message = std::format("VK: Swapchain Count: {}", swapchainCount);
        FK_LOG_9(message.c_str());

		VkSwapchainCreateInfoKHR createSwapChainInfo{
		    .sType					= VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext					= 0,
            .flags					= 0,
            .surface				= surface,
            .minImageCount			= 3,
            .imageFormat			= VkFormat::VK_FORMAT_R8G8B8A8_UNORM,
            .imageColorSpace		= VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .imageExtent			= { .width = capabilities.maxImageExtent.width, .height = capabilities.maxImageExtent.height },
            .imageArrayLayers		= 1,
            .imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount	= 0,
            .pQueueFamilyIndices	= nullptr,
            .preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode			= VK_PRESENT_MODE_FIFO_KHR,
            .clipped				= false,
            .oldSwapchain			= nullptr
		};

        FK_LOG_9("VK: Creating Swapchain!");
		if (auto res = vkCreateSwapchainKHR(vkRS.device, &createSwapChainInfo, nullptr, &swapchain); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("VK: Failed to create vulkan swapchain");
			throw std::runtime_error{ "VK: failed to create vulkan swapchain!" };
		}

		auto desc			= GPUResourceDesc::RenderTarget(IN_WH, IN_format);
		desc._ptr			= swapchain;
		desc.initialLayout	= DeviceLayout::Undefined;

		auto renderTarget = vkRS.CreateGPUResource(desc);

        FK_LOG_9("VK: Getting Swapchain Images!");
		uint imageCount;
		vkGetSwapchainImagesKHR(vkRS.device, swapchain, &imageCount, images);

		VkImageViewCreateInfo createInfo{};
		createInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType							= VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format							= createSwapChainInfo.imageFormat;
		createInfo.components.r						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel	= 0;
		createInfo.subresourceRange.levelCount		= 1;
		createInfo.subresourceRange.baseArrayLayer	= 0;
		createInfo.subresourceRange.layerCount		= 1;

        FK_LOG_9("VK: Getting Images Views!");

		for (size_t i = 0; i < swapchainCount; i++)
		{
			createInfo.image = images[i];
			if (auto res = vkCreateImageView(vkRS.device, &createInfo, nullptr, &views[i]); res != VK_SUCCESS)
				FK_LOG_ERROR("VK: Failed to create swapchain image view");
		}

		VkSemaphoreCreateInfo createSemaphoreInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
		};

        FK_LOG_9("VK: Creating Semaphores!");
        for(size_t i = 0; i < swapchainCount; i++)
		    if (auto res = vkCreateSemaphore(vkRS.device, &createSemaphoreInfo, nullptr, acquireWait + i); res != VK_SUCCESS)
			    throw std::runtime_error("VK: Failed to create binary semaphore!");
            
        for(size_t i = 0; i < swapchainCount; i++)
            if (auto res = vkCreateSemaphore(vkRS.device, &createSemaphoreInfo, nullptr, renderingSignal + i); res != VK_SUCCESS)
                throw std::runtime_error("VK: Failed to create binary semaphore queue!");

        FK_LOG_9("VK: Creating Fences!");

		VkFenceCreateInfo createFenceInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

        for(size_t i = 0; i < swapchainCount; i++)
		    if (auto res = vkCreateFence(vkRS.device, &createFenceInfo, nullptr, frameFences + i); res != VK_SUCCESS)
			    throw std::runtime_error("VK: Failed to create fence for direct queue!");

		vkRS.resources.Set<ResourceFieldID::Extra, ResourceFieldID::Flags, ResourceFieldID::View, ResourceFieldID::XYZW>(
			    renderTarget,
			    (void*)current,
			    ResourceFlags::SwapChain | ResourceFlags::RenderTarget,
			    vkResourceViews{ .imageView = views[0] },
                uint4{ createSwapChainInfo.imageExtent.width, createSwapChainInfo.imageExtent.height }
		);

		resource = renderTarget;

		UpdateBufferIdx();

        FK_LOG_9("VK: Finished Creating Swapchain!");
    }

    void vkSwapchain::UpdateBufferIdx()
    {
        FK_LOG_9("VK: Updating Swapchain Buffer Index!");

        auto& renderSystem = static_cast<vkRenderSystem&>(vkRenderSystem::GetInstance());

        layout[imageIndex] = renderSystem.resources.Get<ResourceFieldID::Layout>(resource);;

        auto acquireWait        = GetAcquireWait();
        auto renderFinishSignal = GetSubmitSignal();

        if (const uint64_t submissionID = frameSubmissionIDs[frameIndex]; submissionID > 0)
        {
            FK_LOG_9("VK:vkSwapChain::UpdateBufferIdx(): Waiting for frame!");
            if(auto HR = vkWaitForFences(renderSystem.device, 1, &GetFrameFence(), true, 10000000000000); HR != VK_SUCCESS)
                FK_LOG_ERROR("VK:vkSwapChain::UpdateBufferIdx(): Failed To Wait for fence! EC: %u", HR);
        }
        if (auto res = vkAcquireNextImageKHR(renderSystem.device, swapchain, 10000000000000, acquireWait, nullptr, &imageIndex); res != VK_SUCCESS)
        {
            FK_LOG_ERROR("VK:vkSwapChain::UpdateBufferIdx(): Failed To Acquire Next Image!");
            throw std::runtime_error("Failed to get next image!");
        }
        renderSystem.resources.Set<ResourceFieldID::APIHandle, ResourceFieldID::Layout, ResourceFieldID::View>(
            resource,
            vkResourceEntry{
                .type	= vkResourceEntry::Type::RenderTarget,
                .image	= images[imageIndex]
            },
            layout[imageIndex],
            vkResourceViews{ .imageView = views[imageIndex] });

        current[0] = acquireWait;
        current[1] = renderFinishSignal;
    }

    uint2 vkSwapchain::GetWH() const
    {
        return vkRenderSystem::GetInstance().GetTextureTilingWH(resource, 0);
    }

    bool vkSwapchain::Present(const uint32_t syncInternal, const uint32_t flag)
    {
        FK_LOG_9("VK: Presenting Swapchain!");

        auto& renderSystem = static_cast<vkRenderSystem&>(vkRenderSystem::GetInstance());
        frameSubmissionIDs[frameIndex] = renderSystem.directSubmissionCounter;
        const auto fence = GetFrameFence();

        VkSwapchainPresentFenceInfoEXT presentFence{
            .sType          = VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
            .pNext          = nullptr,
            .swapchainCount = 1,
            .pFences        = &fence
        };

        VkPresentInfoKHR presentInfo = {
            .sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext				= &presentFence,

            .waitSemaphoreCount	= 1,
            .pWaitSemaphores	= &GetSubmitSignal(),

            .swapchainCount		= 1,
            .pSwapchains		= &swapchain,

            .pImageIndices		= &imageIndex,
            .pResults			= nullptr
        };

        vkResetFences(renderSystem.device, 1, &fence);
        auto res = vkQueuePresentKHR(renderSystem.GetQueue(), &presentInfo) == VK_SUCCESS;

        frameIndex = ++frameIndex % swapchainCount;
        UpdateBufferIdx();

        FK_LOG_9("VK: Finished Presenting Swapchain!");

        return res;
    }

    void vkSwapchain::Resize(const uint2 WH)
    {
        
    }

    void vkSwapchain::Release()
    {
        vkRenderSystem::GetInstance().ReleaseResource(resource);
    }

    ResourceHandle vkSwapchain::Resource() const noexcept
    {
        return resource;
    }

    VkSemaphore& vkSwapchain::GetAcquireWait() 
    {
        return acquireWait[frameIndex];
    }

    VkSemaphore& vkSwapchain::GetSubmitSignal() 
    {
        return renderingSignal[frameIndex];
    }

    VkFence& vkSwapchain::GetFrameFence()
    {
        return frameFences[frameIndex];
    }
}
