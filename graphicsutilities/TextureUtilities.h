
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
#include "..\coreutilities\MathUtils.h"

#ifndef TEXTUREUTILITIES_H
#define TEXTUREUTILITIES_H

namespace FlexKit
{
	/************************************************************************************************/


	class TextureBuffer
	{
	public:
		TextureBuffer() {}

		TextureBuffer(uint2 IN_WH, size_t IN_elementSize, iAllocator* IN_Memory) : 
			Buffer		{ (byte*)IN_Memory->malloc(IN_WH.Product() * IN_elementSize) },
			WH			{ IN_WH },
			ElementSize	{ IN_elementSize },
			Memory		{ IN_Memory },
			Size		{ IN_WH.Product() * IN_elementSize }
		{
		}

		byte*		Buffer;
		uint2		WH;
		size_t		Size;
		size_t		ElementSize;
		iAllocator* Memory;

		void Release()
		{
			Memory->_aligned_free(Buffer);
		}
	};


	/************************************************************************************************/


	template<typename TY>
	struct TextureBufferView
	{
		TextureBufferView(TextureBuffer* Buffer) : Texture(Buffer) {}

		TY& operator [](uint2 XY)
		{
			TY* buffer = (TY*)Texture->Buffer;
			return buffer[Texture->WH[1] * XY[1] + XY[0]];
		}

		TextureBuffer* Texture;
	};


	/************************************************************************************************/


	struct RGBA
	{
		byte Red;
		byte Green;
		byte Blue;
		byte Reserved;
	};


	FLEXKITAPI bool LoadBMP			( const char* File, iAllocator* Memory, TextureBuffer* Out );
	FLEXKITAPI bool CheckerBoard	( int XSize, int YSize, TextureBuffer* Out );
}


#endif