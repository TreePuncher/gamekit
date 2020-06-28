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


namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    FlexKit::DeviceFormat FormatStringToDeviceFormat(const std::string& format)
    {
        static std::map<std::string, FlexKit::DeviceFormat> formatMap = {
            { "RGBA8_UNORM",    DeviceFormat::R8G8B8A8_UNORM       },
            { "RGBA16_FLOAT",   DeviceFormat::R16G16B16A16_FLOAT   },
            { "RGBA32_FLOAT",   DeviceFormat::R32G32B32A32_FLOAT   },
        };

        return formatMap[format];
    }

    /************************************************************************************************/


    struct CubeMapFace
    {
        std::vector<FlexKit::TextureBuffer> mipLevels;

        Blob CreateBlob();
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


        ResourceBlob CreateBlob() override;
    };


    inline std::shared_ptr<iResource> CreateCubeMapResource(std::shared_ptr<TextureCubeMap_MetaData> metaData);


    /************************************************************************************************/


    void CreateCompressedDDSTextureResource()
    {
    }


    /************************************************************************************************/


    enum class DDSTextureFormat
    {
        DXT5,
        UNKNOWN
    };


    /************************************************************************************************/


    DDSTextureFormat FormatStringToFormatID(const std::string& ID)
    {
        if (ID == "DXT5")
            return DDSTextureFormat::DXT5;

        return DDSTextureFormat::UNKNOWN;
    }


    /************************************************************************************************/


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


    /************************************************************************************************/


    class TextureResource : public iResource
    {
    public:
        TextureResource() {}

        ResourceBlob CreateBlob() override;

        std::string         ID;
        GUID_t              assetHandle;
        FlexKit::DeviceFormat  format;

        void*     buffer;
        size_t    bufferSize;
    };


    /************************************************************************************************/


    _TextureMipLevelResource CreateMIPMapResource(const std::string& string);
    inline std::shared_ptr<iResource> CreateTextureResource(std::shared_ptr<Texture_MetaData> metaData);


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
