/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#include "TextureUtilities.h"
#include "Assets.h"

#include <stb_image.h>

namespace FlexKit
{   /************************************************************************************************/


	TextureBuffer::TextureBuffer(uint2 IN_WH, size_t IN_elementSize, iAllocator* IN_Memory) :
		Buffer		{ (byte*)IN_Memory->_aligned_malloc(IN_WH.Product() * IN_elementSize)	},
		WH			{ IN_WH									},
		ElementSize	{ IN_elementSize						},
		Memory		{ IN_Memory								},
		Size		{ IN_WH.Product() * IN_elementSize		} {}


	TextureBuffer::TextureBuffer(uint2 IN_WH, size_t IN_elementSize, size_t BufferSize, iAllocator* IN_Memory) :
		Buffer		{ (byte*)IN_Memory->_aligned_malloc(BufferSize)	},
		WH			{ IN_WH											},
		ElementSize	{ IN_elementSize								},
		Memory		{ IN_Memory										},
		Size		{ BufferSize									} {}

	TextureBuffer::TextureBuffer(uint2 IN_WH, byte* buffer, size_t IN_elementSize) :
		Buffer		{ buffer								},
		WH			{ IN_WH									},
		ElementSize	{ IN_elementSize						},
		Size		{ IN_WH.Product() * IN_elementSize		} {}

	TextureBuffer::TextureBuffer(uint2 IN_WH, byte* buffer, size_t bufferSize, size_t IN_elementSize, iAllocator* allocator) :
		Buffer      { buffer			},
		WH          { IN_WH				},
		ElementSize { IN_elementSize	},
		Size        { bufferSize		},
		Memory      { allocator			} {}

	TextureBuffer::~TextureBuffer()
	{
		Release();
	}

	TextureBuffer::TextureBuffer(const TextureBuffer& rhs)
	{
		if (Buffer)
			Memory->_aligned_free(Buffer);

		if (!rhs.Memory)
			return;

		Memory			= rhs.Memory;
		Size            = rhs.Size;
		WH				= rhs.WH;
		ElementSize		= rhs.ElementSize;
		Buffer			= (byte*)Memory->_aligned_malloc(Size);

		memcpy(Buffer, rhs.Buffer, Size);
	}


	TextureBuffer& TextureBuffer::operator =(const TextureBuffer& rhs)
	{
		if (Buffer && Memory)
			Memory->_aligned_free(Buffer);

		Memory		= rhs.Memory;
		WH			= rhs.WH;
		Size		= rhs.Size;
		ElementSize = rhs.ElementSize;
		Buffer		= (byte*)Memory->_aligned_malloc(Size);

		memcpy(Buffer, rhs.Buffer, Size);

		return *this;
	}


	// Move Operators
	TextureBuffer::TextureBuffer(TextureBuffer&& rhs) noexcept
	{
		Buffer		= rhs.Buffer;
		WH			= rhs.WH;
		Size		= rhs.Size;
		ElementSize = rhs.ElementSize;
		Memory		= rhs.Memory;

		rhs.Buffer		= nullptr;
		rhs.WH			= { 0, 0 };
		rhs.Size		= 0;
		rhs.ElementSize	= 0;
		rhs.Memory		= nullptr;
	}


	TextureBuffer& TextureBuffer::operator =(TextureBuffer&& rhs) noexcept
	{
		Buffer		= rhs.Buffer;
		WH			= rhs.WH;
		Size		= rhs.Size;
		ElementSize = rhs.ElementSize;
		Memory		= rhs.Memory;

		rhs.Buffer		= nullptr;
		rhs.WH			= { 0, 0 };
		rhs.Size		= 0;
		rhs.ElementSize	= 0;
		rhs.Memory		= nullptr;

		return *this;
	}


	void TextureBuffer::Release()
	{
		if (!Memory)
			return;

		Memory->_aligned_free(Buffer);
		Buffer			= nullptr;
		WH				= { 0, 0 };
		Size			= 0;
		ElementSize		= 0;
		Memory			= nullptr;
	}


	/************************************************************************************************/


