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


#include "GUIInputState.h"

namespace FlexKit
{
	bool GUIInputEventHandler_Helper(FrameworkState* StateMemory, Event evt)
	{
		InputState* ThisState = (InputState*)StateMemory;

		if (evt.InputSource == Event::Keyboard)
		{
			switch (evt.Action)
			{
			case Event::Pressed:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_TILDA: {
				}	break;
				case KC_BACKSPACE:
					if (ThisState->OnBackSpace)
						ThisState->OnBackSpace(ThisState);
					break;
				case KC_ENTER:
				{
					if (ThisState->OnEnter)
						ThisState->OnEnter(ThisState);
				}	break;
				case KC_ARROWUP:
				{
				}	break;
				case KC_ARROWDOWN:
				{

				}	break;
				case KC_SPACE:
				{
				}	break;
				default:
				{
					if ((evt.mData1.mKC[0] >= KC_A && evt.mData1.mKC[0] <= KC_Z) ||
						(evt.mData1.mKC[0] >= KC_0 && evt.mData1.mKC[0] <= KC_9) ||
						(evt.mData1.mKC[0] >= KC_SYMBOLS_BEGIN && evt.mData1.mKC[0] <= KC_SYMBOLS_END) ||
						(evt.mData1.mKC[0] == KC_PLUS) || (evt.mData1.mKC[0] == KC_MINUS) ||
						(evt.mData1.mKC[0] == KC_UNDERSCORE) || (evt.mData1.mKC[0] == KC_EQUAL) ||
						(evt.mData1.mKC[0] == KC_SYMBOL))
					{
						if (ThisState->OnChar)
							ThisState->OnChar(ThisState, (char)evt.mData2.mINT[0]);
					}
				}	break;
				}
			}	break;
			}
		}
		return false;
	}


	InputState*	CreateTextBoxInputState(GameFramework* framework, GUITextBoxHandle Handle, iAllocator* Memory, size_t BufferSize)
	{
		InputState* State = &Memory->allocate_aligned<InputState>();

		State->VTable.EventHandler   = GUIInputEventHandler_Helper;
		State->VTable.PostDrawUpdate = nullptr;
		State->VTable.PreDrawUpdate  = nullptr;
		State->VTable.Release        = nullptr;
		State->VTable.Update         = nullptr;

		return State;
	}

}// Namespace FlexKit