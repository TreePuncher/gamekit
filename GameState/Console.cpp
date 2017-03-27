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

#include "Console.h"

void InitateConsole(Console* Out, FontAsset* Font, EngineMemory* Engine)
{
	Out->Lines.clear();
	Out->Memory          = Engine->BlockAllocator;
	Out->Font            = Font;
	Out->InputBufferSize = 0;

	memset(Out->InputBuffer, '\0', sizeof(Out->InputBuffer));
}


void ReleaseConsole(Console* out)
{
	//ConsolePrintf(out, 1, 2, 3, 4);
}


void DrawConsole(Console* C, ImmediateRender* IR, uint2 Window_WH)
{
	const float LineHeight = (float(C->Font->FontSize[1]) / Window_WH[1]) / 4;
	const float AspectRatio = float(Window_WH[0]) / float(Window_WH[1]);
	size_t itr = 0;

	float y = 1.0f - float(1 + (itr)) * LineHeight;
	PrintText(IR, C->InputBuffer, C->Font, { 0, y }, float2(1.0f, 1.0f) - float2(0.0f, y), float4(WHITE, 1.0f), { 0.5f / AspectRatio, 0.5f });


	for (auto Line : C->Lines) {
		float y = 1.0f - float(2 + (itr)) * LineHeight ;

		if (y > 0) {
			float2 Position(0.0f, y);

			PrintText(IR, Line, C->Font, Position, float2(1.0f, 1.0f) - Position, float4(WHITE, 1.0f), { 0.5f / AspectRatio, 0.5f });
			itr++;
		}
		else
			break;
	}
}


void InputConsole(Console* C, char InputCharacter)
{
	C->InputBuffer[C->InputBufferSize++] = InputCharacter;
}


void EnterLineConsole(Console* C)
{
	C->InputBuffer[C->InputBufferSize++] = '\0';
	char* str = (char*)C->Memory->malloc(C->InputBufferSize);
	strcpy(str, C->InputBuffer);

	ConsolePrint(C, str);

	C->InputBufferSize = 0;
	memset(C->InputBuffer, '\0', sizeof(C->InputBuffer));
}


void ConsolePrint(Console* out, const char* _ptr)
{
	out->Lines.push_back(_ptr);
}