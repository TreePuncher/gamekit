#include <RenderSystemInterface.hpp>
#include <vulkan/vulkan.hpp>

namespace VK_internal
{
    using namespace FlexKit;

	IRenderWindow* CreateWin32Surface(VkInstance instance, VkDevice device, uint2 WH, DeviceFormat format);

	void vkWin32UpdateInput();
}
