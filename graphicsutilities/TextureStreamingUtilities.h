#include "memoryutilities.h"
#include "TextureUtilities.h"
#include "graphics.h"

#define CRND_HEADER_FILE_ONLY
#include <crn_decomp.h>

namespace FlexKit
{   /************************************************************************************************/

    class iDecompressor
    {
    public:
        virtual ~iDecompressor() {}

        virtual UploadReservation ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx) = 0;
    };

    class CRNDecompressor final : public iDecompressor
    {
    public:
        CRNDecompressor(const uint32_t mipLevel, AssetHandle asset, iAllocator* IN_allocator) :
                allocator{ IN_allocator }
        {
            TextureResourceBlob* resource = reinterpret_cast<TextureResourceBlob*>(FlexKit::GetAsset(asset));

            crnd::crn_level_info levelInfo;
            crnd::crnd_get_level_info(resource->GetBuffer(), resource->GetBufferSize(), mipLevel, &levelInfo);

            rowPitch   = levelInfo.m_blocks_x * levelInfo.m_bytes_per_block;
            bufferSize = levelInfo.m_blocks_y * rowPitch;

            blockSize   = levelInfo.m_bytes_per_block;
            blocks_X    = levelInfo.m_blocks_x;
            blocks_Y    = levelInfo.m_blocks_y;

            buffer = (char*)allocator->malloc(bufferSize);

            void* data[1]   = { (void*)buffer};

            crnd::crnd_unpack_context   crn_context = crnd::crnd_unpack_begin(resource->GetBuffer(), resource->GetBufferSize());
            auto res    = crnd::crnd_unpack_level(crn_context, data, bufferSize, rowPitch, mipLevel);
            crnd::crnd_unpack_end(crn_context);

            FreeAsset(asset);
        }



        ~CRNDecompressor()
        {
            allocator->free(buffer);
        }


        UploadReservation ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx) override
        {
            auto reservation    = ctx.Reserve(64 * KILOBYTE);
            auto blocksX        = TileSize[0] / 4;
            auto blocksY        = TileSize[1] / 4;
            auto localRowPitch  = blockSize * blocksX;

            const auto tileX = id.GetTileX();
            const auto tileY = id.GetTileY();

            /*
            for (size_t row = 0; row < 64; ++row)
                for (size_t column = 0; column < 16; ++column)
                    memcpy(
                        reservation.buffer + blockSize * 256 * column + blockSize * row * 4,
                        buffer + blockSize * column * 4 + rowPitch * row + tileY * rowPitch * blocksY + tileX * localRowPitch,
                        blockSize * 4);
            */

            for (size_t row = 0; row < blocksY; ++row)
                memcpy(
                    reservation.buffer + row * localRowPitch,
                    buffer + row * rowPitch + tileX * localRowPitch,
                    localRowPitch);

            return reservation;
        }

        uint32_t                    blocks_X;
        uint32_t                    blocks_Y;
        uint32_t                    blockSize;
        char*                       buffer;
        size_t                      rowPitch;
        size_t                      bufferSize;
        iAllocator*                 allocator;
    };


    /************************************************************************************************/


    inline std::optional<CRNDecompressor*> CreateCRNDecompressor(const uint32_t MIPlevel, AssetHandle asset, iAllocator* allocator)
    {
        TextureResourceBlob* resource   = reinterpret_cast<TextureResourceBlob*>(FlexKit::GetAsset(asset));
        auto buffer                     = resource->GetBuffer();
        const auto bufferSize           = resource->GetBufferSize();
       
        EXITSCOPE(FreeAsset(asset));

        crnd::crn_texture_info info;
        if (crnd::crnd_get_texture_info(buffer, resource->GetBufferSize(), &info))
        {
            crnd::crn_level_info levelInfo;

            if (!crnd::crnd_get_level_info(buffer, bufferSize, 0, &levelInfo))
                return {};

            return &allocator->allocate<CRNDecompressor>(MIPlevel, asset, allocator);
        }
        else
            return {};
    }


    /************************************************************************************************/


    bool IsDDS(DeviceFormat format)
    {
        switch (format)
        {
        case DeviceFormat::BC1_TYPELESS:
        case DeviceFormat::BC1_UNORM:
        case DeviceFormat::BC1_UNORM_SRGB:
        case DeviceFormat::BC2_TYPELESS:
        case DeviceFormat::BC2_UNORM:
        case DeviceFormat::BC2_UNORM_SRGB:
        case DeviceFormat::BC3_TYPELESS:
        case DeviceFormat::BC3_UNORM:
        case DeviceFormat::BC3_UNORM_SRGB:
        case DeviceFormat::BC4_TYPELESS:
        case DeviceFormat::BC4_UNORM:
        case DeviceFormat::BC4_SNORM:
        case DeviceFormat::BC5_TYPELESS:
        case DeviceFormat::BC5_UNORM:
        case DeviceFormat::BC5_SNORM:
            return true;
        default:
            return false;
        }
    }


    /************************************************************************************************/


    struct TextureStreamContext
    {
        TextureStreamContext(iAllocator* IN_allocator) :
            allocator{ IN_allocator } {}


        ~TextureStreamContext()
        {
            Close();
        }


        bool Open(const uint32_t MIPlevel, AssetHandle asset)
        {
            auto textureAvailable   = isAssetAvailable(asset);
            auto assetHandle        = LoadGameAsset(asset);
            EXITSCOPE(FreeAsset(assetHandle));

            TextureResourceBlob* resource   = reinterpret_cast<TextureResourceBlob*>(FlexKit::GetAsset(assetHandle));


            if (IsDDS((DeviceFormat)resource->format))
            {
                Close();

                auto res        = CreateCRNDecompressor(MIPlevel, assetHandle, allocator);
                decompressor    = res.value_or(nullptr);

                return res.has_value();
            }
            else
            {
                FK_ASSERT(0);
            }

            return false;
        }

        void Close()
        {
            if (decompressor)
                allocator->release(*decompressor);
        }


        UploadReservation ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx)
        {
            return decompressor->ReadTile(id, TileSize, ctx);
        }


        uint2 GetBlockSize() const
        {
            return { 256, 256 };
        }


        DeviceFormat    format;
        iDecompressor*  decompressor = nullptr;
        AssetHandle     currentAsset = -1;
        iAllocator*     allocator;
    };


}   /************************************************************************************************/

/**********************************************************************

Copyright (c) 2020 Robert May

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
