/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#ifndef DDSUTILITIES_H
#define DDSUTILITIES_H

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\graphics.h"
#include "Dds.h"
#include <d3d12.h>

namespace FlexKit
{
	struct DDSFILE_1
	{
		DWORD		dwMagic;
		DDS_HEADER	Header;
		BYTE		bdata[];
	};

	struct DDSFILE_2
	{
		DWORD				dwMagic;
		DDS_HEADER			Header;
		DDS_HEADER_DXT10	DXT10Header;
		BYTE				bdata[];
	};

	struct DDSTexture2D
	{
		uint32_t		MipMapLevels;
		uint2			WH;
		DDS_ALPHA_MODE	Alpha;
		DXGI_FORMAT		Format;
		ID3D12Resource* Texture;
	};

	struct DDSTexture3D
	{

	};

	typedef FlexKit::Pair<DDSTexture2D*, bool> DDSTexture2DLoad_RES;
	typedef FlexKit::Pair<DDSTexture3D*, bool> DDSTexture3DLoad_RES;

	FLEXKITAPI DDSTexture2DLoad_RES LoadDDSTexture2DFromFile(const char* File, iAllocator* In, RenderSystem* RS);
	FLEXKITAPI DDSTexture3DLoad_RES LoadDDSTexture3DFromFile();

	FLEXKITAPI Texture2D LoadDDSIntoResource();
}

#endif