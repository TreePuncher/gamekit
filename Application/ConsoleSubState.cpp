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


	bool  ConsoleSubState::Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		return !PauseBackgroundLogic;
	}


	/************************************************************************************************/


	bool  ConsoleSubState::DebugDraw(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		return false;
	}

	/************************************************************************************************/


	bool  ConsoleSubState::Draw(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
	{
		DrawConsole(Console, Graph, GetCurrentBackBuffer(&Core->Window), Engine->GetTempMemory());

		return false;
	}


	/************************************************************************************************/


	bool  ConsoleSubState::EventHandler(Event evt)
	{
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
					PopSubState(Framework);
					return false;
				}	break;
				case KC_BACKSPACE:
					BackSpaceConsole(&Framework->Console);
					break;
				case KC_ENTER:
				{
					RecallIndex = 0;
					EnterLineConsole(&Framework->Console, Framework->Core->GetTempMemory());
				}	break;
				case KC_ARROWUP:
				{
					if(Console->CommandHistory.size()){
						auto line	  = Console->CommandHistory[RecallIndex].Str;
						auto LineSize = strlen(line);

						strcpy_s(Console->InputBuffer, Console->CommandHistory[RecallIndex]);

						Console->InputBufferSize = LineSize;
						IncrementRecallIndex();
					}
				}	break;
				case KC_ARROWDOWN:
				{
					if (Console->CommandHistory.size()) {
						auto line		= Console->CommandHistory[RecallIndex].Str;
						auto LineSize	= strlen(line);

						strcpy_s(Console->InputBuffer, Console->CommandHistory[RecallIndex]);

						Console->InputBufferSize = LineSize;
						DecrementRecallIndex();
					}
				}	break;
				case KC_SPACE:
				{
					InputConsole(&Framework->Console, ' ');
				}	break;
				default:
				{
					if ((evt.mData1.mKC[0] >= KC_A && evt.mData1.mKC[0] <= KC_Z ) || 
						(evt.mData1.mKC[0] >= KC_0 && evt.mData1.mKC[0] <= KC_9)  ||
						(evt.mData1.mKC[0] >= KC_SYMBOLS_BEGIN && evt.mData1.mKC[0] <= KC_SYMBOLS_END ) || 
						(evt.mData1.mKC[0] == KC_PLUS) || (evt.mData1.mKC[0] == KC_MINUS) ||
						(evt.mData1.mKC[0] == KC_UNDERSCORE) || (evt.mData1.mKC[0] == KC_EQUAL) ||
						(evt.mData1.mKC[0] == KC_SYMBOL ))
						InputConsole(&Framework->Console, (char)evt.mData2.mINT[0]);

				}	break;
				}
			}	break;
			}
		}
		return false;
	}


	/************************************************************************************************/


	void ConsoleSubState::IncrementRecallIndex()
	{
		RecallIndex = (RecallIndex + 1) % Console->CommandHistory.size();
	}


	/************************************************************************************************/


	void ConsoleSubState::DecrementRecallIndex()
	{
		RecallIndex = (Console->CommandHistory.size() + RecallIndex - 1) % Console->CommandHistory.size();
	}


	/************************************************************************************************/

} // namespace FlexKit;