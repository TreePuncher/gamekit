#include "Brush.hpp"
#include "CameraComponent.hpp"
#include "MathUtilities.hpp"
#include "Transforms.hpp"

namespace FlexKit
{	/************************************************************************************************/


	size_t CreateSortingID(bool Posed, bool Textured, size_t Depth)
	{
		size_t DepthPart	= (Depth & 0x00ffffffffffff);
		size_t PosedBit		= (size_t(Posed)	<< (Posed		? 63 : 0));
		size_t TextureBit	= (size_t(Textured) << (Textured	? 62 : 0));

		return DepthPart | PosedBit | TextureBit;
	}

	void SortDrawList(std::span<BrushEntry> drawList, Camera* C)
	{
		if(!drawList.size())
			return;

		auto CP = FlexKit::GetPositionW(C->Node);
		for(auto& v : drawList)
		{
			auto b = v.brush;
			auto P = FlexKit::GetPositionW( b->Node );

			auto Depth = (size_t)abs(float3(CP - P).magnitudeSq() * 10000);
			auto SortID = CreateSortingID(false, b->Textured, Depth);
			v.SortID = SortID;
		}
		
		std::sort(drawList.begin(), drawList.end(), [](const BrushEntry& R, const BrushEntry& L ) -> bool
		{
			return ( (size_t)R.SortID < (size_t)L.SortID);
		} );
	}


	/************************************************************************************************/


	void SortDrawListTransparent(std::span<BrushEntry> drawList, Camera* C)
	{
		if(!drawList.size())
			return;

		auto CP = FlexKit::GetPositionW( C->Node );
		for( auto& v : drawList)
		{
			const Brush* b	= v.brush;
			const float3 P	= GetPositionW( b->Node );
			const float D	= float3{ CP - P }.magnitudeSq() * (b->DrawLast ? -1.0f : 1.0f);
			v.SortID		= (uint64_t)D;
		}

		std::sort(drawList.begin(), drawList.end(), []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R > (size_t)L );
		} );
	}


	/************************************************************************************************/
}


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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
