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

#include "ConsoleSubState.h"


void ConsoleEventHandler(SubState* StateMemory, Event evt)
{

}

bool UpdateConsoleScreen(SubState* StateMemory, EngineMemory*, double DT)
{
	ConsoleSubState* ThisState = (ConsoleSubState*)StateMemory;
	
	return !ThisState->PauseBackgroundLogic;
}

bool DrawConsoleScreen(SubState* StateMemory, EngineMemory*, double DT)
{
	ConsoleSubState* ThisState = (ConsoleSubState*)StateMemory;

	FlexKit::Draw_TEXT Text;
	Text.CLIPAREA_BLEFT		= { 0.0f, 0.5f };
	Text.CLIPAREA_TRIGHT	= { 1.0f, 1.0f };
	Text.Text               = &ThisState->C->TextArea;

	return true;
}

void Release(SubState* StateMemory)
{

}

ConsoleSubState* CreateConsoleSubState(EngineMemory* Engine, Console* C, BaseState* Base)
{
	ConsoleSubState* out;
	out = &Engine->BlockAllocator.allocate_aligned<ConsoleSubState>();
	
	out->VTable.PreDrawUpdate = DrawConsoleScreen;
	out->VTable.Update		  = UpdateConsoleScreen;
	out->VTable.EventHandler  = ConsoleEventHandler;
	out->C                    = C;
	out->Base                 = Base;
	out->Engine               = Engine;
	out->PauseBackgroundLogic = true;

	return out;
}