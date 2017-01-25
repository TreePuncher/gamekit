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

#include "PlayState.h"

//typedef void(*FN_EVENT_HANDLER) (SubState* StateMemory, Event);


/************************************************************************************************/


bool PlayUpdate(SubState* StateMemory, EngineMemory*, double DT)
{
	return false;
}


/************************************************************************************************/


bool PreDrawUpdate(SubState* StateMemory, EngineMemory*, double DT)
{
	return false;
}


/************************************************************************************************/


void Play_EventHandler(SubState* StateMemory, Event evt)
{
	if(evt.InputSource == Event::Keyboard)
		StateMemory->Base->Quit = true;
}


/************************************************************************************************/


PlayState* CreatePlayState(EngineMemory* Engine, BaseState* Base)
{
	PlayState* out = nullptr;

	out                       = &Engine->BlockAllocator.allocate_aligned<PlayState>();
	out->VTable.PreDrawUpdate = PreDrawUpdate;
	out->VTable.Update        = PlayUpdate;
	out->VTable.EventHandler  = Play_EventHandler;
	out->Base				  = Base;

	return out;
}