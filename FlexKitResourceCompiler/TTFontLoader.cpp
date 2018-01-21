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

#include "..\graphicsutilities\Fonts.h"

typedef uint8_t TTF_UBYTE;
typedef  int8_t TTF_BYTE;

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

	TTF_DirectoryEntry ConvertEndianess() const
	{
		TTF_DirectoryEntry Converted = *this;
		Converted.checkSum	= ConvertEndianness(checkSum);
		Converted.offset	= ConvertEndianness(offset);
		Converted.length	= ConvertEndianness(length);

		return Converted;
	}
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
CalcTableChecksum(TTF_ULONG *Table, TTF_ULONG Length)
{
	TTF_ULONG  Sum = 0L;
	TTF_ULONG* Endptr = Table + ((Length + 3) & ~3) / sizeof(TTF_ULONG);
	while (Table < Endptr)
		Sum += *Table++;
	return Sum;
}



inline float FixedToFloat(TTF_FIXED N)
{
	return float((N & 0xFF00)>> 8) +  float(N & 0x00FF) / 100.0f;
}

struct Format4Encoding
{
	TTF_USHORT	Format;
	TTF_USHORT	Length; // Byte Length
	TTF_USHORT	Language;
	TTF_USHORT	segCountX2;
	TTF_USHORT	SearchRange;
	TTF_USHORT	EntrySelector;
	TTF_USHORT	RangeShift;
	TTF_USHORT* StartCodes;
	TTF_USHORT* EndCodes;
	TTF_USHORT* glyphIdArray;
	TTF_SHORT*  idDelta;
	TTF_USHORT* idRangeOffset;

	bool HasCharacter(char Code)
	{
		for (size_t I = 0; I < segCountX2/2; I++)
		{
			auto End = ConvertEndianness(EndCodes[I]);
			if (ConvertEndianness(EndCodes[I]) >= Code)
			{
				auto EndCode	= I - 1;
				auto Begin		= ConvertEndianness(StartCodes[I]);

				if (Begin <= Code)
				{	// Code Point Between these two ranges, Code Page Found
					auto RangeOffset = ConvertEndianness(idRangeOffset[I]);
					int c = 0;
				}
				int c = 0;
			}
		}
		return false;
	}
};

struct _Format4Encoding
{
	TTF_USHORT Format;
	TTF_USHORT Length; // Byte Length
	TTF_USHORT Language; // Only Used On Mac
	TTF_USHORT segCountX2;
	TTF_USHORT SearchRange;
	TTF_USHORT EntrySelector;
	TTF_USHORT RangeShift;
	TTF_USHORT EndCodes[];

	Format4Encoding GetEndianConverted()
	{
		Format4Encoding Converted;
		Converted.Format		= ConvertEndianness(Format);
		Converted.Length		= ConvertEndianness(Length); // Byte Length
		Converted.Language		= ConvertEndianness(Language);
		Converted.segCountX2	= ConvertEndianness(segCountX2);
		Converted.SearchRange	= ConvertEndianness(SearchRange);
		Converted.EntrySelector	= ConvertEndianness(EntrySelector);
		Converted.RangeShift	= ConvertEndianness(RangeShift);
		Converted.EndCodes		= EndCodes;

		auto SegmentCount = Converted.segCountX2 / 2;

		Converted.StartCodes	= EndCodes + SegmentCount + 1;
		Converted.idDelta		= (TTF_SHORT*)	Converted.StartCodes + SegmentCount;
		Converted.idRangeOffset = (TTF_USHORT*)	Converted.idDelta + SegmentCount;
		Converted.glyphIdArray  = (TTF_USHORT*)	Converted.idRangeOffset + SegmentCount;

		return Converted;
	}
};

struct UnicodeRange
{
	TTF_ULONG	startUnicodeValue : 24;
	TTF_UBYTE	additionalCount;

};

struct DefaultUVSTable
{
	TTF_ULONG		numUnicodeValueRanges;
	UnicodeRange	Ranges[];
};

struct Format14Encoding
{
	TTF_USHORT Format;
	TTF_USHORT Length; // Byte Length
	TTF_USHORT NumVarSelectorRecords;

	/*
	Each variation selector records specifies a variation selector character,
	and offsets to “default” and “non-default” tables used to map variation
	sequences using that variation selector.
	*/

	struct varSelector
	{
		TTF_ULONG	VarSelector : 24;
		TTF_ULONG	defaultUVSOffset;
		TTF_ULONG	nonDefaultUVSOffset;

