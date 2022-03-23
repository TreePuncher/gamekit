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

#include "stdafx.h"
#include "TTFontLoader.h"
#include "type.h"
#include "memoryutilities.h"

#include "Fonts.h"

#include <optional>


namespace FlexKit
{   /************************************************************************************************/

    typedef uint8_t TTF_UBYTE;
    typedef  int8_t TTF_BYTE;

    typedef uint16_t TTF_USHORT;
    typedef  int16_t TTF_SHORT;

    typedef uint32_t  TTF_ULONG;
    typedef  int32_t  TTF_LONG;

    typedef  int32_t TTF_FIXED;

    typedef  int16_t TTF_FWORD;

    typedef uint64_t  TTF_LONGDATE;


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

    TTF_ULONG CalcTableChecksum(TTF_ULONG *Table, TTF_ULONG Length)
    {
        TTF_ULONG  Sum = 0L;
        TTF_ULONG* Endptr = Table + ((Length + 3) & ~3) / sizeof(TTF_ULONG);
        while (Table < Endptr)
            Sum += *Table++;
        return Sum;
    }


    /************************************************************************************************/


    inline float FixedToFloat(TTF_FIXED N)
    {
        return float((N & 0xFF00)>> 8) +  float(N & 0x00FF) / 100.0f;
    }


    /************************************************************************************************/


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


        /************************************************************************************************/


        bool HasCharacter(char Code)
        {
            for (size_t I = 0; I < segCountX2 / 2; I++)
            {
                auto End = ConvertEndianness(EndCodes[I]);

                if (EndCodes[I] == 0xffff)
                    return false;

                if (ConvertEndianness(EndCodes[I]) <= Code)
                    continue;

                auto Segment = I - 1;
                auto Begin = ConvertEndianness(StartCodes[Segment]);

                if (Begin <= Code)
                    // Code Point Between these two ranges, Code Page Found
                    return true;
            }
            return false;
        }


        /************************************************************************************************/