	bool LoadBMP(const char* File, iAllocator* Memory, TextureBuffer* Out)
	{
#pragma pack(push, 1)
		struct HeaderStruct
		{
			char		bfType[2];
			uint32_t	FileSize;
			uint32_t	Reserved;
			uint32_t	OffBits;
		}*Header = nullptr;

		struct InfoStruct
		{
			uint32_t InfoSize;
			uint32_t Width;
			uint32_t Height;
			uint16_t Planes;
			uint16_t BitCount;
			uint32_t Compression;
			uint32_t ImageSize;// ImageSize In bytes
			uint32_t XPixelsPerMeter;
			uint32_t YPixelsPerMeter;
			uint32_t ClrUsed;
			uint32_t ClrImportant; // If Zero, all bits used

		}*Info = nullptr;

#pragma pack(pop)
		struct RGBA
		{
			byte Red;
			byte Green;
			byte Blue;
			byte Reserved;
		};

		size_t FileSize = GetFileSize(File);

		byte* Buffer = (byte*)Memory->_aligned_malloc(FileSize + 1);
		if (LoadFileIntoBuffer(File, Buffer, FileSize + 1))
		{
			Header	= (HeaderStruct*)Buffer;
			Info	= (InfoStruct*)(Buffer + sizeof(HeaderStruct));


			const size_t PixelCount = Info->Width * Info->Height;
			RGBA* OutBuffer = (RGBA*)Memory->_aligned_malloc(PixelCount * sizeof(RGBA));
			size_t Width	= Info->Width;
			size_t Height	= Info->Height;

			switch (Info->BitCount)
			{
			case 8:
			{
				// Convert to RGBA
				// Slow Path
				byte *PixelsBGR = (byte*)(Buffer + Header->OffBits);

				for (size_t y = 0; y < Height; ++y)
					for (size_t x = 0; x < Width; ++x) {
						size_t I_Out = x + y * Info->Width;
						size_t I_In = x + (Info->Height - y - 1) * Info->Width;

						OutBuffer[I_Out].Red	= PixelsBGR[I_In];
						OutBuffer[I_Out].Blue	= PixelsBGR[I_In];
						OutBuffer[I_Out].Green	= PixelsBGR[I_In];
					}
			}	break;
			case 24:
				{
					// Convert to RGBA
					// Slow Path
					struct BGRA
					{
						byte Blue;
						byte Green;
						byte Red;
					}*PixelsBGR = (BGRA*)(Buffer + Header->OffBits);

					for (size_t y = 0; y < Height; ++y)
						for (size_t x = 0; x < Width; ++x) {
							size_t I_Out = x + y * Info->Width;
							size_t I_In = x + (Info->Height - y - 1) * Info->Width;

							OutBuffer[I_Out].Red	= PixelsBGR[I_In].Red;
							OutBuffer[I_Out].Blue	= PixelsBGR[I_In].Blue;
							OutBuffer[I_Out].Green	= PixelsBGR[I_In].Green;
						}
				}	break;
			case 32:
			{
				// Convert to RGBA
				// Slow Path
				struct BGRA
				{
					byte Blue;
					byte Green;
					byte Red;
					byte A;
				}*PixelsBGRA = (BGRA*)(Buffer + Header->OffBits);

				
				for (size_t y = 0; y < Height; ++y)
					for (size_t x = 0; x < Width; ++x){
						size_t I_Out	= x + y * Info->Width;
						size_t I_In		= x + (Info->Height - y - 1) * Info->Width;

						OutBuffer[I_Out].Red	= PixelsBGRA[I_In].Red;
						OutBuffer[I_Out].Blue	= PixelsBGRA[I_In].Blue;
						OutBuffer[I_Out].Green	= PixelsBGRA[I_In].Green;
				}
			}	break;

			}
			Out->ElementSize  = 4;
			Out->WH		      = { Info->Width, Info->Height };
			Out->Buffer       = (byte*)OutBuffer;
			Out->Size		  = PixelCount * sizeof(RGBA);
			Out->Memory       = Memory;
			return true;
		}

		return false;
	}


	/************************************************************************************************/


	Pair<uint16_t, uint16_t> GetMinMax(TextureBuffer& sourceMap)
	{
		using RBGA = Vect<4, uint8_t>;
		TextureBufferView	view	= TextureBufferView<RBGA>(sourceMap);
		const uint2			WH		= sourceMap.WH;

		uint16_t minY = 0;
		uint16_t maxY = 0;

		for (uint32_t Y = 0; Y < WH[0]; Y++)
		{
			for (uint32_t X = 0; X < WH[1]; X++)
			{
				minY = Min(view[{ X, Y }][1], minY);
				maxY = Max(view[{ X, Y }][1], maxY);
			}
		}

		return { minY, maxY };
	};


	/************************************************************************************************/


	Vector<TextureBuffer> LoadHDR(const char* str, size_t MIPCount, iAllocator* scratchSpace)
	{
		if (!stbi_is_hdr(str))
			return {};

		int width;
		int height;
		int channelCount;

		float* res = stbi_loadf(str, &width, &height, &channelCount, STBI_rgb);

		if (!res)
			return {};

		struct RGBA
		{
			float rgb[4];
		};

		struct RGB
		{
			float rgb[3];
		};

		Vector<TextureBuffer> MIPChain(scratchSpace);

		auto GetRowPitch = [&]
		{
			const size_t elementSize = sizeof(RGBA);
			return Max((width * elementSize) / 256, 1) * 256;
		};

		const size_t rowPitch = GetRowPitch();
		const size_t bufferSize = rowPitch * height;

		MIPChain.emplace_back(
			uint2{ (uint32_t)width, (uint32_t)height },
			(byte*)scratchSpace->_aligned_malloc(rowPitch * height, 256),
			bufferSize,
			sizeof(RGBA),
			scratchSpace);

		auto view = TextureBufferView<float4>(MIPChain.back(), rowPitch);
		RGB* rgb = (RGB*)res;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				memcpy(
					&view[{(uint32_t)x, (uint32_t)y}],
					&rgb[y * width + x],
					sizeof(RGB));
			}
		}

		for (size_t I = 1; I < MIPCount; I++)
			MIPChain.emplace_back(
				BuildMipMap<float4>(
					MIPChain.back(), scratchSpace, AverageSampler<decltype(view)>));

		free(res);

		return MIPChain;
	}


	/************************************************************************************************/


	bool IsDDS(DeviceFormat format)
	{
		switch (format)
		{
		case DeviceFormat::BC1_TYPELESS:
		case DeviceFormat::BC1_UNORM:
		case DeviceFormat::BC1_UNORM_SRGB:
		case DeviceFormat::BC2_TYPELESS:
		case DeviceFormat::BC2_UNORM:
		case DeviceFormat::BC2_UNORM_SRGB:
		case DeviceFormat::BC3_TYPELESS:
		case DeviceFormat::BC3_UNORM:
		case DeviceFormat::BC3_UNORM_SRGB:
		case DeviceFormat::BC4_TYPELESS:
		case DeviceFormat::BC4_UNORM:
		case DeviceFormat::BC4_SNORM:
		case DeviceFormat::BC5_TYPELESS:
		case DeviceFormat::BC5_UNORM:
		case DeviceFormat::BC5_SNORM:
		case DeviceFormat::BC7_UNORM:
		case DeviceFormat::BC7_SNORM:
			return true;
		default:
			return false;
		}
	}


}	/************************************************************************************************/
