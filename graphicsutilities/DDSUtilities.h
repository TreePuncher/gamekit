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

#ifndef DDSUTILITIES_H
#define DDSUTILITIES_H

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\graphics.h"

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#include <DDSTextureLoader.h>
#include <d3d12.h>
#include <dds.h>

namespace FlexKit
{
    using DirectX::DDS_HEADER;
    using DirectX::DDS_HEADER_DXT10;
    using DirectX::DDS_ALPHA_MODE;
    using DirectX::DDS_PIXELFORMAT;
    using DirectX::DDS_MAGIC;

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

	FLEXKITAPI DDSTexture2DLoad_RES LoadDDSTexture2DFromFile(const char* File, iAllocator* In, RenderSystem* RS, UploadQueueHandle);
	FLEXKITAPI DDSTexture3DLoad_RES LoadDDSTexture3DFromFile();

	typedef FlexKit::Pair<ResourceHandle, bool> LoadDDSTexture2DFromFile_RES;
	FLEXKITAPI	LoadDDSTexture2DFromFile_RES LoadDDSTexture2DFromFile_2(const char* File, iAllocator* Memory, RenderSystem* RS, UploadQueueHandle);

	FLEXKITAPI Texture2D LoadDDSIntoResource();
}

#endif