		varSelector GetEndianConverted()
		{
			varSelector Converted			= *this;
			Converted.VarSelector			= ConvertEndianness(VarSelector);
			Converted.defaultUVSOffset		= ConvertEndianness(defaultUVSOffset);
			Converted.nonDefaultUVSOffset	= ConvertEndianness(nonDefaultUVSOffset);

			return Converted;
		}
	}Records[];
};

struct CMAP_Entry
{
	TTF_USHORT PlatformID;
	TTF_USHORT EncodingID;
	TTF_ULONG  SubTableOffset;

	CMAP_Entry GetEndianConverted()
	{
		CMAP_Entry Converted;
		Converted.PlatformID		= ConvertEndianness(PlatformID);
		Converted.EncodingID		= ConvertEndianness(EncodingID);
		Converted.SubTableOffset	= ConvertEndianness(SubTableOffset);

		return Converted;
	}
};

struct _CMAP
{
	USHORT		Version;
	USHORT		TableSize;
	CMAP_Entry	Table[];
};


struct CMap
{
	USHORT		Version;
	USHORT		TableSize;
	CMAP_Entry*	Tables;
	byte*		Buffer;
	
	bool HasWindowsEntry()
	{
		bool Out = false;

		for (size_t I = 0; I < TableSize; I++)
		{
			auto Table = Tables[I].GetEndianConverted();

			if (	Table.PlatformID == 0x03 &&
					Table.EncodingID == 0x01)
			{
				Out = true;
				break;
			};
		}
		return Out;
	}

	CMAP_Entry GetWindowsEntry()
	{
		for (size_t I = 0; I < TableSize; I++)
		{
			auto Table = Tables[I].GetEndianConverted();

			if (	Table.PlatformID == 0x03 &&
					Table.EncodingID == 0x01)
				return Table;
		}

		return CMAP_Entry();
	}

	Format4Encoding GetSubTableAsFormat4(size_t Offset)
	{
		return reinterpret_cast<_Format4Encoding*>(Buffer + Offset)->GetEndianConverted();
	}

	byte* GetSubTable(size_t Offset)
	{
		return Buffer + Offset;
	}
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

class TTF_File
{
public:
	TTF_File(const char* File, iAllocator* Memory)
	{
		auto FileSize = GetFileSize(File);
		Buffer		= (byte*)Memory->_aligned_malloc(FileSize + 1);
		auto res	= LoadFileIntoBuffer(File, Buffer, FileSize, false);
		FK_ASSERT(res, "failed to Load File!");

		FileDirectory	= (TTF_Directory*)Buffer;
		Entries			= (TTF_DirectoryEntry*)(Buffer + 12);

		TableCount	= ConvertEndianness(FileDirectory->TableCount);
	}


	CMap GetCMap()
	{
		TTF_DirectoryEntry* Entries = (TTF_DirectoryEntry*)(Buffer + 12);

		for (size_t I = 0; I < TableCount; ++I)
		{
			auto Entry    = Entries[I].ConvertEndianess();
			auto Length   = Entry.length;
			size_t Offset = Entry.offset;
			auto CheckSum = CalcTableChecksum((TTF_ULONG*)(&Entries[I]), sizeof(Entry));

			//FK_ASSERT(Checksum == Entry.Checksum);

			if (Entry.Tag_UL == 0x70616d63) {
				auto Temp = reinterpret_cast<_CMAP*>(Buffer + Offset);
				CMap Out;
				Out.Tables		= Temp->Table;
				Out.Version		= ConvertEndianness(Temp->Version);
				Out.TableSize	= ConvertEndianness(Temp->TableSize);
				Out.Buffer		= Buffer + Offset;
				return Out;
			}
		}

		return CMap();
	}

	size_t					TableCount;
	TTF_Directory*			FileDirectory;
	TTF_DirectoryEntry*		Entries;

	size_t					BufferSize;
	byte*					Buffer;
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
	TTFont* Out = nullptr;
	TTF_File	Font(File, Memory);

	// Tables
	CMap CMap	= Font.GetCMap();

	if (CMap.HasWindowsEntry())
	{
		auto Table			= CMap.GetWindowsEntry();
		auto Format4Table	= CMap.GetSubTableAsFormat4(Table.SubTableOffset);

		auto Test = Format4Table.HasCharacter('a');
		int c = 0;
	}
	else
		return { false, Out };

	return{ true, Out };
}