#ifndef TEXTUREUTILITIES_H
#define TEXTUREUTILITIES_H

#include "buildsettings.h"
#include "containers.h"
#include "MathUtils.h"
#include "memoryutilities.h"

namespace FlexKit
{
	/************************************************************************************************/


	enum class DeviceFormat
	{
		R8_UINT,
		R16_FLOAT,
		R16_UINT,
		R16G16_UINT,
		R32_UINT,
		R32G32_UINT,
		R8G8B8A_UINT,
		R8G8B8A8_UINT,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R16G16B16A16_UNORM,
		R8G8_UNORM,
		D24_UNORM_S8_UINT,
		R32_FLOAT,
		D32_FLOAT,

		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,

		R16G16_FLOAT,
		R16G16B16A16_FLOAT,
		R32G32_FLOAT,
		R32G32B32_FLOAT,
		R32G32B32A32_FLOAT,
		BC1_TYPELESS,
		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_TYPELESS,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_TYPELESS,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_TYPELESS,
		BC4_UNORM,
		BC4_SNORM,
		BC5_TYPELESS,
		BC5_UNORM,
		BC5_SNORM,
		BC7_UNORM,
		BC7_SNORM,


		R32G32B32_UINT,
		R32G32B32A32_UINT,

		UNKNOWN
	};


	/************************************************************************************************/


	class TextureBuffer
	{
	public:
		TextureBuffer() = default;

		TextureBuffer(uint2 IN_WH, size_t IN_elementSize, iAllocator* IN_Memory);
		TextureBuffer(uint2 IN_WH, size_t IN_elementSize, size_t BufferSize, iAllocator* IN_Memory);
		TextureBuffer(uint2 IN_WH, byte* buffer, size_t IN_elementSize);
		TextureBuffer(uint2 IN_WH, byte* buffer, size_t bufferSize, size_t IN_elementSize, iAllocator* allocator);

		~TextureBuffer();

		// Copy Operators
		TextureBuffer(const TextureBuffer& rhs);

		TextureBuffer& operator =(const TextureBuffer& rhs);

		// Move Operators
		TextureBuffer(TextureBuffer&& rhs) noexcept;
		TextureBuffer& operator =(TextureBuffer&& rhs) noexcept;

		void Release();

		operator byte* ()	        { return Buffer; }
		size_t BufferSize() const   { return Size; }


		byte*		Buffer			= nullptr;
		uint2		WH				= { 0, 0 };
		size_t		Size			= 0;
		size_t		ElementSize		= 0;
		iAllocator* Memory			= nullptr;
	};


	/************************************************************************************************/


	template<typename TY>
	struct TextureBufferView
	{
		using value_type = TY;

		TextureBufferView(TextureBuffer& Buffer) :
			Texture		{ Buffer                    },
			rowPitch	{ Buffer.WH[0] * sizeof(TY) } {}

		TextureBufferView(TextureBuffer& Buffer, const size_t IN_rowPitch) :
			Texture		{ Buffer        },
			rowPitch	{ IN_rowPitch   } {}

		TY& operator [](uint2 XY)
		{
			byte* row = Texture.Buffer + rowPitch * XY[1];
			return ((TY*)row)[XY[0]];
		}

		TY operator [](uint2 XY) const
		{
			byte* row = Texture.Buffer + rowPitch * XY[1];
			return ((TY*)row)[XY[0]];
		}

		operator byte* ()	{ return Texture.Buffer;    }
		size_t BufferSize()	{ return Texture.Size;      }

		const TextureBuffer& Texture;
		const size_t rowPitch;
	};


	/************************************************************************************************/


	struct WRAP_DIMENSIONS 
	{
		template<typename TY_SampleUnit>
		static uint2 Map(uint2 cordinate, uint2 window)
		{
			FK_ASSERT(0);
			return { 0, 0 };   
		}
	};

	struct STRETCH_DIMENSIONS
	{
		template<typename TY_SampleUnit>
		static uint2 Map(uint2 cordinate, uint2 window)
		{
			FK_ASSERT(0);
			return { 0, 0 };   
		}
	};

	template<typename TY_SampleType = float3, typename TY_EDGEHANDLER = WRAP_DIMENSIONS, size_t BLURSIZE = 3>
	class BlurredTextureSampler
	{
		BlurredTextureSampler(uint2 IN_sampleWindow) : 
			sampleWindow{ IN_sampleWindow }{}

