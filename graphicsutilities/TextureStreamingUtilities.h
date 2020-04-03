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


    
    /************************************************************************************************/


    struct TextureFeedbackPass_Data
    {
        CameraHandle                    camera;
        GatherTask&                     pvs;
        ReserveConstantBufferFunction   constantBufferAllocator;
        FrameResourceHandle             feedbackCounters;
        FrameResourceHandle             feedbackBuffer;
        FrameResourceHandle             feedbackLists;
        FrameResourceHandle             feedbackDepth;

        ReadBackResourceHandle          readbackBuffer;
    };



	/************************************************************************************************/


	constexpr size_t GetMinBlockSize()
	{
		return 64 * KILOBYTE;
	}

	struct TextureCacheDesc
	{
		const size_t textureCacheSize	= MEGABYTE * 16; // Very small for debugging purposes, forces resource eviction and priority, should be changed to a reasonable value
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
		TextureStreamingEngine(RenderSystem& IN_renderSystem, iAllocator* IN_allocator, const TextureCacheDesc& desc = {}) : 
			allocator		        { IN_allocator		},
            textureBlockAllocator   { desc.textureCacheSize / desc.blockSize,   IN_allocator },
			renderSystem	        { IN_renderSystem	},
			settings		        { desc				},
            feedbackBuffer          { IN_renderSystem.CreateUAVBufferResource(MEGABYTE * 2)                                    },
            feedbackDepth           { IN_renderSystem.CreateGPUResource(GPUResourceDesc::DepthTarget({ 240, 240 }, DeviceFormat::D32_FLOAT)) },
            feedbackCounters        { IN_renderSystem.CreateUAVBufferResource(512) },
            feedbackReturnBuffer    { IN_renderSystem.CreateReadBackBuffer(16 * MEGABYTE) },
            heap                    { IN_renderSystem.CreateHeap(GIGABYTE * 1, 0) },
            mappedAssets            { IN_allocator },
            updateThread            { *this, IN_allocator }
        {
            for (size_t I = 0; I < 3; I++)
            {
                renderSystem.SetReadBackEvent(
                    feedbackReturnBuffer,
                    [&, textureStreamingEngine = this](char* buffer, const size_t bufferSize)
                    {
                        if (!buffer)
                            return;

                        updateThread.PushUpdate(buffer, bufferSize, allocator);
                    });
            }


            textureLoadThread = std::thread{ [&] { updateThread.Run(); } };
        }


        ~TextureStreamingEngine()
        {
            updateThread.Shutdown();

            while (updateInProgress);

            textureLoadThread.join();

            renderSystem.ReleaseUAV(feedbackBuffer);
            renderSystem.ReleaseUAV(feedbackCounters);
            renderSystem.ReleaseTexture(feedbackDepth);
        }
        
        void TextureFeedbackPass(
            UpdateDispatcher&                   dispatcher,
            FrameGraph&                         frameGraph,
            CameraHandle                        camera,
            UpdateTaskTyped<GetPVSTaskData>&    sceneGather,
            ResourceHandle                      testTexture,
            ReserveConstantBufferFunction&      constantBufferAllocator);


        struct TextureBlockUpdateThread
        {
            TextureBlockUpdateThread(TextureStreamingEngine& IN_textureStreamEngine, iAllocator* IN_allocator);

            void Run();

            void PushUpdate(char* buffer, const size_t buffer_size, iAllocator* IN_allocator);
            void Shutdown()
            {
                running = false;
                cv.notify_all();
            }

            TextureStreamingEngine& textureStreamEngine;

            struct _PendingUpdate
            {
                uint64_t    requestCount    = 0;
                gpuTileID*  requests        = nullptr;
                iAllocator* allocator       = nullptr;
            };

            Vector<_PendingUpdate>      pendingUpdates;
            std::mutex                  lock;
            std::atomic_bool            running = true;
            std::condition_variable     cv;
            iAllocator*                 allocator;
        } updateThread;


        gpuTileList     UpdateTileStates    (const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator);
        BlockAllocation AllocateTiles       (const gpuTileID* begin, const gpuTileID* end);

        void                        BindAsset           (const AssetHandle textureAsset, const ResourceHandle  resource);
        std::optional<AssetHandle>  GetResourceAsset    (const ResourceHandle  resource) const;

        void PostUpdatedTiles(const BlockAllocation& blocks);
        void MarkUpdateCompleted() { updateInProgress = false; }

	private:


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

        std::thread                 textureLoadThread;

		RenderSystem&				renderSystem;

        bool                        updateInProgress = false;

        UAVResourceHandle		    feedbackBuffer;     // GPU
        UAVResourceHandle           feedbackCounters;   // GPU
        ResourceHandle		        feedbackDepth;      // GPU

        ReadBackResourceHandle      feedbackReturnBuffer; // CPU + GPU

        Vector<MappedAsset>         mappedAssets;
		TextureBlockAllocator		textureBlockAllocator;
        DeviceHeapHandle            heap;

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
