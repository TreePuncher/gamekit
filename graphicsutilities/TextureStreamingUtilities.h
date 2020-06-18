#ifndef TEXTURESTREAMINGUTILITIES_H
#define TEXTURESTREAMINGUTILITIES_H

#include "Memoryutilities.h"
#include "TextureUtilities.h"
#include "ThreadUtilities.h"


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

        virtual UploadReservation ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx) = 0;
    };


    class CRNDecompressor final : public iDecompressor
    {
    public:
        CRNDecompressor(const uint32_t mipLevel, AssetHandle asset, iAllocator* IN_allocator);
        ~CRNDecompressor();
 

        UploadReservation ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx) override;


        uint32_t                    blocks_X;
        uint32_t                    blocks_Y;
        uint32_t                    blockSize;
        char*                       buffer;
        size_t                      rowPitch;
        size_t                      bufferSize;
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

        operator uint64_t () const { return GetSortingID(); }
    };


    using AllocatedBlockList    = Vector<AllocatedBlock>;
    using gpuTileList           = Vector<gpuTileID>;

    struct BlockAllocation
    {
        AllocatedBlockList  reallocations;
        AllocatedBlockList  allocations;
    };

	class TextureBlockAllocator
	{
	public:

        struct Block
        {
            TileID_t        tileID;
            ResourceHandle  resource;
            uint32_t        state;
            uint32_t        blockID;

            operator uint64_t() const
            {
                return ((uint64_t)resource.to_uint()) << 32 | tileID;
            }
        };

        enum EBlockState
        {
            Free,
            InUse,
            Stale// Free for reallocation
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
		TextureStreamingEngine(
                RenderSystem&   IN_renderSystem,
                const uint2     IN_WH,
                iAllocator*     IN_allocator    = SystemAllocator,
                const TextureCacheDesc& desc    = {}) : 
			allocator		        { IN_allocator		},
            textureBlockAllocator   { desc.textureCacheSize / desc.blockSize,   IN_allocator },
			renderSystem	        { IN_renderSystem	},
			settings		        { desc				},
            feedbackOffsets         { IN_renderSystem.CreateUAVBufferResource(OffsetBufferSize) },
            feedbackOutputTemp      { IN_renderSystem.CreateUAVBufferResource(1 * MEGABYTE) },
            feedbackOutputFinal     { IN_renderSystem.CreateUAVBufferResource(1 * MEGABYTE) },
            feedbackPPLists         { IN_renderSystem.CreateUAVTextureResource({256, 256}, DeviceFormat::R32_UINT)  },
            feedbackTemp1           { IN_renderSystem.CreateUAVTextureResource({128, 128}, DeviceFormat::R32_UINT)  },
            feedbackTemp2           { IN_renderSystem.CreateUAVTextureResource({128, 128}, DeviceFormat::R32_UINT)  },
            feedbackBuffer          { IN_renderSystem.CreateUAVBufferResource(MEGABYTE) },
            feedbackDepth           { IN_renderSystem.CreateGPUResource(GPUResourceDesc::DepthTarget({ 256, 256 }, DeviceFormat::D32_FLOAT)) },
            feedbackCounters        { IN_renderSystem.CreateUAVBufferResource(512) },
            feedbackReturnBuffer    { IN_renderSystem.CreateReadBackBuffer(1 * MEGABYTE) },
            feedbackTarget          { IN_renderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(IN_WH, DeviceFormat::R32G32_UINT)) },
            heap                    { IN_renderSystem.CreateHeap(GIGABYTE * 1, 0) },
            mappedAssets            { IN_allocator }
        {
            renderSystem.RegisterPSOLoader(
                TEXTUREFEEDBACKPASS,
                { &renderSystem.Library.RSDefault, CreateTextureFeedbackPassPSO });

            renderSystem.RegisterPSOLoader(
                TEXTUREFEEDBACKCLEAR,
                { &renderSystem.Library.RSDefault, CreateTextureFeedbackClearPSO });

            renderSystem.RegisterPSOLoader(
                TEXTUREFEEDBACKCOMPRESSOR,
                { &renderSystem.Library.RSDefault, CreateTextureFeedbackCompressorPSO });

            renderSystem.SetReadBackEvent(
                feedbackReturnBuffer,
                [&, textureStreamingEngine = this](ReadBackResourceHandle resource)
                {
                    if (!updateInProgress)
                        return;

                    taskInProgress = true;
                    auto& task = allocator->allocate<TextureStreamUpdate>(resource, *this, allocator);
                    renderSystem.threads.AddBackgroundWork(task);
                });

            renderSystem.QueuePSOLoad(TEXTUREFEEDBACKPASS);
            renderSystem.QueuePSOLoad(TEXTUREFEEDBACKCLEAR);
            renderSystem.QueuePSOLoad(TEXTUREFEEDBACKCOMPRESSOR);

            renderSystem.SetDebugName(feedbackTarget, "feedbackTarget");
        }


        ~TextureStreamingEngine()
        {
            updateInProgress = false;

            renderSystem.SetReadBackEvent(
                feedbackReturnBuffer,
                [](ReadBackResourceHandle resource){});

            while (taskInProgress);

            renderSystem.WaitforGPU(); // Flush any pending reads
            renderSystem.FlushPendingReadBacks();

            renderSystem.ReleaseUAV(feedbackOffsets);
            renderSystem.ReleaseUAV(feedbackOutputTemp);
            renderSystem.ReleaseUAV(feedbackOutputFinal);

            renderSystem.ReleaseUAV(feedbackTemp1);
            renderSystem.ReleaseUAV(feedbackTemp2);

            renderSystem.ReleaseUAV(feedbackPPLists);
            renderSystem.ReleaseUAV(feedbackCounters);

            renderSystem.ReleaseTexture(feedbackDepth);
            renderSystem.ReleaseTexture(feedbackTarget);
        }


        void TextureFeedbackPass(
            UpdateDispatcher&                   dispatcher,
            FrameGraph&                         frameGraph,
            CameraHandle                        camera,
            UpdateTaskTyped<GetPVSTaskData>&    sceneGather,
            ResourceHandle                      testTexture,
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

        void PostUpdatedTiles(const BlockAllocation& blocks);
        void MarkUpdateCompleted() { updateInProgress = false; taskInProgress = false; }


        auto& ClearFeedbackBuffer(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB)
        {
            return ClearIntegerRenderTarget_RG32(frameGraph, feedbackTarget, reserveCB, { (uint32_t)-1, (uint32_t)-1 });
        }


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

        UAVResourceHandle		feedbackOffsets;        // GPU
        UAVTextureHandle        feedbackPPLists;        // GPU
        UAVResourceHandle		feedbackBuffer;         // GPU
        UAVResourceHandle       feedbackCounters;       // GPU

        ResourceHandle		    feedbackDepth;  // GPU
        ResourceHandle		    feedbackTarget; // GPU

        UAVResourceHandle		feedbackOutputTemp;   // GPU
        UAVResourceHandle		feedbackOutputFinal;  // GPU
        UAVTextureHandle        feedbackTemp1;        // GPU
        UAVTextureHandle        feedbackTemp2;        // GPU

        ReadBackResourceHandle  feedbackReturnBuffer; // CPU + GPU

        Vector<MappedAsset>     mappedAssets;
		TextureBlockAllocator	textureBlockAllocator;
        DeviceHeapHandle        heap;

        iAllocator*                 allocator;
		const TextureCacheDesc		settings;
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
