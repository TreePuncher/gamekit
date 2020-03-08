#include <stb_image.h>
#include <vector>

#include "Common.h"
#include "MetaData.h"
#include "TextureUtilities.h"
#include "ResourceUtilities.h"

#include "crnlib.h"
#include "crn_decomp.h"
#include "dds_defs.h"

#pragma comment(lib,"crunch.lib")


FlexKit::DeviceFormat FormatStringToDeviceFormat(const std::string& format)
{
    static std::map<std::string, FlexKit::DeviceFormat> formatMap = {
        { "RGBA8_UNORM",    DeviceFormat::R8G8B8A8_UNORM       },
        { "RGBA16_FLOAT",   DeviceFormat::R16G16B16A16_FLOAT   },
        { "RGBA32_FLOAT",   DeviceFormat::R32G32B32A32_FLOAT   },
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
    FlexKit::DeviceFormat          format;

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
    resource->format    = FormatStringToDeviceFormat(metaData->format);

    for (auto& face : resource->Faces)
        face.mipLevels.resize(metaData->mipLevels.size());

    for (auto& mipLevel : metaData->mipLevels)
    {
        auto cubeMap = std::static_pointer_cast<TextureCubeMapMipLevel_MetaData>(mipLevel);

        for (size_t itr = 0; itr < 6; ++itr)
            resource->Faces[itr].mipLevels[cubeMap->level] = FlexKit::LoadHDR(cubeMap->TextureFiles[itr].c_str(), 1)[0];
    }

    resource->Height = resource->Faces[0].mipLevels[0].WH[0];
    resource->Width = resource->Faces[0].mipLevels[0].WH[1];

    return resource;
}


void CreateCompressedDDSTextureResource()
{
}


enum class DDSTextureFormat
{
    DXT5,
    UNKNOWN
};


DDSTextureFormat FormatStringToFormatID(const std::string& ID)
{
    if (ID == "DXT5")
        return DDSTextureFormat::DXT5;

    return DDSTextureFormat::UNKNOWN;
}

struct _TextureMipLevelResource
{
    int         width;
    int         height;
    int         channelCount;

    crn_uint32* buffer;


    void Release()
    {
        delete[] buffer;
        buffer = nullptr;
    }
};

class TextureResource : public iResource
{
public:
    TextureResource() {}

    ResourceBlob CreateBlob() override
    {
        TextureResourceBlob headerData;

        headerData.format       = format;
        headerData.ResourceSize = sizeof(headerData) + bufferSize;
        headerData.GUID         = assetHandle;

        Blob header { headerData };
        Blob body   = { (const char*)buffer, bufferSize };

        strncpy(headerData.ID, ID.c_str(), min(sizeof(headerData.ID), ID.size()));

        auto [data, size]   = (header + body).Release();

        ResourceBlob out;
        out.buffer          = (char*)data;
        out.bufferSize      = size;
        out.GUID            = assetHandle;
        out.ID              = ID;
        out.resourceType    = EResourceType::EResource_Scene;

        return out;
    }

    std::string         ID;
    GUID_t              assetHandle;
    FlexKit::DeviceFormat  format;

    void*     buffer;
    size_t    bufferSize;
};

_TextureMipLevelResource CreateMIPMapResource(const std::string& string)
{
    int width;
    int height;
    int channelCount;

    float* res = stbi_loadf(string.c_str(), &width, &height, &channelCount, STBI_default);
    if (!res)
        return {};

    auto pixelCount = width * height;

    crn_uint32* buffer = new crn_uint32[pixelCount];

    for (size_t I = 0; I < pixelCount; I++)
    {
        switch (channelCount)
        {
        case 3:
        {
            struct
            {
                uint8_t R;
                uint8_t G;
                uint8_t B;
                uint8_t padding;
            }pixel = {
                (uint8_t)(res[I * channelCount + 0] * 255),
                (uint8_t)(res[I * channelCount + 1] * 255),
                (uint8_t)(res[I * channelCount + 2] * 255),
            };

            memcpy(&buffer[I], &pixel, sizeof(pixel));
        }   break;
        case 4:
        {
            struct
            {
                uint8_t R;
                uint8_t G;
                uint8_t B;
                uint8_t A;
            }pixel = {
                (uint8_t)(res[I * channelCount + 0] * 255),
                (uint8_t)(res[I * channelCount + 1] * 255),
                (uint8_t)(res[I * channelCount + 2] * 255),
                (uint8_t)(res[I * channelCount + 3] * 255),
            };
            memcpy(&buffer[I], &pixel, sizeof(pixel));
        }   break;
        default:
            break;
        }
    }

    return {
        .width          = width,
        .height         = height,
        .channelCount   = channelCount,
        .buffer         = buffer
    };
}


inline std::shared_ptr<iResource> CreateTextureResource(std::shared_ptr<Texture_MetaData> metaData)
{
    auto format = FormatStringToFormatID(metaData->format);
    FlexKit::DeviceFormat  FK_format;

    std::vector<_TextureMipLevelResource> mipLevels{};

    if (metaData->mipLevels.size() && !metaData->generateMipMaps)
    {
        for (auto& mipLevel : metaData->mipLevels)
            mipLevels.push_back(
                CreateMIPMapResource(
                    std::static_pointer_cast<TextureMipLevel_MetaData>(mipLevel)->file));
    }
    else
        mipLevels.push_back(CreateMIPMapResource(metaData->file));


    crn_comp_params params = {};
    params.clear();

    switch (format)
    {
    case DDSTextureFormat::DXT5:
    {
        params.m_faces                           = 1;
        params.m_width                           = mipLevels[0].width;
        params.m_height                          = mipLevels[0].height;
        params.m_target_bitrate                  = 0;
        params.m_format                          = crn_format::cCRNFmtDXT5;
        params.m_num_helper_threads              = min((size_t)std::thread::hardware_concurrency(), cCRNMaxHelperThreads);
        params.m_dxt_compressor_type             = crn_dxt_compressor_type::cCRNDXTCompressorCRNF;
        params.m_dxt_quality                     = cCRNDXTQualityNormal;
        params.m_crn_color_selector_palette_size = 8;
        params.m_file_type                       = cCRNFileTypeCRN;

        FK_format = DeviceFormat::BC5_UNORM;

        if (!params.check())
            return {};
    }   break;
    default:
        return {};
    }

    crn_uint32  size = 0;
    crn_uint32  actualQualityLevel;
    float       actualBitrate;

    for (size_t I = 0; I < mipLevels.size(); I++)
        params.m_pImages[0][I] = mipLevels[I].buffer;

    auto compressedTexture =
        crn_compress(
            params,
            size,
            &actualQualityLevel,
            &actualBitrate);

    auto resource = std::make_shared<TextureResource>();

    resource->ID            = metaData->stringID;
    resource->assetHandle   = metaData->assetID;
    resource->buffer        = compressedTexture;
    resource->bufferSize    = size;
    resource->format        = FK_format;

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
