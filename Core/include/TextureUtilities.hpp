#pragma once

#include "BuildSettings.hpp"
#include "Containers.hpp"
#include "MathUtilities.hpp"
#include "MemoryUtilities.hpp"
#include <RenderSystemInterface.hpp>

namespace FlexKit
{	/************************************************************************************************/


	class TextureBuffer
	{
	public:
		TextureBuffer() = default;

		TextureBuffer(uint2 IN_WH, size_t IN_elementSize, iAllocator* IN_Memory);
		TextureBuffer(uint2 IN_WH, size_t IN_elementSize, size_t BufferSize, iAllocator* IN_Memory);
		TextureBuffer(uint2 IN_WH, std::byte* buffer, size_t IN_elementSize);
		TextureBuffer(uint2 IN_WH, std::byte* buffer, size_t bufferSize, size_t IN_elementSize, iAllocator* allocator);

		~TextureBuffer();

		// Copy Operators
		TextureBuffer(const TextureBuffer& rhs);

		TextureBuffer& operator =(const TextureBuffer& rhs);

		// Move Operators
		TextureBuffer(TextureBuffer&& rhs) noexcept;
		TextureBuffer& operator =(TextureBuffer&& rhs) noexcept;

		void Release();

		operator std::byte* () { return Buffer; }
		operator void* () { return Buffer; }
		size_t BufferSize() const   { return Size; }

		void Copy(std::byte* src, size_t size)	{ memcpy(Buffer, src, size); }

		std::byte*	Buffer			= nullptr;
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
			std::byte* row = Texture.Buffer + rowPitch * XY[1];
			return ((TY*)row)[XY[0]];
		}

		TY operator [](uint2 XY) const
		{
			std::byte* row = Texture.Buffer + rowPitch * XY[1];
			return ((TY*)row)[XY[0]];
		}

		operator std::byte* ()	{ return Texture.Buffer;    }
		size_t BufferSize()		{ return Texture.Size;      }

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
		std::byte Red;
		std::byte Green;
		std::byte Blue;
		std::byte Reserved;
	};


	struct RGBA
	{
		std::byte Red;
		std::byte Green;
		std::byte Blue;
		std::byte Alpha;

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
		using ViewType = TextureBufferView<Vect<4, uint8_t>>;
		using elementType = ViewType::value_type;

	    FK_ASSERT(TileXSize > 0);
		FK_ASSERT(TileYSize > 0);

		const auto WH = view.Texture.WH;

		for (uint32_t y = 0; y < WH[1]; ++y)
		{
			for (uint32_t x = 0; x < WH[0]; ++x)
			{
				uint8_t val = ((x / TileXSize + (y / TileYSize % 2)) % 2) * 255;

				view[uint2{ x, y }] = val;
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

	size_t BlockSize(DeviceFormat format);

	struct DDSLevelInfo
	{
		size_t  RowPitch = 0;
		uint2   WH;
		bool    tiled;
	};

	struct DDSInfo
	{
		uint8_t     MIPCount = 0;
		uint2       WH = { 0, 0 };

		DeviceFormat format;
	};

	DDSInfo			GetDDSInfo(AssetHandle asset, struct ReadContext& ctx);
	DDSLevelInfo	GetMIPLevelInfo(const size_t Level, const uint2 WH, const DeviceFormat format);

}


/**********************************************************************

Copyright (c) 2019-2025 Robert May

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

