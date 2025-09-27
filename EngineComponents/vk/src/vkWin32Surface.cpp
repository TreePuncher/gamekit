#define WINDOWS_LEAN_AND_MEAN
#include "vkWin32Surface.hpp"
#include "windows.h"
#include "Events.hpp"
#include "timeapi.h"
#include <vulkan/vulkan.hpp>
#include <vkRenderSystem.hpp>

#pragma comment(lib, "Winmm.lib")

namespace FlexKit
{
    // Globals
	inline HWND			gWindowHandle = 0;
	inline HINSTANCE	gInstance = 0;
	inline uint2		internal_WH = {};

	LRESULT CALLBACK WindowProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		int Param = wParam;
		auto ShiftState = GetAsyncKeyState(VK_LSHIFT) | GetAsyncKeyState(VK_RSHIFT);

		EventNotifier<>* eventHandler;

		if(message == WM_CREATE)
		{
			CREATESTRUCT* CreateStruct = (CREATESTRUCT*)lParam;
			eventHandler = (EventNotifier<>*)CreateStruct->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)eventHandler);
			auto res = timeBeginPeriod(1);
		}

		eventHandler = (EventNotifier<>*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		switch( message )
		{
		case WM_SIZE:
		{
			FlexKit::Event ev;
			ev.mType			= Event::Internal;
			ev.InputSource		= Event::E_SystemEvent;
			ev.Action			= Event::InputAction::Resized;
			ev.mData1.mINT[0]	= (lParam & 0x000000000000ffff);		// Width
			ev.mData1.mINT[1]	= (lParam & 0x00000000ffff0000) >> 16;	// Heigth
			ev.mData2.mUser		= nullptr;

			eventHandler->NotifyEvent(ev);

			//internal_WH = { ev.mData1.mINT[0], ev.mData1.mINT[1] }; // TODO: Need to handle multiple render windows
		}
			break;
		case WM_PAINT:
			break;

		case WM_DESTROY:
		{
			PostQuitMessage( 0 );

			FlexKit::Event ev;
			ev.mType        = Event::Internal;
			ev.InputSource  = Event::E_SystemEvent;
			ev.Action       = Event::InputAction::Exit;

			eventHandler->NotifyEvent(ev);
		}	break;
		case WM_MOUSEMOVE:
		{
			FlexKit::Event ev;
			ev.mType = Event::Input;
			ev.InputSource = Event::Mouse;
			ev.Action = Event::InputAction::Moved;

			ev.mData1.mINT[0] = GET_X_LPARAM(lParam);
			ev.mData1.mINT[1] = GET_Y_LPARAM(lParam);

			eventHandler->NotifyEvent(ev);
		}	break;
		case WM_LBUTTONDOWN:
		{
			Event ev;
			ev.InputSource   = Event::Mouse;
			ev.mType		 = Event::Input;
			ev.Action		 = Event::InputAction::Pressed;

			ev.mData1.mKC[0] = KEYCODES::KC_MOUSELEFT;
			eventHandler->NotifyEvent(ev);
		}	break;
		case WM_LBUTTONUP:
		{
			Event ev;
			ev.InputSource	= Event::Mouse;
			ev.mType		= Event::Input;
			ev.Action		= Event::InputAction::Release;

			ev.mData1.mKC[0] = KEYCODES::KC_MOUSELEFT;

			eventHandler->NotifyEvent(ev);
		}	break;

		case WM_RBUTTONDOWN:
		{
			Event ev;
			ev.InputSource   = Event::Mouse;
			ev.mType         = Event::Input;
			ev.Action        = Event::InputAction::Pressed;
			ev.mData1.mKC[0] = KEYCODES::KC_MOUSERIGHT;

			eventHandler->NotifyEvent(ev);
		}	break;
		case WM_RBUTTONUP:
		{
			Event ev;
			ev.InputSource   = Event::Mouse;
			ev.mType         = Event::Input;
			ev.Action        = Event::InputAction::Release;
			ev.mData1.mKC[0] = KEYCODES::KC_MOUSERIGHT;

			eventHandler->NotifyEvent(ev);
		}	break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			Event ev;
			ev.mType       = Event::Input;
			ev.InputSource = Event::Keyboard;
			ev.Action      = message == WM_KEYUP ? Event::InputAction::Release : Event::InputAction::Pressed;

			switch (wParam)
			{
			case VK_F1:
				ev.mData1.mKC[0] = KC_F1;
				break;
			case VK_F2:
				ev.mData1.mKC[0] = KC_F2;
				break;
			case VK_F3:
				ev.mData1.mKC[0] = KC_F3;
				break;
			case VK_F4:
				ev.mData1.mKC[0] = KC_F4;
				break;
			case VK_F5:
				ev.mData1.mKC[0] = KC_F5;
				break;
			case VK_F6:
				ev.mData1.mKC[0] = KC_F6;
				break;
			case VK_F7:
				ev.mData1.mKC[0] = KC_F7;
				break;
			case VK_F8:
				ev.mData1.mKC[0] = KC_F8;
				break;
			case VK_F9:
				ev.mData1.mKC[0] = KC_F9;
				break;
			case VK_F10:
				ev.mData1.mKC[0] = KC_F10;
				break;
			case VK_BACK:
				ev.mData1.mKC[0] = KC_BACKSPACE;
				break;
			case VK_RETURN:
				ev.mData1.mKC[0] = KC_ENTER;
				break;
			case VK_SPACE:
				ev.mData1.mKC[0]   = KC_SPACE;
				break;
			case VK_ESCAPE:
				ev.mData1.mKC [0]  = KC_ESC;
				break;
			case VK_OEM_PLUS:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_EQUAL;
					Param = '=';
				} else {
					ev.mData1.mKC[0] = KC_PLUS;
					Param = '+';
				}
				break;
			case VK_OEM_MINUS:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_MINUS;
					Param = '-';
				} else {
					ev.mData1.mKC[0] = KC_UNDERSCORE;
					Param = '_';
				}
				break;
			case VK_OEM_7:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '\'';
				}
				else {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '\"';
				}	break;
			case VK_OEM_COMMA:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = ',';
				}
				else {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '<';
				}	break;
			case VK_UP:
				ev.mData1.mKC[0] = KC_ARROWUP;
				break;
			case VK_DOWN:
				ev.mData1.mKC[0] = KC_ARROWDOWN;
				break;
			case VK_LEFT:
				ev.mData1.mKC[0] = KC_ARROWLEFT;
				break;
			case VK_RIGHT:
				ev.mData1.mKC[0] = KC_ARROWRIGHT;
				break;
			case VK_OEM_PERIOD:
				if (!ShiftState) {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '.';
				}
				else {
					ev.mData1.mKC[0] = KC_SYMBOL;
					Param = '>';
				}	break;
				// 0 - 9
			case 0x30:
			case 0x31:
			case 0x32:
			case 0x33:
			case 0x34:
			case 0x35:
			case 0x36:
			case 0x37:
			case 0x38:
			case 0x39:
				if (ShiftState) {
					switch (wParam)
					{
					case 0x30:
						ev.mData1.mKC[0] = KC_RIGHTPAREN;
						Param = ')';
						break;
					case 0x31:
						ev.mData1.mKC[0] = KC_EXCLAMATION;
						Param = '!';
						break;
					case 0x32:
						ev.mData1.mKC[0] = KC_AT;
						Param = '@';
						break;
					case 0x33:
						ev.mData1.mKC[0] = KC_HASH;
						Param = '#';
						break;
					case 0x34:
						ev.mData1.mKC[0] = KC_DOLLAR;
						Param = '$';
						break;
					case 0x35:
						ev.mData1.mKC[0] = KC_PERCENT;
						Param = '%';
						break;
					case 0x36:
						ev.mData1.mKC[0] = KC_CHEVRON;
						Param = '^';
						break;
					case 0x37:
						ev.mData1.mKC[0] = KC_AMPERSAND;
						Param = '&';
						break;
					case 0x38:
						ev.mData1.mKC[0] = KC_STAR;
						Param = '*';
						break;
					case 0x39:
						ev.mData1.mKC[0] = KC_LEFTPAREN;
						Param = '(';
						break;
					default:
						break;
					}
				}
				else
				{
					ev.mData1.mKC[0]	= (FlexKit::KEYCODES)(KC_0 + wParam - 0x30);
					ev.mData1.mINT[2]	= wParam - 0x30;
				}
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
			case 'T':
			case 'U':
			case 'V':
			case 'W':
			case 'X':
			case 'Y':
			case 'Z':
				ev.mData1.mKC [0]  = wParam;
				if(!ShiftState)	Param += ('a' - 'A');
				break;
			case VK_OEM_3:
				if (ShiftState) {
					ev.mData1.mKC[0] = KC_TILDA;
				}
				break;
			case VK_CONTROL:
				ev.mData1.mKC[0] = KC_LEFTCTRL;
				break;
			default:

				break;
			}

			ev.mData2.mINT[0] = Param;
			eventHandler->NotifyEvent(ev);
		}
		}
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	void RegisterWindowClass( HINSTANCE hinst )
	{
		// Register Window Class
		WNDCLASSEXW wcex = {0};

		wcex.cbSize			= sizeof( wcex );
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= &WindowProcess;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hinst;
		wcex.hIcon			= LoadIcon( wcex.hInstance, IDI_APPLICATION );
		wcex.hCursor		= LoadCursor( nullptr, IDC_ARROW );
		wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW );
		wcex.lpszMenuName	= nullptr;
		wcex.lpszClassName	= L"RENDER_WINDOW";
		wcex.hIconSm		= LoadIcon( wcex.hInstance, IDI_APPLICATION );

		FK_ASSERT(RegisterClassExW( &wcex ));
	}


	struct vkRenderWindow : IRenderWindow
	{
	    virtual ~vkRenderWindow() {};

		virtual ResourceHandle      GetBackBuffer() const
		{
		    
		}

		virtual uint2               GetWH() const
		{
		    
		}

		virtual bool                Present(const uint32_t syncInternal = 0, const uint32_t flags = 0)
		{
		    
		}

		virtual void                Resize(const uint2 WH) = 0;

		virtual void				Release() = 0;

		operator ResourceHandle () { return GetBackBuffer(); }

		float2  GetPixelSize() const	{ return float2{ 1.0f, 1.0f } / GetWH(); }
		float   GetAspectRatio() const	{ const auto WH = GetWH(); return float(WH[0]) / float(WH[1]); }
	};


	IRenderWindow* CreateWin32VKSurface(IRenderSystem& renderSystem, uint2 WH, DeviceFormat format)
	{
		static bool _TEMP =
			[]
			{
				gWindowHandle = GetConsoleWindow();
				gInstance = GetModuleHandle(0);
				RegisterWindowClass(gInstance);
				SetProcessDPIAware();
				return true;
			}();

		auto& vkRS = static_cast<VK_internal::vkRenderSystem&>(renderSystem);

		VkInstance	instance	= vkRS.instance;
		VkDevice	device		= vkRS.device;

		auto windowHWND = CreateWindowW(L"RENDER_WINDOW", L"Render Window", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
								0,
								0,
			                    WH[0],
								WH[1],
								nullptr,
								nullptr,
								gInstance,
								new FlexKit::EventNotifier<>);

		ShowWindow(windowHWND, 5);
		VkWin32SurfaceCreateInfoKHR createSurfaceInfo{
			.sType		= VkStructureType::VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
	        .pNext		= nullptr,
	        .flags		= 0,
	        .hinstance	= gInstance,
	        .hwnd		= windowHWND
		};

		VkSurfaceKHR surface;
		if (auto res = vkCreateWin32SurfaceKHR(instance, &createSurfaceInfo, nullptr, &surface); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("Failed to create vulkan surface");
			return nullptr;
		}

		VkSwapchainCreateInfoKHR createSwapChainInfo{
		    .sType					= VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext					= 0,
            .flags					= 0,
            .surface				= surface,
            .minImageCount			= 3,
            .imageFormat			= VkFormat::VK_FORMAT_R8G8B8A8_UNORM,
            .imageColorSpace		= VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .imageExtent			= { .width = WH[0], .height = WH[1] },
            .imageArrayLayers		= 1,
            .imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount	= 0,
            .pQueueFamilyIndices	= nullptr,
            .preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode			= VK_PRESENT_MODE_FIFO_LATEST_READY_EXT,
            .clipped				= false,
            .oldSwapchain			= nullptr
		};

		VkSwapchainKHR swapchain = nullptr;
		if (auto res = vkCreateSwapchainKHR(device, &createSwapChainInfo, nullptr, &swapchain); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("Failed to create vulkan swapchain");
			return nullptr;
		}

		return nullptr;
	}

	void vkWin32UpdateInput()
	{
		MSG  msg;
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