        size_t GetCodePointGlyphIndex(char CP)
        {
            for (size_t I = 0; I < segCountX2/2; I++)
            {
                auto End = ConvertEndianness(EndCodes[I]);

                if (EndCodes[I] == 0xffff)
                    return -1;

                if (ConvertEndianness(EndCodes[I]) <= CP)
                    continue;

                auto Segment	= I - 1;
                auto Begin		= ConvertEndianness(StartCodes[Segment]);

                if (Begin <= CP)
                {	// Code Point Between these two ranges, Code Page Found
                    auto RangeOffset = ConvertEndianness(idRangeOffset[Segment]);

                    if (RangeOffset)
                        return *(RangeOffset / 2 + (CP - Begin) + &idRangeOffset[Segment]);
                    else
                        return ConvertEndianness(glyphIdArray[CP]);
                }
            }

            return -1;
        }

    };


    /************************************************************************************************/


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

            const auto SegmentCount = Converted.segCountX2 / 2;

            Converted.StartCodes	= EndCodes + SegmentCount + 1;
            Converted.idDelta		= (TTF_SHORT*)	Converted.StartCodes + SegmentCount;
            Converted.idRangeOffset = (TTF_USHORT*)	Converted.idDelta + SegmentCount;
            Converted.glyphIdArray  = (TTF_USHORT*)	Converted.idRangeOffset + SegmentCount;

            return Converted;
        }
    };


    /************************************************************************************************/


    struct UnicodeRange
    {
        TTF_ULONG	startUnicodeValue : 24;
        TTF_UBYTE	additionalCount;
    };


    /************************************************************************************************/


    struct DefaultUVSTable
    {
        TTF_ULONG		numUnicodeValueRanges;
        UnicodeRange	Ranges[];
    };


    /************************************************************************************************/


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


    /************************************************************************************************/


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


    /************************************************************************************************/


    struct _CMAP
    {
        USHORT		Version;
        USHORT		TableSize;
        CMAP_Entry	Table[];
    };


    /************************************************************************************************/


    struct CMap
    {
        USHORT		Version;
        USHORT		TableSize;
        CMAP_Entry*		Tables;
        FlexKit::byte*	Buffer;
        
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

        FlexKit::byte* GetSubTable(size_t Offset)
        {
            return Buffer + Offset;
        }
    };


    /************************************************************************************************/


    struct Glyph
    {
        TTF_SHORT NumberOfContours;// Negative Values Flag multiple Glyphs
        TTF_FWORD xMin;
        TTF_FWORD yMin;
        TTF_FWORD xMax;
        TTF_FWORD yMax;

        Glyph GetConverted() const
        {
            return {
                .NumberOfContours  = ConvertEndianness(NumberOfContours),
                .xMin              = ConvertEndianness(xMin),
                .yMin              = ConvertEndianness(yMin),
                .xMax              = ConvertEndianness(xMax),
                .yMax              = ConvertEndianness(yMax),
            };
        }
    };


    /************************************************************************************************/


    struct SimpleGlyph
    {
        TTF_USHORT*		EndPtsOfCountours;
        TTF_USHORT		InstructionCount;
        TTF_BYTE*		Inustructions;
        TTF_BYTE*		Flags;
        
        union
        {
            TTF_SHORT*		XCoords_16bit;
            TTF_SHORT*		YCoords_16bit;

            TTF_BYTE*		XCoords_8bit;
            TTF_BYTE*		YCoords_8bit;
        };

        enum class Bit
        {
            BIT_8,
            BIT_16,
        }Bits;
    };


    /************************************************************************************************/


    struct GlyphEntry
    {
        GlyphEntry() = default;

        GlyphEntry(
            const TTF_USHORT            IN_contourCount,
            const bool                  IN_compound,
            const FlexKit::byte* const  IN_buffer) :
                contourCount    { IN_contourCount   },
                compound        { IN_compound       },
                buffer          { IN_buffer         } {}

        struct Curve
        {
            FlexKit::float4 A = { 0 };
            FlexKit::float4 B = { 0 };
        };


        Curve GetCurve(const size_t Idx, const size_t Contour = 0) const
        {
            Curve Out;

            const FlexKit::byte* const Begin = buffer + sizeof(Glyph);

            if (compound)
            {
                const TTF_USHORT* const endPtsOfContours	= 
                    reinterpret_cast<const TTF_USHORT*>(Begin);

                const size_t InstructionLength = 
                    ConvertEndianness(endPtsOfContours[contourCount]);

                const TTF_UBYTE* instructions = 
                    reinterpret_cast<const TTF_UBYTE*>(endPtsOfContours + contourCount + 1);
            }
            else
            {
                const TTF_USHORT* endPtsOfContours = 
                    reinterpret_cast<const TTF_USHORT*>(Begin);

                const TTF_USHORT InstructionLength = 
                    endPtsOfContours[contourCount];

                const TTF_UBYTE* const instructions =
                    reinterpret_cast<const TTF_UBYTE*>(endPtsOfContours + contourCount + 1);

            }

            return Out;
        }

        const TTF_USHORT	        contourCount    = 0;
        const bool		            compound        = false;
        const FlexKit::byte* const  buffer          = nullptr;
    };


    GlyphEntry GetGlyphEntry(FlexKit::byte* buffer)
    {
        Glyph glyph;
        memcpy(&glyph, buffer, sizeof(Glyph));
        glyph = glyph.GetConverted();

        const auto Compound                 = glyph.NumberOfContours > 1;
        const TTF_USHORT NumberOfContours   = abs(glyph.NumberOfContours);
        const auto BufferBegin              = buffer + sizeof(Glyph);

        return { NumberOfContours, Compound, BufferBegin };
    }



    struct CompoundGlyph
    {
        TTF_USHORT	Flags;
        TTF_USHORT	GlyphIndex;

        union
        {
            struct 
            {	TTF_SHORT*		XCoords_S;
                TTF_SHORT*		YCoords_S;
            };

            struct 
            {	TTF_USHORT*		XCoords_US;
                TTF_USHORT*		YCoords_US;
            };

            struct
            {	TTF_BYTE*		XCoords_B;
                TTF_BYTE*		YCoords_B;
            };
        };

        enum class Bit
        {
            BIT_8,
            BIT_16,
        }Bits;

        bool Signed;

        TTF_USHORT	Options;
    };


    /************************************************************************************************/


    // https://docs.microsoft.com/en-us/typography/opentype/spec/head
    struct Head
    {
        Head() = default;

        Head(FlexKit::byte* Buffer) noexcept
        {
            Head* Temp = reinterpret_cast<Head*>(Buffer);
            Version				= ConvertEndianness(Temp->Version);
            FontVersion			= ConvertEndianness(Temp->FontVersion);
            CheckSumAdjustment	= ConvertEndianness(Temp->CheckSumAdjustment);
            MagicNumber			= ConvertEndianness(Temp->MagicNumber);
            Flags				= ConvertEndianness(Temp->Flags);
            UintsPerEm			= ConvertEndianness(Temp->UintsPerEm);
            Created				= ConvertEndianness(Temp->Created);
            Modified			= ConvertEndianness(Temp->Modified);
            XMin				= ConvertEndianness(Temp->XMin);
            YMin				= ConvertEndianness(Temp->YMin);
            XMax				= ConvertEndianness(Temp->XMax);
            YMax				= ConvertEndianness(Temp->YMax);
            MacStyle			= ConvertEndianness(Temp->MacStyle);
            LowestRecPPEM		= ConvertEndianness(Temp->LowestRecPPEM);
            FontDirectionHint	= ConvertEndianness(Temp->FontDirectionHint);
            IndexToLocFormat	= ConvertEndianness(Temp->IndexToLocFormat);
            GlyphDataFormat		= ConvertEndianness(Temp->GlyphDataFormat);
        }

        TTF_FIXED		Version;
        TTF_FIXED		FontVersion;

        TTF_ULONG		CheckSumAdjustment;
        TTF_ULONG		MagicNumber;

        TTF_SHORT		Flags;
        TTF_USHORT		UintsPerEm;

        TTF_LONGDATE	Created;
        TTF_LONGDATE	Modified;

        TTF_SHORT		XMin;
        TTF_SHORT		YMin;

        TTF_SHORT		XMax;
        TTF_SHORT		YMax;

        TTF_USHORT		MacStyle;
        TTF_SHORT		LowestRecPPEM;
        TTF_SHORT		FontDirectionHint;

        TTF_SHORT		IndexToLocFormat;
        TTF_SHORT		GlyphDataFormat;
    };


    /************************************************************************************************/


    struct Hhea
    {

    };


    /************************************************************************************************/


    struct Loca
    {
        Loca(FlexKit::byte* LocaBuffer, bool LongVersion)
        {
            LONG	= reinterpret_cast<TTF_ULONG*>(LocaBuffer);
            Length	= LongVersion;
        }

        union
        {
            TTF_ULONG*	LONG;
            TTF_USHORT*	SHORT;
        };

        TTF_ULONG GetOffset(size_t CP)	
        { 
            return Length ? ConvertEndianness(LONG[CP]) : (ConvertEndianness(SHORT[CP]) * 2);
        }

        bool	Length = false;
    };


    /************************************************************************************************/


    struct Maxp
    {

    };


    /************************************************************************************************/


    struct Name
    {
         
    };


    /************************************************************************************************/


    struct Post
    {

    };


    /************************************************************************************************/


    struct OS2
    {

    };


    /************************************************************************************************/


    class TTF_File
    {
    public:
        TTF_File(std::string& File, iAllocator* Memory)
        {
            auto FileSize	= GetFileSize(File.c_str());
            Buffer			= (FlexKit::byte*)Memory->_aligned_malloc(FileSize + 1);
            auto res		= LoadFileIntoBuffer(File.c_str(), Buffer, FileSize, false);
            FK_ASSERT(res, "failed to Load File!");

            FileDirectory	= (TTF_Directory*)Buffer;
            Entries			= (TTF_DirectoryEntry*)(Buffer + 12);

            TableCount		= ConvertEndianness(FileDirectory->TableCount);

    #ifdef _DEBUG
            PrintTableList();
    #endif
        }


        /************************************************************************************************/


        void PrintTableList()
        {
            TTF_DirectoryEntry* Entries = (TTF_DirectoryEntry*)(Buffer + 12);
            for (size_t I = 0; I < TableCount; ++I)
            {
                std::cout <<
                    Entries[I].Tag_C[0] <<
                    Entries[I].Tag_C[1] <<
                    Entries[I].Tag_C[2] <<
                    Entries[I].Tag_C[3] << '\n';
            }
        }


        /************************************************************************************************/


        std::optional<TTF_DirectoryEntry> FindDirectoryEntry(TTF_ULONG ID) const
        {
            TTF_DirectoryEntry* Entries = (TTF_DirectoryEntry*)(Buffer + 12);

            for (size_t I = 0; I < TableCount; ++I)
            {
                auto Entry    = Entries[I].ConvertEndianess();
                auto Length   = Entry.length;
                size_t Offset = Entry.offset;

                if (Entry.Tag_UL == ID) 
                    return { Entry };
            }

            return {};
        }


        /************************************************************************************************/


        std::optional<TTF_DirectoryEntry> FindDirectoryEntry(const char* Tag) const
        {
            TTF_DirectoryEntry* Entries = (TTF_DirectoryEntry*)(Buffer + 12);

            for (size_t I = 0; I < TableCount; ++I)
            {
                auto Entry		= Entries[I].ConvertEndianess();
                auto Length		= Entry.length;
                size_t Offset	= Entry.offset;

                if (Entry.Tag_C[0] == Tag[0] && 
                    Entry.Tag_C[1] == Tag[1] && 
                    Entry.Tag_C[2] == Tag[2] && 
                    Entry.Tag_C[3] == Tag[3])
                        return { Entry };
            }

            return {};
        }


        /************************************************************************************************/


        CMap GetCMap()
        {
            if (auto cmap = FindDirectoryEntry("cmap"); cmap)
            {
                TTF_DirectoryEntry& Entry	= cmap.value();


                size_t Offset = Entry.offset;

                //auto CheckSum = CalcTableChecksum((TTF_ULONG*)(&Entries[I]), sizeof(Entry));
                //FK_ASSERT(Checksum == Entry.Checksum);

                auto Temp = reinterpret_cast<_CMAP*>(Buffer + Offset);
                CMap Out;
                Out.Tables		= Temp->Table;
                Out.Version		= ConvertEndianness(Temp->Version);
                Out.TableSize	= ConvertEndianness(Temp->TableSize);
                Out.Buffer		= Buffer + Offset;

                return Out;
            }

            return {};
        }


        /************************************************************************************************/


        Loca GetLoca() const
        {
            if(auto RES  = FindDirectoryEntry("loca"); RES)
            {
                const auto Head = GetHead();
                const auto& EntryLoca = RES.value();

                return { Buffer + EntryLoca.offset, static_cast<bool>(Head.IndexToLocFormat) };
            }

            return { nullptr, false };
        }


        /************************************************************************************************/


        Head GetHead() const
        {
            if (auto head = FindDirectoryEntry("head"); head)
            {
                const TTF_DirectoryEntry& EntryLoca = head.value();
                return { Buffer + EntryLoca.offset };
            }

            return {};
        }


        /************************************************************************************************/


        const Glyph* GetGlyphs()
        {
            if (auto glyf = FindDirectoryEntry("glyf"); glyf)
            {
                const TTF_DirectoryEntry& Entry = glyf.value();

                const auto Offset = Entry.offset;
                const auto Glyphs = reinterpret_cast<Glyph*>(Buffer + Offset);

                return Glyphs;
            }

            return nullptr;
        }


        /************************************************************************************************/


        GlyphEntry GetGlyph(size_t Glyph_offset)
        {
            if (auto glyf = FindDirectoryEntry("glyf"); glyf)
            {
                const TTF_DirectoryEntry& Entry = glyf.value();
                return GetGlyphEntry( Buffer + Entry.offset + Glyph_offset );
            }

            return {};
        }


        /************************************************************************************************/


        struct Curve
        {
            FlexKit::float4 A = { 0 };
            FlexKit::float4 B = { 0 };
        };



        /************************************************************************************************/


        size_t					TableCount;
        TTF_Directory*			FileDirectory;
        TTF_DirectoryEntry*		Entries;

        size_t					BufferSize;
        FlexKit::byte*			Buffer;
    };


    /************************************************************************************************/


    size_t GetGlyphCode(
        const TTF_USHORT*	IDRangeOffsets, 
        const TTF_USHORT*	StartCodes, 
        const TTF_USHORT*	EndCodes, 
        const size_t		TableSize, 
        char				C = 'A')
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


    /************************************************************************************************/


    Pair<bool, TTFont*> LoadTTFFile(std::string& file, iAllocator* Memory)
    {
        TTFont*		Out = nullptr;
        TTF_File	Font(file, Memory);

        // Tables
        std::cout << "Tables in Font:\n";
        Font.PrintTableList();

        Head	Head		= Font.GetHead();
        Loca    Loca		= Font.GetLoca();
        CMap	CMap		= Font.GetCMap();

        if (CMap.HasWindowsEntry())
        {
            auto Table			= CMap.GetWindowsEntry();
            auto Format4Table	= CMap.GetSubTableAsFormat4(Table.SubTableOffset);

            const auto glyph        = Font.GetGlyph(Loca.GetOffset(0));
            const auto contourCount = glyph.contourCount;
            const auto curve        = glyph.GetCurve(0);
            /*
            if (Glyph.NumberOfContours > 0)
            {
                auto Glyphs = Font.GetGlyphs_Simple(Loca.GetOffset('A'));
                size_t ContourCount = std::abs(Glyph.NumberOfContours);
            }
            */
            int x = 0;
        }
        else
            return { false, Out };

        return{ true, Out };
    }


}/************************************************************************************************/
