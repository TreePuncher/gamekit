/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#include "ImageUtilities.h"


namespace FlexKit
{

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


	bool CheckerBoard(int XSize, int YSize, TextureBuffer* Out)
	{
		FK_ASSERT(Out);
		FK_ASSERT(XSize > 0);
		FK_ASSERT(YSize > 0);

		const size_t Width = Out->WH[0];
		const size_t Height= Out->WH[1];

		RGBA* Pixels = (RGBA*)Out->Buffer;
		for(size_t y = 0; y < Height; ++y)
			for (size_t x = 0; x < Width; ++x) {
				int Val = ((x/XSize + (y/YSize % 2)) % 2) * 255;
				Pixels[x + y * Width].Blue	= Val;
				Pixels[x + y * Width].Green	= Val;
				Pixels[x + y * Width].Red	= Val;
			}

		return true;
	}

}