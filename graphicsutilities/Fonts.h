

/**********************************************************************

Copyright (c) 2014-2018 Robert May

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


#ifndef FONTS_H
#define FONTS_H

#include "../buildsettings.h"
#include "../coreutilities/MathUtils.h"
#include "../coreutilities/memoryutilities.h"

namespace FlexKit
{	/************************************************************************************************/


	struct TextEntry
	{
		float2 POS;
		float2 Size;
		float2 TopLeftUV;
		float2 BottomRightUV;
		float4 Color;
	};


	struct TextArea
	{
		char*		TextBuffer;
		size_t		BufferSize;
		uint2		BufferDimensions;
		uint2		Position;
		float2		ScreenPosition;
		float2		CharacterScale;
		uint2		ScreenWH; // Screen Width - In Pixels

		size_t				 CharacterCount;
		size_t				 Dirty;

		iAllocator*			 Memory;
	};


	struct TextArea_Desc
	{
		float2 POS;		// Screen Space Cord
		float2 WH;		// WH of Area Being Rendered to,		Percent of Screen
		float2 CharWH;	// Size of Characters Being Rendered,	Percent of Screen
	};


	struct FontAsset
	{
		uint2	FontSize = { 0, 0 };
		bool	Unicode	 = false;

		uint2			TextSheetDimensions;
		Texture2D		Texture;
		uint4			Padding = 0; // Ordering { Top, Left, Bottom, Right }
		iAllocator*		Memory;

		struct Glyph
		{
			float2		WH;
			float2		XY;
			float2		Offsets;
			float		Xadvance;
			uint32_t	Channel;
		}GlyphTable[256];

		size_t	KerningTableSize = 0;
		struct Kerning
		{
			char	ID[2];
			float   Offset;
		}*KerningTable;


		char	FontName[128];
		char*	FontDir; // Texture Directory
	};


	struct Character
	{

	};

	struct CodePage
	{

	};

	struct TTFAsset
	{

	};


	/************************************************************************************************/
}

#endif