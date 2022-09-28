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

#include "buildsettings.h"
#include "containers.h"
#include "memoryutilities.h"
#include "graphics.h"

#include <d3d12.h>
#include <DirectXTK12/DDSTextureLoader.h>
#include "..\ThirdParty\DDS.h"

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
				((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
				((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))


namespace FlexKit
{
	using DirectX::DDS_ALPHA_MODE;

	struct DDS_PIXELFORMAT
	{
		uint32_t    size;
		uint32_t    flags;
		uint32_t    fourCC;
		uint32_t    RGBBitCount;
		uint32_t    RBitMask;
		uint32_t    GBitMask;
		uint32_t    BBitMask;
		uint32_t    ABitMask;
	};

	struct DDS_HEADER
	{
		uint32_t        size;
		uint32_t        flags;
		uint32_t        height;
		uint32_t        width;
		uint32_t        pitchOrLinearSize;
		uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
		uint32_t        mipMapCount;
		uint32_t        reserved1[11];
		DDS_PIXELFORMAT ddspf;
		uint32_t        caps;
		uint32_t        caps2;
		uint32_t        caps3;
		uint32_t        caps4;
		uint32_t        reserved2;
	};

	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT     dxgiFormat;
		uint32_t        resourceDimension;
		uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
		uint32_t        arraySize;
		uint32_t        miscFlags2; // see DDS_MISC_FLAGS2
	};

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
	typedef FlexKit::Pair<DDSTexture3D*, bool> DDSTexture3DLoad_RES; // Not-Implemented

	FLEXKITAPI DDSTexture2DLoad_RES LoadDDSTexture2DFromFile(const char* File, iAllocator* In, RenderSystem* RS, CopyContextHandle);
	FLEXKITAPI DDSTexture3DLoad_RES LoadDDSTexture3DFromFile();

	typedef FlexKit::Pair<ResourceHandle, bool> LoadDDSTexture2DFromFile_RES;
	FLEXKITAPI	LoadDDSTexture2DFromFile_RES LoadDDSTexture2DFromFile_2(const char* File, iAllocator* Memory, RenderSystem* RS, CopyContextHandle);

	FLEXKITAPI Texture2D LoadDDSIntoResource();
}

#endif
