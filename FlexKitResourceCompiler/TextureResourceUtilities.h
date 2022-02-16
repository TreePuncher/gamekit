#pragma once
#include <vector>

#include "Common.h"
#include "MetaData.h"
#include "TextureUtilities.h"
#include "ResourceUtilities.h"

#include "Compressonator.h"
#include <filesystem>

#include "../coreutilities/Serialization.hpp"


namespace FlexKit
{   /************************************************************************************************/


    inline FlexKit::DeviceFormat FormatStringToDeviceFormat(const std::string& format);


    /************************************************************************************************/


    struct CubeMapFace
    {
        std::vector<FlexKit::TextureBuffer> mipLevels;

        Blob CreateBlob() const;
    };


    /************************************************************************************************/


    class CubeMapTexture : public iResource
    {
    public:
        CubeMapFace                 Faces[6];
        size_t                      GUID;
        std::string					ID;
        FlexKit::DeviceFormat       format;

        size_t                      Width   = 0;
        size_t                      Height  = 0;


        ResourceBlob CreateBlob() const override;

        const ResourceID_t GetResourceTypeID() const noexcept override { return CubeMapResourceTypeID; }
    };


    inline std::shared_ptr<iResource> CreateCubeMapResource(std::shared_ptr<TextureCubeMap_MetaData> metaData);


    /************************************************************************************************/


    inline void CreateCompressedDDSTextureResource()
    {
    }


    /************************************************************************************************/


    enum class DDSTextureFormat
    {
        DXT3,
        DXT5,
        DXT7,
        UNKNOWN
    };


    /************************************************************************************************/


    inline DDSTextureFormat FormatStringToFormatID(const std::string& ID)
    {
        if (ID == "DXT3")
            return DDSTextureFormat::DXT3;
        if (ID == "DXT5")
            return DDSTextureFormat::DXT5;
        if (ID == "DXT7")
            return DDSTextureFormat::DXT7;

        return DDSTextureFormat::UNKNOWN;
    }


    /************************************************************************************************/


    struct _TextureMipLevelResource
    {
        CMP_MipSet mipSet;
        std::string file;

        void Release()
        {
            CMP_FreeMipSet(&mipSet);
        }
    };


    /************************************************************************************************/


    class TextureResource : public Serializable<TextureResource, iResource, GetTypeGUID(TextureResource)>
    {
    public:
        void Serialize(auto& ar)
        {
            ar & ID;
            ar & assetHandle;
            ar & format;
            ar & mipLevels;
            ar & mipOffsets;
            ar & WH;
            ar & bufferSize;
            ar & RawBuffer{ buffer, bufferSize };
        }

        TextureResource() {}

        const std::string&  GetResourceID()     const noexcept final { return ID; }
        const uint64_t      GetResourceGUID()   const noexcept final { return assetHandle; }
        const ResourceID_t  GetResourceTypeID() const noexcept final { return TextureResourceTypeID; }

        ResourceBlob CreateBlob() const override;

        std::string             ID              = "";
        GUID_t                  assetHandle     = -1;
        FlexKit::DeviceFormat   format          = FlexKit::DeviceFormat::UNKNOWN;
        uint32_t                mipLevels       = 0;
        uint32_t                mipOffsets[15]  = { 0 };
        uint2                   WH              = { 0, 0 };

        void*     buffer        = nullptr;
        size_t    bufferSize    = 0;
    };


    /************************************************************************************************/


    _TextureMipLevelResource    CreateMIPMapResource(const std::string& string);

    std::shared_ptr<iResource>  CreateTextureResource(const std::filesystem::path& path, const std::string& formatString);
    std::shared_ptr<iResource>  CreateTextureResource(std::shared_ptr<Texture_MetaData> metaData);
    std::shared_ptr<iResource>  CreateTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string format);
    std::shared_ptr<iResource>  CreateCompressedTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string format);


}    /************************************************************************************************/



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
