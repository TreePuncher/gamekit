#pragma once

#include "buildsettings.h"
#include "Events.h"
#include "Input.h"
#include "graphics.h"
#include "ProfilingUtilities.h"
#include <windows.h>

#pragma comment(lib, "Winmm.lib")

namespace FlexKit
{   /************************************************************************************************/


	struct Win32RenderWindow;

	// Globals
	inline HWND			        gWindowHandle   = 0;
	inline HINSTANCE		    gInstance       = 0;


	/************************************************************************************************/


	inline void UpdateInput()
	{
		MSG  msg;
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	/************************************************************************************************/

	
	FLEXKITAPI struct  Win32RenderWindow : public IRenderWindow
	{
		ResourceHandle GetBackBuffer() const override
		{
			return backBuffer;
		}


		bool Present(const uint32_t syncInternal = 0, const uint32_t flags = 0) override
		{
			ProfileFunction();

			if (!swapChain)
				return false;

			auto res = SUCCEEDED(swapChain->Present(syncInternal, flags | ((syncInternal == 0 && !fullscreen)? DXGI_PRESENT_ALLOW_TEARING : 0)));

			if (!res)
			{
				FK_LOG_ERROR("Failed to Present SwapChain! : Args { %u, %u }", syncInternal, flags);
				renderSystem->_OnCrash();
			}

			renderSystem->Textures.SetBufferedIdx(backBuffer, swapChain->GetCurrentBackBufferIndex());

			return res;
		}


		uint2 GetWH() const override
		{
			if (!swapChain)
				return { 0, 0 };
			else
				return WH;
		}


		void Resize(const uint2 newWH) override
		{
			if (!swapChain)
				return;

			WH = newWH;

			renderSystem->WaitforGPU();
			renderSystem->_ForceReleaseTexture(backBuffer);

			ID3D12Resource* before_ptr[3];
			swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)(before_ptr + 0));
			swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)(before_ptr + 1));
			swapChain->GetBuffer(2, __uuidof(ID3D12Resource), (void**)(before_ptr + 2));

			for (auto r : before_ptr)
				r->Release();

			try
			{
				const auto HR = swapChain->ResizeBuffers(0, WH[0], WH[1], DXGI_FORMAT_R16G16B16A16_FLOAT, flags);
				if (FAILED(HR))
					FK_ASSERT(0, "Failed to resize back buffer!");
			}
			catch (...) {}

			//recreateBackBuffer
			ID3D12Resource* buffer[3];

			DXGI_SWAP_CHAIN_DESC1 desc;
			swapChain->GetDesc1(&desc);
			desc.Width  = newWH[0];
			desc.Height = newWH[1];

			const auto bufferCount = desc.BufferCount;

			for (UINT I = 0; I < bufferCount; ++I)
			{
				swapChain->GetBuffer(I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
				if (!buffer[I])
					FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");

			}

			backBuffer = renderSystem->CreateGPUResource(
				GPUResourceDesc::BackBuffered(
					{ desc.Width, desc.Height },
					DeviceFormat::R16G16B16A16_FLOAT,
					buffer, 3));

			renderSystem->SetDebugName(backBuffer, "BackBuffer");
			renderSystem->Textures.SetBufferedIdx(backBuffer, swapChain->GetCurrentBackBufferIndex());
		}


		void Release()
		{
			if (!swapChain)
				return;
			else
			{
				swapChain->SetFullscreenState(false, nullptr);

				DestroyWindow(hWindow);

				swapChain->Release();
				ShowCursor(true);

				swapChain   = nullptr;
				hWindow     = 0;
			}
		}


		void ShowSystemCursor()
		{
			ShowCursor(true);
		}


		void HideSystemCursor()
		{
			ShowCursor(false);
		}


		bool isValid() const
		{
			return swapChain != nullptr;
		}

		void SetWindowTitle(const char* str)
		{
			SetWindowTextA(hWindow, str);
		}

		MouseInputState UpdateCapturedMouseInput(double dT)
		{
			const double updateFreq = 1.0f / 50.0f;

			if (mouseCapture)
			{
				T += dT;

				if (T < updateFreq)
					return mouseState;

				mouseState.dPos = GetMousedPos();
				mouseState.Position.x -= mouseState.dPos[0] / 2.0f;
				mouseState.Position.y += mouseState.dPos[1] / 2.0f;

				mouseState.Position.x = Max(0.0f, Min((float)mouseState.Position.x, (float)WH[0]));
				mouseState.Position.y = Max(0.0f, Min((float)mouseState.Position.y, (float)WH[1]));

				mouseState.NormalizedPos.x     = Max(0.0f, Min(float(mouseState.Position.x) / float(WH[0]), 1));
				mouseState.NormalizedPos.y     = Max(0.0f, Min(float(mouseState.Position.y) / float(WH[1]), 1));
				mouseState.NormalizedScreenCord = Position2SS(mouseState.NormalizedPos);

				const float HorizontalMouseMovement = float(mouseState.dPos[0]) / float(WH[0]);
				const float VerticalMouseMovement   = float(mouseState.dPos[1]) / float(WH[1]);

				mouseState.Normalized_dPos = { HorizontalMouseMovement, VerticalMouseMovement };
				SetSystemCursorToWindowCenter();

				T = 0.0f;

				return mouseState;
			}

			mouseState.Normalized_dPos = { 0, 0 };
			T = 0.0f;

			return mouseState;
		}


		void EnableCaptureMouse(bool enable)
		{
			if (enable) {
				mouseCapture = true;
				HideSystemCursor();
				SetSystemCursorToWindowCenter();
			}
			else
			{
				mouseCapture = false;
				ShowSystemCursor();
				SetSystemCursorToWindowCenter();
			}
		}

		void ToggleMouseCapture()
		{
			EnableCaptureMouse(!mouseCapture);
		}


		int2 GetMousedPos() const
		{
			auto POS        = WindowCenterPosition;
			POINT CENTER    = { (long)POS[0], (long)POS[1] };
			POINT P;

			GetCursorPos(&P);
			ScreenToClient(hWindow, &P);

			int2 dMouse = { CENTER.x - (int)P.x , CENTER.y - (int)P.y };
			return dMouse;
		}


		void SetSystemCursorToWindowCenter()
		{
			auto POS        = WindowCenterPosition;
			POINT CENTER    = { (long)POS[0], (long)POS[1] };
			POINT P;

			GetCursorPos(&P);
			ScreenToClient(hWindow, &P);

			ClientToScreen(hWindow, &CENTER);
			SetCursorPos(CENTER.x, CENTER.y);
		}


		IDXGISwapChain4* _GetSwapChain() const override
		{
			return swapChain;
		}

		void PIX_SetActiveWindow()
		{
			auto pix = renderSystem->pix;

			if (pix)	// if pix attached, set active pix target
				PIXSetTargetWindow(hWindow);
		}


		HWND						WindowHandle() { return hWindow; }


		RenderSystem*               renderSystem;
		IDXGISwapChain4*            swapChain;
		ResourceHandle              backBuffer;
		DXGI_FORMAT					Format;
		HWND						hWindow;
		UINT                        flags;

		uint2						WH; // Width-Height
		uint2						WindowCenterPosition;
		uint2                       LastMousePOS;
		Viewport					VP;

		bool						fullscreen = false;
		bool                        mouseCapture = false;
		double                      T = 0.0f;
		MouseInputState             mouseState;

		std::shared_ptr<EventNotifier<>> Handler = std::make_shared<EventNotifier<>>();

		WORD	InputBuffer[128];
		size_t	InputBuffSize;
	};


	/************************************************************************************************/


	inline uint2 internal_WH = {};

	inline LRESULT CALLBACK WindowProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
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
			ev.mType		  = Event::Internal;
			ev.InputSource	  = Event::E_SystemEvent;
			ev.Action		  = Event::InputAction::Resized;
			ev.mData1.mINT[0] = (lParam & 0x000000000000ffff);			// Width
			ev.mData2.mINT[0] = (lParam & 0x00000000ffff0000) >> 16;	// Heigth

			eventHandler->NotifyEvent(ev);

			internal_WH = { ev.mData1.mINT[0], ev.mData2.mINT[0] };
		}
			break;
		case WM_PAINT:
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );

			FlexKit::Event ev;
			ev.mType        = Event::Internal;
			ev.InputSource  = Event::E_SystemEvent;
			ev.Action       = Event::InputAction::Exit;

			eventHandler->NotifyEvent(ev);
			break;
		case WM_MOUSEMOVE:
		{
			/*
			FlexKit::Event ev;
			ev.mType = Event::Input;
			ev.InputSource = Event::Mouse;
			ev.Action = Event::InputAction::Moved;

			ev.mData1.mINT[0] = GET_X_LPARAM(lParam);
			ev.mData1.mINT[1] = GET_Y_LPARAM(lParam);

			size_t itr = 0;

			ev.mData2.mINT[0] = gLastMousePOS[0] - ev.mData1.mINT[0];
			ev.mData2.mINT[1] = gLastMousePOS[1] - ev.mData1.mINT[1];

			//gLastMousePOS = {ev.mData1.mINT[0], ev.mData1.mINT[1]};

			//gInputWindow->Handler.NotifyEvent(ev);
			*/
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


	/************************************************************************************************/


	inline void RegisterWindowClass( HINSTANCE hinst )
	{
		// Register Window Class
		WNDCLASSEX wcex = {0};

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

		FK_ASSERT( RegisterClassEx( &wcex ) );
	 }


	/************************************************************************************************/


	struct Win32RenderWindowDesc
	{
		bool		fullscreen;
		uint64_t	hInstance;
		uint64_t	hWindow;
		uint32_t	height;
		uint32_t	width;
		uint32_t	depth;
		uint32_t	AA_Count;
		uint32_t	AA_Quality;
		uint32_t	POS_X;
		uint32_t	POS_Y;
		char		ID[64];
	};


	inline Win32RenderWindowDesc DefaultWindowDesc(uint2 WH, bool fullscreen = false)
	{
		return {
			fullscreen,
			(uint64_t)gInstance,
			0,
			WH[1],
			WH[0],
			1,
			1,
			1,
			0,
			0,
			"RenderWindow"
		};
	}


	/************************************************************************************************/


	inline FLEXKITAPI std::optional<Win32RenderWindow> CreateWin32RenderWindow(RenderSystem& renderSystem, const Win32RenderWindowDesc& renderWindowDesc)
	{
		static bool _TEMP   =
			[]
			{
				gWindowHandle       = GetConsoleWindow();
				gInstance           = GetModuleHandle(0);
				RegisterWindowClass(gInstance);
				SetProcessDPIAware();
				return true;
			}();

		Win32RenderWindow renderWindow;
		static size_t Window_Count = 0;

		Window_Count++;

		// Register Window Class
		auto windowHWND = CreateWindow( L"RENDER_WINDOW", L"Render Window", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
								renderWindowDesc.POS_X,
								renderWindowDesc.POS_Y,
								renderWindowDesc.width,
								renderWindowDesc.height,
								nullptr,
								nullptr,
								gInstance,
								(void*)renderWindow.Handler.get());

		RECT ClientRect;
		RECT WindowRect;
		GetClientRect(windowHWND, &ClientRect);
		GetWindowRect(windowHWND, &WindowRect);
		MoveWindow(
			windowHWND,
			renderWindowDesc.POS_X, renderWindowDesc.POS_Y,
			WindowRect.right - WindowRect.left - ClientRect.right + renderWindowDesc.width,
			WindowRect.bottom - WindowRect.top - ClientRect.bottom + renderWindowDesc.height,
			false);

		POINT cursor;
		ScreenToClient(windowHWND, &cursor);
		POINT NewCursorPOS = {
		(LONG)( WindowRect.right - WindowRect.left - ClientRect.right + renderWindowDesc.width ) / 2,
		(LONG)( WindowRect.bottom - WindowRect.top - ClientRect.bottom + renderWindowDesc.height ) / 2 };

		renderWindow.WindowCenterPosition[ 0 ] = NewCursorPOS.x;
		renderWindow.WindowCenterPosition[ 1 ] = NewCursorPOS.y;
		ClientToScreen	(windowHWND, &NewCursorPOS );
		SetCursorPos	( NewCursorPOS.x, NewCursorPOS.y );
		GetCursorPos	( &cursor );

		renderWindow.LastMousePOS = uint2{ (uint32_t)cursor.x, (uint32_t)cursor.y };

		renderWindow.hWindow        = windowHWND;
		renderWindow.VP.Height	    = renderWindowDesc.height;
		renderWindow.VP.Width	    = renderWindowDesc.width;
		renderWindow.VP.X		    = 0;
		renderWindow.VP.Y		    = 0;
		renderWindow.VP.Max	        = 1.0f;
		renderWindow.VP.Min         = 0.0f;



		IDXGISwapChain1* NewSwapChain_ptr = nullptr;

		// Create Swap Chain
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Stereo			= false;
		SwapChainDesc.BufferCount		= 3;
		SwapChainDesc.Width				= renderWindowDesc.width;
		SwapChainDesc.Height			= renderWindowDesc.height;
		SwapChainDesc.Format			= DXGI_FORMAT_R16G16B16A16_FLOAT;
		SwapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count	= 1;
		SwapChainDesc.Flags				=
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH |
			(!renderWindowDesc.fullscreen ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

		HRESULT HR = renderSystem.pGIFactory->CreateSwapChainForHwnd(
			renderSystem.GraphicsQueue, windowHWND,
			&SwapChainDesc, nullptr, nullptr,
			&NewSwapChain_ptr);

		if (FAILED(HR))
		{
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE SWAP CHAIN!");
			return {};
		}

		renderWindow.swapChain  = static_cast<IDXGISwapChain4*>(NewSwapChain_ptr);
		renderWindow.flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		//CreateBackBuffer
		ID3D12Resource* buffer[3];

		if (renderWindowDesc.fullscreen)
			ShowWindow(windowHWND, 5);

		for (UINT I = 0; I < SwapChainDesc.BufferCount; ++I)
		{
			NewSwapChain_ptr->GetBuffer( I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
			if (!buffer[I]) {
				FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");
				return {};
			}
		}


		if(!renderWindowDesc.fullscreen)
			SetActiveWindow(windowHWND);

		renderWindow.WH             = { SwapChainDesc.Width, SwapChainDesc.Height };
		renderWindow.Format		    = SwapChainDesc.Format;
		renderWindow.renderSystem   = renderSystem;
		renderWindow.fullscreen		= renderWindowDesc.fullscreen;

		memset(renderWindow.InputBuffer, 0, sizeof(renderWindow.InputBuffer));

		if (renderWindowDesc.fullscreen)
		{
			auto res = NewSwapChain_ptr->SetFullscreenState(true, nullptr);

			for (auto& b : buffer)
				b->Release();

			NewSwapChain_ptr->ResizeBuffers(3, internal_WH[0], internal_WH[1], DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
			renderWindow.WH = internal_WH;

			for (UINT I = 0; I < SwapChainDesc.BufferCount; ++I)
			{
				NewSwapChain_ptr->GetBuffer(I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
				if (!buffer[I]) {
					FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");
					return {};
				}

			}
		}

		renderWindow.backBuffer = renderSystem.CreateGPUResource(
			GPUResourceDesc::BackBuffered(
				{ SwapChainDesc.Width, SwapChainDesc.Height },
				DeviceFormat::R16G16B16A16_FLOAT,
				buffer, 3));

		renderSystem.SetDebugName(renderWindow.backBuffer, "BackBuffer");
		renderSystem.Textures.SetBufferedIdx(renderWindow.backBuffer, renderWindow.swapChain->GetCurrentBackBufferIndex());

		return renderWindow;
	}


	/************************************************************************************************/


	inline FLEXKITAPI std::pair<Win32RenderWindow, bool> CreateWin32RenderWindowFromHWND (RenderSystem& renderSystem, HWND hwnd)
	{
		Win32RenderWindow renderWindow;

		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Stereo			= false;
		SwapChainDesc.BufferCount		= 3;
		SwapChainDesc.Width				= 800;
		SwapChainDesc.Height			= 600;
		SwapChainDesc.Format			= DXGI_FORMAT_R16G16B16A16_FLOAT;
		SwapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count	= 1;
		SwapChainDesc.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		IDXGISwapChain1* NewSwapChain_ptr = nullptr;
		HRESULT HR = renderSystem.pGIFactory->CreateSwapChainForHwnd(
			renderSystem.GraphicsQueue, hwnd,
			&SwapChainDesc, nullptr, nullptr,
			&NewSwapChain_ptr);

		if (FAILED(HR))
		{
			std::cout << "Failed to Create Swap Chain!\n";
			FK_ASSERT(FAILED(HR), "FAILED TO CREATE SWAP CHAIN!");
			return { {}, false };
		}

		renderWindow.swapChain  = static_cast<IDXGISwapChain4*>(NewSwapChain_ptr);
		renderWindow.flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		//CreateBackBuffer
		ID3D12Resource* buffer[3];

		for (UINT I = 0; I < SwapChainDesc.BufferCount; ++I)
		{
			NewSwapChain_ptr->GetBuffer(I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
			if (!buffer[I]) {
				FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");
				return { {}, false };
			}

		}

		renderWindow.backBuffer = renderSystem.CreateGPUResource(
			GPUResourceDesc::BackBuffered(
				{ SwapChainDesc.Width, SwapChainDesc.Height },
				DeviceFormat::R16G16B16A16_FLOAT,
				buffer, 3));

		renderSystem.SetDebugName(renderWindow.backBuffer, "BackBuffer");
		renderSystem.Textures.SetBufferedIdx(renderWindow.backBuffer, renderWindow.swapChain->GetCurrentBackBufferIndex());

		renderWindow.WH             = { SwapChainDesc.Width, SwapChainDesc.Height };
		renderWindow.Format         = SwapChainDesc.Format;
		renderWindow.renderSystem   = renderSystem;
		renderWindow.fullscreen		= false;

		memset(renderWindow.InputBuffer, 0, sizeof(renderWindow.InputBuffer));

		return { renderWindow, true };
	}

}// Namespace FlexKit


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2022 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
