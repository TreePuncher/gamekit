#include "AndroidRenderWindow.hpp"
#include "game-activity/native_app_glue/android_native_app_glue.h"

#include <android/hardware_buffer.h>
#include <android/log.h>
#include <Logging.hpp>
#include <RenderSystemInterface.hpp>
#include <vkRenderSystem.hpp>
#include <vkSwapchain.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_android.h>


struct AndroidRenderWindow : public FlexKit::IRenderWindow
{
    using vkSwapchain   = VK_internal::vkSwapchain;
    using EventNotifier = FlexKit::EventNotifier<>;

    AndroidRenderWindow(VkSurfaceKHR surface, FlexKit::uint2 WH, FlexKit::DeviceFormat format)  : 
        swapchain{ surface, WH, format } {}
    
    FlexKit::ResourceHandle GetBackBuffer() const override;
	FlexKit::uint2          GetWH() const override;

	bool                    Present(const uint32_t syncInternal = 0, const uint32_t flags = 0) override;
    void                    Resize(const FlexKit::uint2 WH) override;

	void				    Release() override;

    VkSurfaceKHR            surface;
    vkSwapchain             swapchain;

    EventNotifier           eventHandler;

    struct android_app*     pApp;
};


FlexKit::ResourceHandle AndroidRenderWindow::GetBackBuffer() const
{
    return swapchain.Resource();
}


FlexKit::uint2 AndroidRenderWindow::GetWH() const
{
    return swapchain.GetWH();
}


bool AndroidRenderWindow::Present(const uint32_t syncInternal, const uint32_t flags)
{
    return swapchain.Present(syncInternal, flags);
}


void AndroidRenderWindow::Resize(const FlexKit::uint2 WH)
{

}


void AndroidRenderWindow::Release()
{

}


FlexKit::DeviceFormat AHardwareFormat2DeviceFormat(int32_t IN_format)
{
    switch(IN_format)
    {
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
            return FlexKit::DeviceFormat::R8G8B8A8_UNORM;
        case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
            return FlexKit::DeviceFormat::R8G8B8X8_UNORM;
        case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
            return FlexKit::DeviceFormat::R8G8B8_UNORM;
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
            return FlexKit::DeviceFormat::R5G6B5_UNORM;
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
            return FlexKit::DeviceFormat::R16G16B16A16_FLOAT;
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
            return FlexKit::DeviceFormat::R10G10B10A2_UNORM;
        case AHARDWAREBUFFER_FORMAT_BLOB:
            return FlexKit::DeviceFormat::UNKNOWN;
        case AHARDWAREBUFFER_FORMAT_D16_UNORM:
            return FlexKit::DeviceFormat::D16_UNORM;
        case AHARDWAREBUFFER_FORMAT_D24_UNORM:
            return FlexKit::DeviceFormat::D24_UNORM;
        case AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT:
            return FlexKit::DeviceFormat::D24_UNORM_S8_UINT;
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT:
            return FlexKit::DeviceFormat::D32_FLOAT;
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT:
            return FlexKit::DeviceFormat::D32_FLOAT_S8_UINT;
        case AHARDWAREBUFFER_FORMAT_S8_UINT:
            return FlexKit::DeviceFormat::S8_UINT;
        case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
            return FlexKit::DeviceFormat::Y8Cb8Cr8_420;
        case AHARDWAREBUFFER_FORMAT_YCbCr_P010:
            return FlexKit::DeviceFormat::YCbCr_P010;
        case AHARDWAREBUFFER_FORMAT_YCbCr_P210:
            return FlexKit::DeviceFormat::YCbCr_P210;
        case AHARDWAREBUFFER_FORMAT_R8_UNORM:
            return FlexKit::DeviceFormat::R8_UNORM;
        case AHARDWAREBUFFER_FORMAT_R16_UINT:
            return FlexKit::DeviceFormat::R16_UINT;
        case AHARDWAREBUFFER_FORMAT_R16G16_UINT:
            return FlexKit::DeviceFormat::R16G16_UINT;
        case AHARDWAREBUFFER_FORMAT_R10G10B10A10_UNORM:
            return FlexKit::DeviceFormat::R10G10B10A10_UNORM;
        default:
            return FlexKit::DeviceFormat::UNKNOWN;
    }
}


