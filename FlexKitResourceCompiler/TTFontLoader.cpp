/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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
#include "TTFontLoader.h"
#include "..\coreutilities\type.h"
#include "..\coreutilities\memoryutilities.h"

typedef uint16_t TTF_USHORT;
typedef  int16_t TTF_SHORT;

typedef uint32_t  TTF_ULONG;
typedef  int32_t  TTF_LONG;

typedef int32_t TTF_FIXED;


struct TTF_Directory
{
	TTF_FIXED  Version;
	TTF_USHORT TableCount;
	TTF_USHORT SearchRange;
	TTF_USHORT EntrySelector;
	TTF_USHORT RangeShift;
};

inline float FixedToFloat(TTF_FIXED N)
{
	return float((N & 0xFF00)>> 8) +  float(N & 0x00FF) / 100.0f;
}

Pair<bool, TTFont*> LoadTTFFile(const char* File, iAllocator* Memory)
{
	auto FileSize = FlexKit::GetFileSize(File);
	byte* Buffer = (byte*)Memory->_aligned_malloc(FileSize + 1);

	LoadFileIntoBuffer(File, Buffer, FileSize);

	TTFont* Out = nullptr;
	TTF_Directory* FileDirectory = (TTF_Directory*)Buffer;

	return{ true, Out };
}