#include <RenderSystemInterface.hpp>

namespace FlexKit
{
	IRenderWindow* CreateWin32VKSurface(IRenderSystem& renderSystem, uint2 WH, DeviceFormat format);

	void vkWin32UpdateInput();
}
