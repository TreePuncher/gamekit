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

#include "stdafx.h"
#include "TTFontLoader.h"
#include "..\coreutilities\type.h"
#include "..\coreutilities\memoryutilities.h"

typedef uint16_t TTF_USHORT;
typedef  int16_t TTF_SHORT;

typedef uint32_t  TTF_ULONG;
typedef  int32_t  TTF_LONG;

typedef  int32_t TTF_FIXED;

typedef  int16_t TTF_FWORD;


struct TTF_DirectoryEntry
{
	union {
		char		Tag_C[4];
		TTF_ULONG	Tag_UL;
	};
	TTF_ULONG checkSum;
	TTF_ULONG offset;
	TTF_ULONG length;
};

struct TTF_Directory
{
	TTF_FIXED  Version;
	TTF_USHORT TableCount;
	TTF_USHORT SearchRange;
	TTF_USHORT EntrySelector;
	TTF_USHORT RangeShift;
};

TTF_ULONG
CalcTableChecksum(TTF_ULONG* Table, TTF_ULONG Length)
{
	TTF_ULONG	Sum	= 0L;
	TTF_ULONG*	End	= (TTF_ULONG*)(Table + ((Length + 3) & ~3) / sizeof(ULONG));

	while ((size_t)Table < (size_t)End)
		Sum += *Table++;
	return Sum;
}


inline float FixedToFloat(TTF_FIXED N)
{
	return float((N & 0xFF00)>> 8) +  float(N & 0x00FF) / 100.0f;
}

struct Format4Encoding
{
	TTF_USHORT Format;
	TTF_USHORT Length; // Byte Length
	TTF_USHORT Version;
	TTF_USHORT segCountX2;
	TTF_USHORT SearchRange;
	TTF_USHORT EntrySelector;
	TTF_USHORT RangeShift;
	TTF_USHORT EndCodes[];
};

struct CMAP_Entry
{
	TTF_USHORT PlatformID;
	TTF_USHORT EncodingID;
	TTF_ULONG  SubTableOffset;
};

struct CMAP
{
	USHORT Version;
	USHORT TableSize;
	CMAP_Entry	Table[];
};

struct Glyf
{
	TTF_SHORT NumberOfContours;
	TTF_FWORD xMin;
	TTF_FWORD yMin;
	TTF_FWORD xMax;
	TTF_FWORD yMax;
};

struct Head
{

};

struct Hhea
{

};

struct Loca
{

};

struct Maxp
{

};

struct Name
{
	 
};

struct Post
{

};

struct OS2
{

};

size_t GetGlyphCode(const TTF_USHORT* IDRangeOffsets, const TTF_USHORT* StartCodes, const TTF_USHORT* EndCodes, const size_t TableSize, char C = 'A')
{
	FK_ASSERT(IDRangeOffsets != StartCodes);

	size_t I = 0;
	for (; I < TableSize; ++I) {
		auto Start	= ConvertEndianness(StartCodes[I]);
		auto End	= ConvertEndianness(EndCodes[I]);

		if (End > C)
			break;
		auto x = 0;
	}

	USHORT RangeOffset	= ConvertEndianness(IDRangeOffsets[I])/2;
	USHORT StartCount	= ConvertEndianness(StartCodes[I]);
	size_t GlyphCode	= *(IDRangeOffsets + I + RangeOffset + (C - StartCount));

	return GlyphCode;
}

#define CONVERT(A) A = ConvertEndianness(A);

Pair<bool, TTFont*> LoadTTFFile(const char* File, iAllocator* Memory)
{
	auto FileSize = GetFileSize(File);
	byte* Buffer  = (byte*)Memory->_aligned_malloc(FileSize + 1);

	Buffer[FileSize] = 0xff;

	auto res = LoadFileIntoBuffer(File, Buffer, FileSize, false);

	TTFont* Out = nullptr;
	TTF_Directory* FileDirectory = (TTF_Directory*)Buffer;

	TTF_DirectoryEntry* Entries = (TTF_DirectoryEntry*)(Buffer + 12);

	auto temp = ConvertEndianness(0x0102);

	size_t end = ConvertEndianness(FileDirectory->TableCount);
	// Tables
	CMAP* CMap    = nullptr;
	Glyf* Glyfs   = nullptr;

	for (size_t I = 0; I < end; ++I)
	{
		auto Entry    = Entries[I];
		auto Length   = ConvertEndianness(Entries[I].length);
		auto CheckSum = CalcTableChecksum((TTF_ULONG*)(Entries + I), Length);

		switch (Entry.Tag_UL)
		{
		case 0x70616d63: // CMAP Tag
		{
			size_t Offset	= ConvertEndianness(Entries[I].offset);
			CMap		    = (CMAP*)(Buffer + Offset);

			/*
			size_t TableSize = ConvertEndianness(CMap->TableSize);
			for (size_t II = 0; II < TableSize; ++II)
			{
				auto PlatformID = ConvertEndianness(CMap->Table[II].PlatformID);
				auto EncodingID = ConvertEndianness(CMap->Table[II].EncodingID);
				auto TableOffset = ConvertEndianness(CMap->Table[II].SubTableOffset);

				if (PlatformID == 3)
				{
					switch (EncodingID) 
					{
					case 0x01: // Unicode Encoding
					{
						Format4Encoding* Table	= (Format4Encoding*)(Buffer + Offset);
						size_t TableByteLength	= ConvertEndianness(Table->Length);
						size_t SegCount			= ConvertEndianness(Table->segCountX2) / 2;
						USHORT* EndCodes		= Table->EndCodes;
						USHORT* StartCodes		= EndCodes + SegCount + 1;

						USHORT* IDDelta			= StartCodes		+ SegCount;
						USHORT* IDRangeOffsets	= IDDelta			+ SegCount;
						USHORT* GlyphIDs		= IDRangeOffsets	+ SegCount;

						auto GlyphCode_A = GetGlyphCode(IDRangeOffsets, StartCodes, EndCodes, 'A');
						auto GlyphCode_B = GetGlyphCode(IDRangeOffsets, StartCodes, EndCodes, 'B');
						auto GlyphCode_C = GetGlyphCode(IDRangeOffsets, StartCodes, EndCodes, 'C');

						int x = 0;
					}
					}
				}
				size_t C = 0;
			}
			*/
		}	break;
		case 0x66796c67:{
			size_t Offset = ConvertEndianness(Entries[I].offset);
			CONVERT(Entries[I].length);
			Glyfs         = (Glyf*)(Buffer + Offset);
		}	break;
		default:
			break;
		}

		auto c = 0;
	}

	if (Glyfs && CMap)
	{	// Load Glyfs
		int x = 0;
		CONVERT(Glyfs->NumberOfContours);
		CONVERT(Glyfs->xMax);
		CONVERT(Glyfs->xMin);
		CONVERT(Glyfs->yMax);
		CONVERT(Glyfs->yMin);
	}

	return{ true, Out };
}