
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

	Pair<uint16_t, uint16_t> GetMinMax(TextureBuffer& sourceMap)
	{
		using RBGA						= Vect<4, uint8_t>;
		TextureBufferView	view		= TextureBufferView<RBGA>(&sourceMap);
		const uint2			WH			= sourceMap.WH;

		uint16_t minY = 0;
		uint16_t maxY = 0;

		for (size_t Y = 0; Y < WH[0]; Y++)
		{
			for (size_t X = 0; X < WH[1]; X++) 
			{
				minY = min(view[{X, Y}][1], minY);
				maxY = max(view[{X, Y}][1], maxY);
			}
		}

		return { minY, maxY };
	};

	template<typename TY_sample = Vect<4, uint8_t>>
	auto AverageSampler (
		TY_sample Sample1, 
		TY_sample Sample2, 
		TY_sample Sample3, 
		TY_sample Sample4) noexcept
	{
		return (
			Sample1 +
			Sample2 +
			Sample3 +
			Sample4) / 4;
	}

	template<typename FN_Sampler = decltype(AverageSampler<>)>
	TextureBuffer BuildMipMap(TextureBuffer& sourceMap, iAllocator* memory, FN_Sampler sampler = AverageSampler<>)
	{
		using RBGA = Vect<4, uint8_t>;

		TextureBuffer		MIPMap	= TextureBuffer( sourceMap.WH / 2, sizeof(RGBA), memory );
		TextureBufferView	View	= TextureBufferView<RBGA>(&sourceMap);
		TextureBufferView	MipView = TextureBufferView<RBGA>(&MIPMap);

		const auto WH = MIPMap.WH;
		for (size_t Y = 0; Y < WH[0]; Y++)
		{
			for (size_t X = 0; X < WH[1]; X++)
			{
				uint2 in_Cord	= uint2{ min(X, WH[0] - 1) * 2, min(Y, WH[1] - 1) * 2 };
				uint2 out_Cord	= uint2{ min(X, WH[0] - 1), min(Y, WH[1] - 1) };

				MipView[out_Cord] = 
					sampler(
						View[in_Cord + uint2{ 0, 0 }],
						View[in_Cord + uint2{ 0, 1 }],
						View[in_Cord + uint2{ 1, 0 }],
						View[in_Cord + uint2{ 1, 1 }]);
			}
		}

		return MIPMap;
	}
}


#endif