		template<typename TY_BufferView>
		TY_SampleType operator() (const TY_BufferView& view, const float2 UV)
		{
			float           blurKernel[BLURSIZE] = {0.2, 0.2, 0.2, 0.2, 0.2};
			TY_SampleType   sample = { 0 };

			for(size_t Y = 0; Y < 5; Y++)
				for(size_t X = 0; X < 5; X++)
					sample += blurKernel[X] * blurKernel[Y] * view[TY_EDGEHANDLER::MAP({X, Y}, sampleWindow)];

			return sample;
		}

		const uint2 sampleWindow;
	};



	/************************************************************************************************/


	struct RGB
	{
		byte Red;
		byte Green;
		byte Blue;
		byte Reserved;
	};


	struct RGBA
	{
		byte Red;
		byte Green;
		byte Blue;
		byte Reserved;

		RGBA& operator = (const RGB& rhs) noexcept
		{
			Red		= rhs.Red;
			Green	= rhs.Green;
			Blue	= rhs.Blue;

			return *this;
		}
	};


	/************************************************************************************************/


	bool LoadBMP(const char* File, iAllocator* Memory, TextureBuffer* Out);


	inline bool CheckerBoard(const int TileXSize, const int TileYSize, TextureBufferView<Vect<4, uint8_t>>& view)
	{
		FK_ASSERT(TileXSize > 0);
		FK_ASSERT(TileYSize > 0);

		const auto WH = view.Texture.WH;

		for (size_t y = 0; y < WH[1]; ++y)
		{
			for (size_t x = 0; x < WH[0]; ++x)
			{
				uint8_t val = ((x / TileXSize + (y / TileYSize % 2)) % 2) * 255;

				view[{x, y}] = { val, val, val };
			}
		}

		return true;
	}


	inline Pair<uint16_t, uint16_t> GetMinMax(TextureBuffer& sourceMap);


	/************************************************************************************************/


	template<typename TY_VIEW = TextureBufferView<Vect<4, uint8_t>>>
	auto AverageSampler (const TY_VIEW& view, uint2 xy) noexcept
	{
		auto Sample1 = view[xy + uint2{ 0, 0 }] / 4;
		auto Sample2 = view[xy + uint2{ 0, 1 }] / 4;
		auto Sample3 = view[xy + uint2{ 1, 0 }] / 4;
		auto Sample4 = view[xy + uint2{ 1, 1 }] / 4;

		return (
			Sample1 +
			Sample2 +
			Sample3 +
			Sample4);
	}


	template<typename TY_sample = Vect<4, uint8_t>>
	auto TestSamples(
		TY_sample Sample1,
		TY_sample Sample2,
		TY_sample Sample3,
		TY_sample Sample4) noexcept
	{
		return TY_sample(0, 0, 0, 0);
	}


	template<typename TY_FORMAT = Vect<4, uint8_t>, bool AlignedRows = true>
	TextureBuffer BuildMipMap(TextureBuffer& sourceMap, iAllocator* memory, auto sampler)
	{
		const size_t	elementSize = sizeof(TY_FORMAT);
		const uint2		WH = sourceMap.WH / 2;

		auto GetRowPitch = [&]
		{
			if constexpr (AlignedRows)
			{
				const auto offset = WH[0] * sizeof(TY_FORMAT) % 256;

				return (offset == 0) ? WH[0] * elementSize : WH[0] * elementSize + (256 - offset);
			}
			else
				return  WH[0] * sizeof(TY_FORMAT);
		};

		size_t					RowPitch	= GetRowPitch();
		TextureBuffer			NewMIP		= TextureBuffer(WH, sizeof(TY_FORMAT), WH[1] * RowPitch, memory);
		TextureBufferView		DestiView	= TextureBufferView<TY_FORMAT>(NewMIP, RowPitch);
		const TextureBufferView	InputView	= TextureBufferView<TY_FORMAT>(sourceMap);

		for (uint32_t Y = 0; Y < WH[1]; Y++)
		{
			for (uint32_t X = 0; X < WH[0]; X++)
			{
				const uint2 in_Cord     = { X * 2, Y * 2 };
				const uint2 out_Cord    = { X, Y };

				auto newSample = sampler(InputView, in_Cord);
				DestiView[out_Cord] = newSample;
			}
		}

		return NewMIP;
	}

	template<typename TY_FORMAT = Vect<4, uint8_t>>
	TextureBuffer BuildMipMap(TextureBuffer& sourceMap, iAllocator* memory)
	{
		return BuildMipMap(sourceMap, memory, AverageSampler<TextureBufferView<TY_FORMAT>>);
	}

	Vector<TextureBuffer> LoadHDR(const char* str, size_t MIPCount = 8, iAllocator* scratchSpace = SystemAllocator);

	bool IsDDS(DeviceFormat format);
}


/**********************************************************************

Copyright (c) 2019-2021 Robert May

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



#endif
