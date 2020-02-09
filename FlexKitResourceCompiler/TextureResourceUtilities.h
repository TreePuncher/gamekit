#include <stb_image.h>
#include <vector>

#include "Common.h"
#include "MetaData.h"
#include "TextureUtilities.h"
#include "ResourceUtilities.h"




FlexKit::FORMAT_2D FormatStringToFORMAT_2D(const std::string& format)
{
    static std::map<std::string, FlexKit::FORMAT_2D> formatMap = {
        { "RGBA8_UNORM",    FORMAT_2D::R8G8B8A8_UNORM       },
        { "RGBA16_FLOAT",   FORMAT_2D::R16G16B16A16_FLOAT   },
        { "RGBA32_FLOAT",   FORMAT_2D::R32G32B32A32_FLOAT   },
    };

    return formatMap[format];
}


struct CubeMapFace
{
    std::vector<FlexKit::TextureBuffer> mipLevels;

    Blob CreateBlob()
    {
        size_t currentOffset  = sizeof(size_t) + sizeof(size_t) * mipLevels.size() * 2;
        const size_t mipCount = mipLevels.size();

        Blob offsets;
        Blob sizes;
        Blob Body;

        for (auto& mipLevel : mipLevels)
        {
            offsets += Blob{ currentOffset };
            sizes   += Blob{ mipLevel.Size };
            currentOffset += mipLevel.Size;

            Blob textureBuffer;
            textureBuffer.resize(mipLevel.BufferSize());
            memcpy(textureBuffer.data(), mipLevel.Buffer, mipLevel.Size);

            Body += textureBuffer;
        }

        return Blob{ mipCount } + offsets + sizes + Body;
    }
};


class CubeMapTexture : public iResource
{
public:
    CubeMapFace                 Faces[6];
    size_t                      GUID;
    std::string					ID;
    FlexKit::FORMAT_2D          format;

    size_t                      Width   = 0;
    size_t                      Height  = 0;


    ResourceBlob CreateBlob()
    {
        struct Header
        {
            size_t			ResourceSize;
            EResourceType	Type = EResourceType::EResource_CubeMapTexture;
            GUID_t			GUID;
            size_t			Pad;

            size_t          Width = 0;
            size_t          Height = 0;
            size_t          MipCount;

            size_t          Format;

            size_t offsets[6] = { 0 };
        } header;

        size_t currentOffset = sizeof(Header);

        Blob TextureBlob;
        for (size_t I = 0; I < 6; I++)
        {
            auto blob = Faces[I].CreateBlob();
            header.offsets[I] = currentOffset;

            currentOffset += blob.size();
            TextureBlob += blob;
        }

        header.GUID         = GUID;
        header.ResourceSize = sizeof(Header) + TextureBlob.size();
        header.Height       = Height;
        header.Width        = Width;
        header.MipCount     = Faces[0].mipLevels.size();
        header.Format       = (size_t)format;

        auto [_ptr, size] = (Blob{ header } + TextureBlob).Release();

        ResourceBlob out;
        out.buffer          = (char*)_ptr;
        out.bufferSize      = size;
        out.GUID            = GUID;
        out.ID              = ID;
        out.resourceType    = EResourceType::EResource_CubeMapTexture;

        return out;
    }
};


inline std::shared_ptr<iResource> CreateCubeMapResource(std::shared_ptr<TextureCubeMap_MetaData> metaData)
{
    auto resource       = std::make_shared<CubeMapTexture>();
    resource->GUID      = metaData->Guid;
    resource->ID        = metaData->ID;
    resource->format    = FormatStringToFORMAT_2D(metaData->format);

    for (auto& face : resource->Faces)
        face.mipLevels.resize(metaData->mipLevels.size());

    for (auto& mipLevel : metaData->mipLevels)
    {
        auto cubeMap = std::static_pointer_cast<TextureCubeMapMipLevel_MetaData>(mipLevel);

        for (size_t itr = 0; itr < 6; ++itr)
            resource->Faces[itr].mipLevels[cubeMap->level] = FlexKit::LoadHDR(cubeMap->TextureFiles[itr].c_str(), 1)[0];
    }

    resource->Height    = resource->Faces[0].mipLevels[0].WH[0];
    resource->Width     = resource->Faces[0].mipLevels[0].WH[1];

    return resource;
}




/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
