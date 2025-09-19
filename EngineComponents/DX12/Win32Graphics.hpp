#pragma once

#include "BuildSettings.hpp"
#include "Events.hpp"
#include "Input.hpp"
#include "RenderSystemInterface.hpp"

struct IDXGISwapChain4;

namespace FlexKit
{   /************************************************************************************************/


	using Win32InputSubscriber = EventNotifier<>::Subscriber&;

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


	Win32RenderWindowDesc DefaultWindowDesc(uint2 WH, bool fullscreen = false);


	/************************************************************************************************/


	void Win32UpdateInput();


	IRenderWindow*	CreateWin32RenderWindow			(IRenderSystem& renderSystem, const Win32RenderWindowDesc& renderWindowDesc);
	IRenderWindow*	CreateWin32RenderWindowFromHWND	(IRenderSystem& renderSystem, uint64_t hwnd);


	float			GetDPIScaling(const IRenderWindow*);
	int2			GetMousedPos(const IRenderWindow*);

	void			HideSystemCursor(IRenderWindow*);
	void			ShowSystemCursor(IRenderWindow*);

	void			ToggleMouseCapture(IRenderWindow*);
	MouseInputState	UpdateCapturedMouseInput(double dT, IRenderWindow*);
	bool			IsMouseCaptured(IRenderWindow*);

	bool			isValid(const IRenderWindow*);

	void			SetWindowTitle(const char* str, IRenderWindow*);
	void			SetMouseCapture(bool enable, IRenderWindow*);
	void			SetSystemCursorToWindowCenter(IRenderWindow*);

	void			Subscribe(IRenderWindow*, Win32InputSubscriber& subscriber);

	IDXGISwapChain4*	INTERNAL_GetSwapChain(const IRenderWindow*);
	uint64_t			INTERNAL_WindowHandle(const IRenderWindow*);

	void PIX_INTERNAL_SetActiveWindow(IRenderWindow*);


	/************************************************************************************************/

}// Namespace FlexKit


/**********************************************************************

Copyright (c) 2022 - 2025 Robert May

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
