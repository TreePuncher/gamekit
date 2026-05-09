#include <RenderSystemInterface.hpp>

namespace FlexKit
{
#ifdef WIN32
	IRenderWindow* CreateWin32VKSurface(IRenderSystem& renderSystem, uint2 WH, DeviceFormat format);

	void vkWin32UpdateInput();
#endif

	//IRenderWindow* CreateWaylandSurface(IRenderSystem&, uint2 WH, DeviceFormat);
	void ProcessEvents(IRenderWindow&);


}
