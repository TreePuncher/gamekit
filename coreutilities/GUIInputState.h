/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef GUIINPUTSTATE_H
#define GUIINPUTSTATE_H

#include "..\Application\GameFramework.h"
#include "..\graphicsutilities\GuiUtilities.h"

namespace FlexKit
{
	struct InputState;

	struct ARROW_DIRECTION
	{
		enum : char
		{
			UP,
			DOWN,
			LEFT,
			RIGHT
		};
	};

	typedef char ARROW_DIR;

	typedef void (*InputStateCallback_ARROW)		(InputState* InputState, ARROW_DIR);
	typedef void (*InputStateCallback_CHARACTER)	(InputState* InputState, char c);
	typedef void (*InputStateCallback_KEY)			(InputState* InputState);

	struct InputState : public FlexKit::FrameworkState
	{
		InputStateCallback_ARROW		OnArrow		= nullptr;	
		InputStateCallback_CHARACTER	OnChar		= nullptr;
		InputStateCallback_KEY			OnEnter		= nullptr;
		InputStateCallback_KEY			OnBackSpace	= nullptr;

		void* USR;
	};

	InputState*	CreateTextBoxInputState(GameFramework* Framework, GUITextBoxHandle Handle, iAllocator* Memory, size_t BufferSize);

	bool GUIInputEventHandler_Helper(FrameworkState* StateMemory, Event evt);
}

#endif