#include <BuildSettings.hpp>
#include "vkRenderDocDebug.hpp"
#include "renderdoc_app.h"


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

namespace VK_internal
{
	namespace RenderDocDebugUtils
	{
		RENDERDOC_API_1_1_2* rdoc_api = nullptr;

#ifdef WIN32
		bool ConnectWin32()
		{
			// At init, on windows
			if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
			{
				pRENDERDOC_GetAPI RENDERDOC_GetAPI =
					(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
				int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
				FK_ASSERT(ret == 1);

				FK_LOG_INFO("VK: RenderDoc connected!");

				return true;
			}

			return false;
		}
#endif

		bool Connect()
		{
#ifdef WIN32
			return ConnectWin32();
#endif
			rdoc_api = nullptr;
			return false;
		}

		bool Connected()
		{
			return rdoc_api != nullptr;
		}

		void BeginFrame()
		{
			if (rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);
		}

		void EndFrame()
		{
			if (rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);
		}
	}
}
