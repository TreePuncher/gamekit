/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "stdafx.h"
#include "ConsoleSubState.h"


namespace FlexKit
{	/************************************************************************************************/


	bool ConsoleEventHandler(FrameworkState* StateMemory, Event evt)
	{
		ConsoleSubState* ThisState = (ConsoleSubState*)StateMemory;

		if (evt.InputSource == Event::Keyboard)
		{
			switch (evt.Action)
			{
			case Event::Pressed:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_TILDA: {
					std::cout << "Popping Console State\n";
					PopSubState(ThisState->Framework);
					return false;
				}	break;
				case KC_BACKSPACE:
					BackSpaceConsole(&ThisState->Framework->Console);
					break;
				case KC_ENTER:
				{
					ThisState->RecallIndex = 0;
					EnterLineConsole(&ThisState->Framework->Console);
				}	break;
				case KC_ARROWUP:
				{
					if(ThisState->C->CommandHistory.size()){
						auto line = ThisState->C->CommandHistory[ThisState->RecallIndex].Str;
						auto LineSize = strlen(line);

						strcpy_s(ThisState->C->InputBuffer, ThisState->C->CommandHistory[ThisState->RecallIndex]);

						ThisState->C->InputBufferSize = LineSize;
						ThisState->IncrementRecallIndex();
					}
				}	break;
				case KC_ARROWDOWN:
				{
					if (ThisState->C->CommandHistory.size()) {
						auto line = ThisState->C->CommandHistory[ThisState->RecallIndex].Str;
						auto LineSize = strlen(line);

						strcpy_s(ThisState->C->InputBuffer, ThisState->C->CommandHistory[ThisState->RecallIndex]);

						ThisState->C->InputBufferSize = LineSize;
						ThisState->DecrementRecallIndex();
					}
				}	break;
				case KC_SPACE:
				{
					InputConsole(&ThisState->Framework->Console, ' ');
				}	break;
				default:
				{
					if ((evt.mData1.mKC[0] >= KC_A && evt.mData1.mKC[0] <= KC_Z ) || 
						(evt.mData1.mKC[0] >= KC_0 && evt.mData1.mKC[0] <= KC_9)  ||
						(evt.mData1.mKC[0] >= KC_SYMBOLS_BEGIN && evt.mData1.mKC[0] <= KC_SYMBOLS_END ) || 
						(evt.mData1.mKC[0] == KC_PLUS) || (evt.mData1.mKC[0] == KC_MINUS) ||
						(evt.mData1.mKC[0] == KC_UNDERSCORE) || (evt.mData1.mKC[0] == KC_EQUAL) ||
						(evt.mData1.mKC[0] == KC_SYMBOL ))
						InputConsole(&ThisState->Framework->Console, (char)evt.mData2.mINT[0]);

				}	break;
				}
			}	break;
			}
		}
		return false;
	}


	/************************************************************************************************/


	bool UpdateConsoleScreen(FrameworkState* StateMemory, EngineMemory*, double DT)
	{
		ConsoleSubState* ThisState = (ConsoleSubState*)StateMemory;
	
		return !ThisState->PauseBackgroundLogic;
	}


	/************************************************************************************************/


	bool DrawConsoleScreen(FrameworkState* StateMemory, EngineMemory* Engine, double DT)
	{
		ConsoleSubState* ThisState = (ConsoleSubState*)StateMemory;
	
		FlexKit::Draw_RECT Rect;
		Rect.BLeft	= { 0, 0.5 };
		Rect.TRight = { 1, 1 };
		Rect.Color  = float4(Grey(0.0f), 0.5f);
		PushRect(&ThisState->Framework->Immediate, Rect);

		DrawConsole(ThisState->C, ThisState->Framework->Immediate, GetWindowWH(Engine));

		return false;
	}


	/************************************************************************************************/


	void ReleaseConsoleState(FrameworkState* StateMemory)
	{
		ConsoleSubState* ThisState = (ConsoleSubState*)StateMemory;
		ThisState->Framework->ConsoleActive = false;
	}


	/************************************************************************************************/


	ConsoleSubState* CreateConsoleSubState(GameFramework* Framework)
	{
		ConsoleSubState* out;
		out = &Framework->Engine->BlockAllocator.allocate_aligned<ConsoleSubState>();
	
		out->VTable.PreDrawUpdate = DrawConsoleScreen;
		out->VTable.Update		  = UpdateConsoleScreen;
		out->VTable.EventHandler  = ConsoleEventHandler;
		out->VTable.Release		  = ReleaseConsoleState;

		out->C                    = &Framework->Console;
		out->Framework            = Framework;
		out->Engine               = Framework->Engine;
		out->PauseBackgroundLogic = true;

		return out;
	}


	/************************************************************************************************/
} // namespace FlexKit;