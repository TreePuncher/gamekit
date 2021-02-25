#include "TextureResourceUtilities.h"
#include <filesystem>
#include <numeric>


#ifdef _DEBUG
#pragma comment(lib, "CMP_Framework_MDd.lib")
#else
#ifdef NDEBUG
#pragma comment(lib, "CMP_Framework_MD.lib")
#endif
#endif

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    inline FlexKit::DeviceFormat FormatStringToDeviceFormat(const std::string& format)
    {
        static std::map<std::string, FlexKit::DeviceFormat> formatMap = {
            { "RGBA8_UNORM",    DeviceFormat::R8G8B8A8_UNORM       },
            { "RGBA16_FLOAT",   DeviceFormat::R16G16B16A16_FLOAT   },
            { "RGBA32_FLOAT",   DeviceFormat::R32G32B32A32_FLOAT   },
            { "BC3",           DeviceFormat::BC3_UNORM            },
            { "BC5",           DeviceFormat::BC5_UNORM            },
            { "BC7",           DeviceFormat::BC7_UNORM            },

            { "DXT3",           DeviceFormat::BC3_UNORM            },
            { "DXT5",           DeviceFormat::BC5_UNORM            },
            { "DXT7",           DeviceFormat::BC7_UNORM            },
        };

        return formatMap[format];
    }


    /************************************************************************************************/

    Blob CubeMapFace::CreateBlob()
    {
        size_t currentOffset = sizeof(size_t) + sizeof(size_t) * mipLevels.size() * 2;
        const size_t mipCount = mipLevels.size();

        Blob offsets;
        Blob sizes;
        Blob Body;

        for (auto& mipLevel : mipLevels)
        {
            offsets += Blob{ currentOffset };
            sizes += Blob{ mipLevel.Size };
            currentOffset += mipLevel.Size;

            Blob textureBuffer;
            textureBuffer.resize(mipLevel.BufferSize());
            memcpy(textureBuffer.data(), mipLevel.Buffer, mipLevel.Size);

            Body += textureBuffer;
        }

        return Blob{ mipCount } +offsets + sizes + Body;
    }


    /************************************************************************************************/


    ResourceBlob CubeMapTexture::CreateBlob()
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


    /************************************************************************************************/


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


    /************************************************************************************************/


    ResourceBlob TextureResource::CreateBlob()
    {
        TextureResourceBlob headerData;

        headerData.format       = format;
        headerData.ResourceSize = sizeof(headerData) + bufferSize;
        headerData.GUID         = assetHandle;
        headerData.WH           = WH;
        headerData.mipLevels    = mipLevels;

        memcpy(headerData.mipOffsets, mipOffsets, sizeof(mipOffsets));

        Blob header { headerData };
        Blob body   = { (const char*)buffer, bufferSize };

        strncpy(headerData.ID, ID.c_str(), Min(sizeof(headerData.ID), ID.size()));

        auto [data, size]   = (header + body).Release();

        ResourceBlob out;
        out.buffer          = (char*)data;
        out.bufferSize      = size;
        out.GUID            = assetHandle;
        out.ID              = ID;
        out.resourceType    = EResourceType::EResource_Texture;

        std::cout << "_DEBUG: " __FUNCTION__ << " : Size : " << out.bufferSize << "\n";
        std::cout << "_DEBUG: " __FUNCTION__ << " : MipCount : " << mipLevels << "\n";

        return out;
    }


    /************************************************************************************************/


    bool CompressionCallback(CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2) {
        (pUser1);
        (pUser2);

        std::printf("\rCompression progress = %2.0f  ", fProgress);

        return 0;
    }

    _TextureMipLevelResource CreateMIPMapResource(const std::string& string)
    {
        CMP_MipSet mipSet;
        memset(&mipSet, 0, sizeof(CMP_MipSet));

        if(auto res = CMP_LoadTexture(string.c_str(), &mipSet); res != CMP_OK)
        {
            std::printf("Error %d: Loading source file!\n", res);
            return {};
        }

        KernelOptions kernel_options;
        memset(&kernel_options, 0, sizeof(KernelOptions));


        kernel_options.encodeWith   = CMP_GPU_DXC;
        kernel_options.format       = CMP_FORMAT_BC7;
        kernel_options.fquality     = 0.05f;
        kernel_options.threads      = 0;

        auto progress = [](CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)-> bool
        {
            //std::cout << "Progress: " << fProgress * 100 << "\n";
            return true;
        };

        CMP_MipSet dstMipSet = { 0 };

        auto cmp_status = CMP_ProcessTexture(&mipSet, &dstMipSet, kernel_options, CompressionCallback);

        return {
            mipSet,
            string
        };
    }


    /************************************************************************************************/


    inline std::shared_ptr<iResource> CreateTextureResource(const std::filesystem::path& path, const std::string& formatString)
    {
        std::printf("building texture resource\n");

        CMP_MipSet mipSet;
        memset(&mipSet, 0, sizeof(CMP_MipSet));

        if (auto res = CMP_LoadTexture(path.string().c_str(), &mipSet); res != CMP_OK)
        {
            std::printf("Error %d: Loading source file!\n", res);
            return {};
        }

        const size_t    temp2       = (size_t)std::log2(mipSet.m_nHeight);
        const CMP_INT   nMinSize    = CMP_CalcMinMipSize(mipSet.m_nHeight, mipSet.m_nWidth, mipSet.m_nMaxMipLevels);

        if (auto res = CMP_GenerateMIPLevels(&mipSet, nMinSize); res != CMP_OK)
        {
            std::printf("Error %d: Failed to create Mip Levels!\n", res);
            return {};
        }


#if 0
        std::cout << "DEBUG: nMinSize: " << nMinSize << "\n";
        std::cout << "DEBUG: INPUT MIP LEVELS: " << mipSet.m_nMipLevels << "\n";
#endif

        KernelOptions kernel_options;
        memset(&kernel_options, 0, sizeof(KernelOptions));

        kernel_options.encodeWith = CMP_HPC;
        kernel_options.format     = CMP_FORMAT_BC7;
        kernel_options.fquality   = 0.001f;
        kernel_options.threads    = 0;

        CMP_MipSet dstMipSet = { 0 };

        auto ddsformat = FormatStringToFormatID(formatString);

        switch (ddsformat)
        {
        case DDSTextureFormat::DXT3:
            kernel_options.format = CMP_FORMAT_BC3;
            break;
        case DDSTextureFormat::DXT5:
            kernel_options.format = CMP_FORMAT_BC5;
                break;
        case DDSTextureFormat::DXT7:
            kernel_options.format = CMP_FORMAT_BC7;
            break;
        default:
            return {};
        }

        auto progress = [](CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)-> bool
        {
            printf("\rProgress: %f", fProgress);
            return false;
        };


        //if (CMP_LoadTexture(std::string(path.string() + ".cached.dds").c_str(), &dstMipSet) != CMP_ERROR::CMP_OK)
        {
            auto cmp_status = CMP_ProcessTexture(&mipSet, &dstMipSet, kernel_options, progress);
            CMP_SaveTexture(std::string(path.string() + ".cached.dds").c_str(), &dstMipSet);
        }

        printf("\n");

        auto resource = std::make_shared<TextureResource>();

        int bufferSize = 0;
        for (int I = 0, offset = 0; I < dstMipSet.m_nMipLevels; I++) {
            CMP_MipLevel* mipLevel = nullptr;
            CMP_GetMipLevel(&mipLevel, &dstMipSet, I, 0);

            bufferSize += mipLevel->m_dwLinearSize;
#if 0
            std::cout << "_DEBUG: " __FUNCTION__ << " : Mip Size : " << mipLevel->m_dwLinearSize << "\n";
#endif
        }

        auto temp = new char[bufferSize];
        memset(resource->mipOffsets, 0, sizeof(TextureResource::mipOffsets));

        for (int I = 0, offset = 0; I < dstMipSet.m_nMipLevels; I++)
        {
            CMP_MipLevel* mipLevel = nullptr;
            CMP_GetMipLevel(&mipLevel, &dstMipSet, I, 0);

            resource->mipOffsets[I] = offset;

#if 0
            std::cout << "_DEBUG: " __FUNCTION__ << " : LinearSize : " << mipLevel->m_dwLinearSize << "\n";
#endif

            memcpy((char*)temp + offset, mipLevel->m_pbData, mipLevel->m_dwLinearSize);
            offset += mipLevel->m_dwLinearSize;
        }
        resource->ID          = path.filename().string();
        resource->WH          = { dstMipSet.m_nWidth, dstMipSet.m_nHeight };
        resource->assetHandle = rand();
        resource->buffer      = temp;
        resource->bufferSize  = bufferSize;
        resource->format      = FormatStringToDeviceFormat(formatString);
        resource->mipLevels   = dstMipSet.m_nMipLevels;

#if 0
        std::cout << "_DEBUG: " __FUNCTION__ << " : Size : " << resource->bufferSize << "\n";
        std::cout << "_DEBUG: " __FUNCTION__ << " : MipCount : " << dstMipSet.m_nMipLevels << "\n";
#endif

        CMP_FreeMipSet(&mipSet);
        CMP_FreeMipSet(&dstMipSet);

        return resource;
    }


    /************************************************************************************************/


    inline std::shared_ptr<iResource> CreateTextureResource(std::shared_ptr<Texture_MetaData> metaData)
    {
        std::printf("building texture resource\n");

        auto format = FormatStringToFormatID(metaData->format);

        CMP_MipSet mipSet;
        memset(&mipSet, 0, sizeof(CMP_MipSet));

        if (auto res = CMP_LoadTexture(metaData->file.c_str(), &mipSet); res != CMP_OK)
        {
            std::printf("Error %d: Loading source file!\n", res);
            return {};
        }

        CMP_INT nMinSize = CMP_CalcMinMipSize(mipSet.m_nHeight, mipSet.m_nWidth, mipSet.m_nMaxMipLevels);

        if (auto res = CMP_GenerateMIPLevels(&mipSet, nMinSize); res != CMP_OK)
        {
            std::printf("Error %d: Failed to create Mip Levels!\n", res);
            return {};
        }

        std::cout << "DEBUG: nMinSize: " << nMinSize << "\n";
        std::cout << "DEBUG: INPUT MIP LEVELS: " << mipSet.m_nMipLevels << "\n";

        KernelOptions kernel_options;
        memset(&kernel_options, 0, sizeof(KernelOptions));

        kernel_options.encodeWith = CMP_HPC;
        kernel_options.format     = CMP_FORMAT_BC7;
        kernel_options.fquality   = 0.05f;
        kernel_options.threads    = 0;

        CMP_MipSet dstMipSet = { 0 };

        switch (format)
        {
        case DDSTextureFormat::DXT3:
            kernel_options.format = CMP_FORMAT_BC3;
            break;
        case DDSTextureFormat::DXT5:
            kernel_options.format = CMP_FORMAT_BC5;
                break;
        case DDSTextureFormat::DXT7:
            kernel_options.format = CMP_FORMAT_BC7;
            break;
        default:
            return {};
        }

        auto progress = [](CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)-> bool
        {
            printf("\rProgress: %f\n", fProgress);
            return false;
        };

        auto cmp_status = CMP_ProcessTexture(&mipSet, &dstMipSet, kernel_options, progress);
        CMP_SaveTexture(std::string(metaData->file + ".cached.dds").c_str(), &dstMipSet);

        auto resource = std::make_shared<TextureResource>();
            
        size_t bufferSize = 0;
        for (int I = 0, offset = 0; I < dstMipSet.m_nMipLevels; I++) {
            CMP_MipLevel* mipLevel = nullptr;
            CMP_GetMipLevel(&mipLevel, &dstMipSet, I, 0);

            bufferSize += mipLevel->m_dwLinearSize;

            std::cout << "_DEBUG: " __FUNCTION__ << " : Mip Size : " << mipLevel->m_dwLinearSize << "\n";
        }

        auto temp = new char[bufferSize];
        memset(resource->mipOffsets, 0, sizeof(TextureResource::mipOffsets));

        for (int I = 0, offset = 0; I < dstMipSet.m_nMipLevels; I++)
        {
            CMP_MipLevel* mipLevel = nullptr;
            CMP_GetMipLevel(&mipLevel, &dstMipSet, I, 0);

            resource->mipOffsets[I] = offset;

            std::cout << "_DEBUG: " __FUNCTION__ << " : LinearSize : " << mipLevel->m_dwLinearSize << "\n";

            memcpy((char*)temp + offset, mipLevel->m_pbData, mipLevel->m_dwLinearSize);
            offset += mipLevel->m_dwLinearSize;
        }

        resource->ID          = metaData->stringID;
        resource->WH          = { dstMipSet.m_nWidth, dstMipSet.m_nHeight };
        resource->assetHandle = metaData->assetID;
        resource->buffer      = temp;
        resource->bufferSize  = bufferSize;
        resource->format      = FormatStringToDeviceFormat(metaData->format);
        resource->mipLevels   = dstMipSet.m_nMipLevels;

        std::cout << "_DEBUG: " __FUNCTION__ << " : Size : " << resource->bufferSize << "\n";
        std::cout << "_DEBUG: " __FUNCTION__ << " : MipCount : " << dstMipSet.m_nMipLevels << "\n";

        CMP_FreeMipSet(&mipSet);
        CMP_FreeMipSet(&dstMipSet);

        return resource;
    }


    /************************************************************************************************/


    std::shared_ptr<iResource>  CreateTextureResource(FlexKit::TextureBuffer& textureBuffer, std::string formatStr)
    {
        FK_ASSERT(false, "Not Implemented");

        std::printf("building texture resource\n");

        auto format = FormatStringToFormatID(formatStr);

        CMP_MipSet mipSet;
        memset(&mipSet, 0, sizeof(CMP_MipSet));
        mipSet.dwDataSize           = (uint32_t)textureBuffer.BufferSize();
        mipSet.m_nHeight            = textureBuffer.WH[1];
        mipSet.m_nWidth             = textureBuffer.WH[0];
        mipSet.m_nMaxMipLevels      = (uint32_t)std::log2(std::min(textureBuffer.WH[1], textureBuffer.WH[0]));
        mipSet.pData                = textureBuffer.Buffer;
        mipSet.m_nMipLevels         = 1;
        mipSet.m_format             = CMP_FORMAT_RGBA_8888;
        mipSet.m_nDepth             = 1;
        mipSet.m_TextureDataType    = TDT_ARGB;

        CMP_MipLevel        mipLevels[12]   = { 0 };
        CMP_MipLevelTable   levels[12]      = { 0 };

        mipLevels[0].m_nHeight       = textureBuffer.WH[1];
        mipLevels[0].m_nWidth        = textureBuffer.WH[0];
        mipLevels[0].m_dwLinearSize  = (uint32_t)textureBuffer.BufferSize();
        mipLevels[0].m_pcData        = (CMP_COLOR*)textureBuffer.Buffer;

        levels[0] = &mipLevels[0];

        mipSet.m_pMipLevelTable      = levels;

        CMP_INT nMinSize    = CMP_CalcMinMipSize(mipSet.m_nHeight, mipSet.m_nWidth, mipSet.m_nMaxMipLevels);

        if (auto res = CMP_GenerateMIPLevels(&mipSet, nMinSize); res != CMP_OK)
        {
            std::printf("Error %d: Failed to create Mip Levels!\n", res);
            return {};
        }

        std::cout << "DEBUG: nMinSize: " << nMinSize << "\n";
        std::cout << "DEBUG: INPUT MIP LEVELS: " << mipSet.m_nMipLevels << "\n";

        KernelOptions kernel_options;
        memset(&kernel_options, 0, sizeof(KernelOptions));

        kernel_options.encodeWith = CMP_HPC;
        kernel_options.format     = CMP_FORMAT_BC7;
        kernel_options.fquality   = 0.05f;
        kernel_options.threads    = 0;

        CMP_MipSet dstMipSet = { 0 };

        switch (format)
        {
        case DDSTextureFormat::DXT3:
            kernel_options.format = CMP_FORMAT_BC3;
            break;
        case DDSTextureFormat::DXT5:
            kernel_options.format = CMP_FORMAT_BC5;
                break;
        case DDSTextureFormat::DXT7:
            kernel_options.format = CMP_FORMAT_BC7;
            break;
        default:
            return {};
        }

        auto progress = [](CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)-> bool
        {
            printf("\rProgress: %f\n", fProgress);
            return false;
        };

        auto cmp_status = CMP_ProcessTexture(&mipSet, &dstMipSet, kernel_options, progress);
        auto resource = std::make_shared<TextureResource>();
            
        size_t bufferSize = 0;
        for (int I = 0, offset = 0; I < dstMipSet.m_nMipLevels; I++) {
            CMP_MipLevel* mipLevel = nullptr;
            CMP_GetMipLevel(&mipLevel, &dstMipSet, I, 0);

            bufferSize += mipLevel->m_dwLinearSize;

            std::cout << "_DEBUG: " __FUNCTION__ << " : Mip Size : " << mipLevel->m_dwLinearSize << "\n";
        }

        auto temp = new char[bufferSize];
        memset(resource->mipOffsets, 0, sizeof(TextureResource::mipOffsets));

        for (int I = 0, offset = 0; I < dstMipSet.m_nMipLevels; I++)
        {
            CMP_MipLevel* mipLevel = nullptr;
            CMP_GetMipLevel(&mipLevel, &dstMipSet, I, 0);

            resource->mipOffsets[I] = offset;

            std::cout << "_DEBUG: " __FUNCTION__ << " : LinearSize : " << mipLevel->m_dwLinearSize << "\n";

            memcpy((char*)temp + offset, mipLevel->m_pbData, mipLevel->m_dwLinearSize);
            offset += mipLevel->m_dwLinearSize;
        }

        resource->ID          = "";
        resource->WH          = { dstMipSet.m_nWidth, dstMipSet.m_nHeight };
        resource->assetHandle = 0;
        resource->buffer      = temp;
        resource->bufferSize  = bufferSize;
        resource->format      = FormatStringToDeviceFormat(formatStr);
        resource->mipLevels   = dstMipSet.m_nMipLevels;

        std::cout << "_DEBUG: " __FUNCTION__ << " : Size : " << resource->bufferSize << "\n";
        std::cout << "_DEBUG: " __FUNCTION__ << " : MipCount : " << dstMipSet.m_nMipLevels << "\n";

        CMP_FreeMipSet(&mipSet);
        CMP_FreeMipSet(&dstMipSet);

        return {};
    }


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
