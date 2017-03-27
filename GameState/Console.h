#ifndef CONSOLE_H
#define CONSOLE_H

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


#include "..\buildsettings.h"	
#include "..\Application\GameMemory.h"
#include "..\graphicsutilities\Graphics.h"
#include "..\graphicsutilities\GuiUtilities.h"


struct Console
{
	FlexKit::CircularBuffer<const char*, 60>	Lines;
	FlexKit::FontAsset*							Font;

	char	InputBuffer[1024];
	size_t	InputBufferSize;

	iAllocator*	Memory;
};

void InitateConsole ( Console* out, FontAsset* Font, EngineMemory* Engine );	
void ReleaseConsole	( Console* out );

void DrawConsole	( Console* C, ImmediateRender* IR, uint2 Window_WH );

void InputConsole		( Console* C, char InputCharacter );
void EnterLineConsole	( Console* C);

void ConsolePrint	( Console* out, const char* _ptr );
void ConsolePrintf	( Console* out );

template<typename Ty, typename ... Ty_Args >
void ConsolePrintf(Console* out, const char* _ptr, Ty, Ty_Args ... Args )
{
	ConsolePrintf(out, Args...);
}

#endif