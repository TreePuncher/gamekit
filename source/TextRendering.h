/**********************************************************************

Copyright (c) 2018 Robert May

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


#ifndef TEXTRENDERING_H
#define TEXTRENDERING_H

#include "FrameGraph.h"
#include "Fonts.h"

namespace FlexKit
{
    class RenderSystem;
    class FrameGraph;


	ID3D12PipelineState* LoadSpriteTextPSO(RenderSystem* RS);


	void DrawSprite_Text(
		const char*				Str,
		FrameGraph&				FGraph, 
		SpriteFontAsset&		Font,
		VertexBufferHandle		Buffer,
		ResourceHandle			RenderTarget,
		iAllocator*				TempMemory,
		PrintTextFormatting&	Formatting);

	void DrawSprite_Text(
		const char*				Str,
		FrameGraph&				FGraph,
		SpriteFontAsset&		Font,
		VertexBufferHandle		Buffer,
		ResourceHandle			RenderTarget,
		iAllocator*				TempMemory,
		PrintTextFormatting&	Formatting);

}
#endif // TEXTRENDERING_H