FlexKit::IRenderWindow* CreateAndroidRenderWindow(FlexKit::IRenderSystem& rendersystem, struct android_app* pApp) 
{
    FK_LOG_0("Creating Android Render Window!");
    EXITSCOPE(
        FK_LOG_0("Finished Creating Android Render Window!");
    );

    auto& VKRS = static_cast<VK_internal::vkRenderSystem&>(rendersystem);
    
    VkAndroidSurfaceCreateInfoKHR info{
        .sType  = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext  = nullptr,
        .flags  = 0,
        .window = pApp->window
    };
    
    auto logMessage = std::format("VK: surface handle: {}", (uint64_t)pApp->window);

    FK_LOG_0("VK: Creating AndroidSurfaceKHR!");
    FK_LOG_0(logMessage.c_str());

    VkSurfaceKHR surface;
    VkResult result = vkCreateAndroidSurfaceKHR(
        VKRS.instance,
        &info,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS) {
        FK_LOG_ERROR("VK: Failed to create Android Render Window");
        throw std::runtime_error("VK: Failed to create Android surface");
    }
    else
        FK_LOG_0("VK: Created AndroidSurfaceKHR!");
    
    int32_t height = ANativeWindow_getHeight(pApp->window);
    int32_t width  = ANativeWindow_getWidth(pApp->window);
    auto androidHardwareFormat = ANativeWindow_getFormat(pApp->window);

    FlexKit::uint2 WH{ abs(width), abs(height) };
    auto appFormat = AHardwareFormat2DeviceFormat(androidHardwareFormat);
    
    auto& renderWindow = VKRS.allocator->allocate<AndroidRenderWindow>(surface, WH, appFormat);
    
    return &renderWindow;
}


void Subscribe(IRenderWindow* iwindow, InputSubscriber& subscriber)
{
    AndroidRenderWindow* window = static_cast<AndroidRenderWindow*>(iwindow);
    window->eventHandler.Subscribe(subscriber);
}


void RenderWindowProcessMessages(FlexKit::IRenderWindow* renderWindow)
{
    while (true) 
    {
        // 0 is non-blocking.
        int timeout = 0;
        int events;
        android_poll_source *pSource;
        int result = ALooper_pollOnce(timeout, nullptr, &events,
                                        reinterpret_cast<void**>(&pSource));
        switch (result) {
            case ALOOPER_POLL_TIMEOUT:
                [[clang::fallthrough]];
            case ALOOPER_POLL_WAKE:
                return;
            case ALOOPER_EVENT_ERROR:
                __android_log_write(ANDROID_LOG_INFO, "FlexKit", "ProcessMessage: ALooper_pollOnce returned an error");
                return;
            case ALOOPER_POLL_CALLBACK:
                break;
            default:
            {
                InputEvent evt;
                evt.mType           = InputEvent::System;
                evt.InputSource     = InputEvent::E_SystemEvent;
                evt.mData1.mINT[0]  = result;

                if(renderWindow)
                {
                    AndroidRenderWindow* window = static_cast<AndroidRenderWindow*>(renderWindow);
                    window->eventHandler.NotifyEvent(evt);
                }
            }
        }
    }
}


void NotifyEnd(FlexKit::IRenderWindow* window)
{
    AndroidRenderWindow* aWindow = static_cast<AndroidRenderWindow*>(window);

    InputEvent evt;
    evt.mType           = InputEvent::System;
    evt.InputSource     = InputEvent::E_SystemEvent;
    evt.Action          = InputEvent::Exit;
    aWindow->eventHandler.NotifyEvent(evt);
}