#ifndef TEXTURESTREAMINGUTILITIES_H
#define TEXTURESTREAMINGUTILITIES_H

#include "Memoryutilities.h"
#include "TextureUtilities.h"
#include "ThreadUtilities.h"
#include "GraphicScene.h"

#define CRND_HEADER_FILE_ONLY
#include <crn_decomp.h>

namespace FlexKit
{   /************************************************************************************************/


    class RenderSystem;

    /************************************************************************************************/


    class iDecompressor
    {
    public:
        virtual ~iDecompressor() {}

        virtual UploadReservation   ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx) = 0;
        virtual UploadReservation   Read(const uint2 WH, CopyContext& ctx) = 0;
        virtual uint2               GetTextureWH() const = 0;

    };


    class CRNDecompressor final : public iDecompressor
    {
    public:
        CRNDecompressor(const uint32_t mipLevel, AssetHandle asset, iAllocator* IN_allocator);
        ~CRNDecompressor();
 

        UploadReservation   ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx) override;
        UploadReservation   Read(const uint2 WH, CopyContext& ctx) override;
        uint2               GetTextureWH() const override;

        uint32_t                    blocks_X;
        uint32_t                    blocks_Y;
        uint32_t                    blockSize;
        char*                       buffer;
        size_t                      rowPitch;
        size_t                      bufferSize;
        uint2                       WH;
        iAllocator*                 allocator;
    };


    /************************************************************************************************/


    std::optional<CRNDecompressor*> CreateCRNDecompressor(const uint32_t MIPlevel, AssetHandle asset, iAllocator* allocator);


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

    size_t GetFormatTileSizeWith(DeviceFormat)
    {
        // TODO: actually fill this out, this is only correct *most* of the time
        return 256;
    }

    struct DDSLevelInfo
    {
        size_t  RowPitch = 0;
        uint2   WH;
        bool    tiled;
    };

    struct DDSInfo
    {
        size_t  MIPCount;
        uint2   WH;

        DeviceFormat format;
    };

    DDSInfo GetDDSInfo(AssetHandle asset);

    DDSLevelInfo GetMIPLevelInfo(const size_t Level, const uint2 WH, const DeviceFormat format)
    {
        // TODO: actually figure out the row pitch and tile size, and width

        const uint32_t levelWidth = max(WH[0] >> Level, 4);

        if (levelWidth > GetFormatTileSizeWith(format))
        {
            return { -1u, { levelWidth, levelWidth }, false };
        }
        else
        {
            return { -1u,  { levelWidth, levelWidth }, true };
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
            if (MIPlevel == currentLevel && asset == currentAsset)
                return true;

            auto textureAvailable   = isAssetAvailable(asset);
            auto assetHandle        = LoadGameAsset(asset);
            EXITSCOPE(FreeAsset(assetHandle));

            TextureResourceBlob* resource   = reinterpret_cast<TextureResourceBlob*>(FlexKit::GetAsset(assetHandle));

            if (IsDDS((DeviceFormat)resource->format))
            {
                Close();

                auto res        = CreateCRNDecompressor(MIPlevel, assetHandle, allocator);
                decompressor    = res.value_or(nullptr);
                format          = DeviceFormat::BC3_UNORM; // TODO: correctly extract the format
                currentLevel    = MIPlevel;
                currentAsset    = asset;

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


        UploadReservation Read(const uint2 WH, CopyContext& ctx)
        {
            return decompressor->Read(WH, ctx);
        }


        uint2 GetBlockSize() const
        {
            return { 256, 256 };
        }


        DeviceFormat Format() const
        {
            return format;
        }

        uint2 WH() const
        {
            return decompressor->GetTextureWH();
        }

        DeviceFormat    format;
        iDecompressor*  decompressor = nullptr;
        AssetHandle     currentAsset = -1;
        uint8_t         currentLevel = -1;
        iAllocator*     allocator;
    };


    inline const PSOHandle TEXTUREFEEDBACKPASS          = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKPASS));
    inline const PSOHandle TEXTUREFEEDBACKCLEAR         = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKCLEAR));
    inline const PSOHandle TEXTUREFEEDBACKCOMPRESSOR    = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKCOMPRESSOR));

    ID3D12PipelineState* CreateTextureFeedbackPassPSO(RenderSystem* RS);
    ID3D12PipelineState* CreateTextureFeedbackClearPSO(RenderSystem* RS);
    ID3D12PipelineState* CreateTextureFeedbackCompressorPSO(RenderSystem* RS);

    
    /************************************************************************************************/


    struct TextureFeedbackPass_Data
    {
        CameraHandle                    camera;
        GatherTask&                     pvs;
        ReserveConstantBufferFunction   reserveCB;
        
        FrameResourceHandle             feedbackCounters;
        FrameResourceHandle             feedbackBuffer;
        FrameResourceHandle             feedbackLists;
        FrameResourceHandle             feedbackDepth;
        FrameResourceHandle             feedbackOffsets;
        FrameResourceHandle             feedbackPPLists;

        FrameResourceHandle             feedbackOutputTemp;
        FrameResourceHandle             feedbackOutputFinal;
        FrameResourceHandle             feedbackPPLists_Temp1;
        FrameResourceHandle             feedbackPPLists_Temp2;


        ReadBackResourceHandle          readbackBuffer;
    };


	/************************************************************************************************/


	constexpr size_t GetMinBlockSize()
	{
		return 64 * KILOBYTE;
	}

	struct TextureCacheDesc
	{
		const size_t textureCacheSize	= GIGABYTE;
		const size_t blockSize			= GetMinBlockSize();
	};


	/************************************************************************************************/


    struct AllocatedBlock
    {
        TileID_t        tileID;
        ResourceHandle  resource;
        uint32_t        offset;
        uint32_t        tileIdx;
    };

    struct gpuTileID
    {
        TileID_t tileID;
        uint32_t TextureID;
        uint32_t prev;// Unused

        uint64_t GetSortingID() const
        {
            return (uint64_t(TextureID) << 32) | tileID.bytes;
        }

        bool IsPacked() const
        {
            return tileID.packed();
        }

        operator uint64_t () const { return GetSortingID(); }
    };


    using AllocatedBlockList    = Vector<AllocatedBlock>;
    using gpuTileList           = Vector<gpuTileID>;

    struct BlockAllocation
    {
        AllocatedBlockList  reallocations;
        AllocatedBlockList  allocations;
        AllocatedBlockList  packedAllocations;
    };

	class TextureBlockAllocator
	{
	public:

        struct Block
        {
            TileID_t        tileID;
            ResourceHandle  resource            = InvalidHandle_t;
            uint8_t         state               = EBlockState::Free;
            uint8_t         packed              = 0;
            uint8_t         parentLevel         = -1;
            uint8_t         packedLevelCount    = -1;
            uint32_t        blockID;

            operator uint64_t() const
            {
                return ((uint64_t)resource.to_uint()) << 32 | tileID;
            }
        };

        enum EBlockState : uint8_t
        {
            Free,
            InUse,
            Stale, // Free for reallocation
        };


        TextureBlockAllocator(const size_t blockCount, iAllocator* IN_allocator);

        Vector<gpuTileID>   UpdateTileStates    (const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator);
        BlockAllocation     AllocateBlocks      (const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator);

        const static uint32_t   blockSize   = 64 * KILOBYTE;
        uint32_t                last        = 0;

        Vector<Block>           blockTable;

		iAllocator*             allocator;
	};


    /************************************************************************************************/



    class FLEXKITAPI TextureStreamingEngine
	{
	public:
		TextureStreamingEngine(RenderSystem&, iAllocator* IN_allocator = SystemAllocator, const TextureCacheDesc& desc    = {});


        ~TextureStreamingEngine();


        void TextureFeedbackPass(
            UpdateDispatcher&                   dispatcher,
            FrameGraph&                         frameGraph,
            CameraHandle                        camera,
            uint2                               renderTargetWH,
            UpdateTaskTyped<GetPVSTaskData>&    sceneGather,
            ReserveConstantBufferFunction&      reserveCB,
            ReserveVertexBufferFunction&        reserveVB);


        struct TextureStreamUpdate : public FlexKit::iWork
        {
            TextureStreamUpdate(ReadBackResourceHandle resource, TextureStreamingEngine& IN_textureStreamEngine, iAllocator* IN_allocator);

            void Run(iAllocator& threadLocalAllocator) override;

            void Release() override
            {
                textureStreamEngine.MarkUpdateCompleted();
                allocator->free(this);
            }

            TextureStreamingEngine& textureStreamEngine;
            ReadBackResourceHandle  resource;

            iAllocator* allocator       = nullptr;
        };


        gpuTileList     UpdateTileStates    (const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator);
        BlockAllocation AllocateTiles       (const gpuTileID* begin, const gpuTileID* end);

        void                        BindAsset           (const AssetHandle textureAsset, const ResourceHandle  resource);
        std::optional<AssetHandle>  GetResourceAsset    (const ResourceHandle  resource) const;

        void PostUpdatedTiles(const BlockAllocation& blocks, iAllocator& threadLocalAllocator);
        void MarkUpdateCompleted() { updateInProgress = false; taskInProgress = false; }

	private:

        inline static const size_t OffsetBufferSize = 240 * 240 * sizeof(uint32_t);

        struct MappedAsset
        {
            ResourceHandle  resource;
            AssetHandle     textureAsset;

            auto GetResourceID() const
            {
                return resource.to_uint();
            }


            uint64_t GetID() const
            {
                return (uint64_t)resource.to_uint() << 32 | textureAsset;
            }
        };

		RenderSystem&			renderSystem;

        std::atomic_bool        updateInProgress    = false;
        std::atomic_bool        taskInProgress      = false;

        UAVResourceHandle		feedbackOffsets;    // GPU
        UAVTextureHandle        feedbackPPLists;    // GPU
        UAVResourceHandle		feedbackBuffer;     // GPU
        UAVResourceHandle       feedbackCounters;   // GPU

        ResourceHandle		    feedbackDepth;  // GPU

        UAVResourceHandle		feedbackOutputTemp;   // GPU
        UAVResourceHandle		feedbackOutputFinal;  // GPU
        UAVTextureHandle        feedbackTemp1;        // GPU
        UAVTextureHandle        feedbackTemp2;        // GPU

        ReadBackResourceHandle  feedbackReturnBuffer; // CPU + GPU

        Vector<MappedAsset>     mappedAssets;
		TextureBlockAllocator	textureBlockAllocator;
        DeviceHeapHandle        heap;

        iAllocator*             allocator;
		const TextureCacheDesc	settings;
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


#endif
