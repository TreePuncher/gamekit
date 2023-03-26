#ifndef TEXTURESTREAMINGUTILITIES_H
#define TEXTURESTREAMINGUTILITIES_H

#include "Memoryutilities.h"
#include "TextureUtilities.h"
#include "ThreadUtilities.h"
#include "Scene.h"
#include "AnimationComponents.h"

namespace FlexKit
{   /************************************************************************************************/


	class RenderSystem;
	struct BrushConstants;


	/************************************************************************************************/


	class iDecompressor
	{
	public:
		virtual ~iDecompressor() {}

		virtual UploadReservation   ReadTile(ReadContext&, const TileID_t id, const uint2 TileSize, CopyContext& ctx) = 0;
		virtual UploadReservation   Read(ReadContext&, const uint2 WH, CopyContext& ctx) = 0;
		virtual uint2               GetTextureWH() const = 0;
		virtual DeviceFormat        GetFormat() const = 0;

	};


	/************************************************************************************************/


	class DDSDecompressor : public iDecompressor
	{
	public:

		DDSDecompressor(ReadContext& readCtx, const uint32_t mipLevel, AssetHandle asset, iAllocator* IN_allocator);
		~DDSDecompressor();

		UploadReservation   ReadTile(ReadContext&, const TileID_t id, const uint2 TileSize, CopyContext& ctx) override;
		UploadReservation   Read(ReadContext&, const uint2 WH, CopyContext& ctx) override;
		uint2               GetTextureWH() const override;
		DeviceFormat        GetFormat() const override { return format; };

		size_t mipLevelOffset;

		size_t      rowPitch;
		size_t      bufferSize;
		size_t      mipLevel;

		uint2       WH;

		DeviceFormat format;

		char* buffer = nullptr;

		AssetHandle     asset;
		iAllocator*     allocator;
	};


	std::optional<DDSDecompressor*> CreateDDSDecompressor(ReadContext& readCtx, const uint32_t MIPlevel, AssetHandle asset, iAllocator* allocator);


	/************************************************************************************************/


	inline size_t BlockSize(DeviceFormat format)
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
		case DeviceFormat::BC7_UNORM:
		case DeviceFormat::BC7_SNORM:
			return 16;
		default:
			FK_ASSERT(false, "INVALID INPUT!");
			return -1;
			break;
		}
	}

	struct DDSLevelInfo
	{
		size_t  RowPitch = 0;
		uint2   WH;
		bool    tiled;
	};

	struct DDSInfo
	{
		uint8_t     MIPCount    = 0;
		uint2       WH          = { 0, 0 };

		DeviceFormat format;
	};

	DDSInfo GetDDSInfo(AssetHandle asset, ReadContext& ctx);

	inline DDSLevelInfo GetMIPLevelInfo(const size_t Level, const uint2 WH, const DeviceFormat format)
	{
		const uint32_t levelHeight  = Max(WH[0] >> Level, 4u);
		const uint32_t levelWidth   = Max(WH[1] >> Level, 4u);
		return { 0xff, { levelWidth, levelHeight }, levelWidth > GetFormatTileSize(format)[0] };
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

			if (!isAssetAvailable(asset))
				return false;

			TextureResourceBlob textureHeader;
			ReadAsset(readContext, asset, &textureHeader, sizeof(textureHeader));

			if (IsDDS((DeviceFormat)textureHeader.format))
			{
				Close();

				auto res = CreateDDSDecompressor(readContext, MIPlevel, asset, allocator);

				FK_ASSERT(res, "Failed to create texture asset decompressor!");

				decompressor    = res.value_or(nullptr);
				format          = decompressor->GetFormat(); // TODO: correctly extract the format
				currentLevel    = MIPlevel;
				currentAsset    = asset;

				return res.has_value();
			}
			else
				__debugbreak();

			return false;
		}

		void Close()
		{
			if (decompressor)
				allocator->release(*decompressor);
		}


		UploadReservation ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx)
		{
			return decompressor->ReadTile(readContext, id, TileSize, ctx);
		}


		UploadReservation Read(const uint2 WH, CopyContext& ctx)
		{
			return decompressor->Read(readContext, WH, ctx);
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

		ReadContext     readContext;
		DeviceFormat    format;
		iDecompressor*  decompressor = nullptr;
		AssetHandle     currentAsset = -1;
		uint8_t         currentLevel = -1;
		iAllocator*     allocator;
	};


	inline const PSOHandle TEXTUREFEEDBACKPASS                  = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKPASS));
	inline const PSOHandle TEXTUREFEEDBACKANIMATEDPASS          = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKANIMATEDPASS));

	inline const PSOHandle TEXTUREFEEDBACKCOMPRESSOR            = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKCOMPRESSOR));
	inline const PSOHandle TEXTUREFEEDBACKMERGEBLOCKS           = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKMERGEBLOCKS));
	inline const PSOHandle TEXTUREFEEDBACKPREFIXSUMBLOCKSIZES   = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKPREFIXSUMBLOCKSIZES));
	inline const PSOHandle TEXTUREFEEDBACKSETBLOCKSIZES         = PSOHandle(GetTypeGUID(TEXTUREFEEDBACKSETBLOCKSIZES));


	ID3D12PipelineState* CreateTextureFeedbackPassPSO(RenderSystem* RS);
	ID3D12PipelineState* CreateTextureFeedbackAnimatedPassPSO(RenderSystem* RS);

	ID3D12PipelineState* CreateTextureFeedbackCompressorPSO(RenderSystem* RS);
	ID3D12PipelineState* CreateTextureFeedbackBlockSizePreFixSum(RenderSystem* RS);
	ID3D12PipelineState* CreateTextureFeedbackMergeBlocks(RenderSystem* RS);
	ID3D12PipelineState* CreateTextureFeedbackSetBlockSizes(RenderSystem* RS);

	
	/************************************************************************************************/


	struct TextureFeedbackPass_Data
	{
		CameraHandle                    camera;
		const GatherPassesTask&         pvs;
		const GatherSkinnedTask&        skinnedModels;
		ReserveConstantBufferFunction   reserveCB;

		FrameResourceHandle             feedbackBuffers[2];

		FrameResourceHandle             feedbackDepth;
		FrameResourceHandle             feedbackCounters;
		FrameResourceHandle             feedbackBlockSizes;
		FrameResourceHandle             feedbackBlockOffsets;

		ReadBackResourceHandle          readbackBuffer;
	};


	/************************************************************************************************/


	constexpr size_t GetTileByteSize()
	{
		return 64 * KILOBYTE;
	}

	struct TextureCacheDesc
	{
		const size_t textureCacheSize	= 64 * MEGABYTE;
		const size_t blockSize			= GetTileByteSize();
	};


	/************************************************************************************************/


	struct AllocatedBlock
	{
		TileID_t		tileID;
		ResourceHandle	resource;
		uint32_t		offset;
		uint32_t		tileIdx;
	};

	struct gpuTileID
	{
		uint32_t TextureID;
		TileID_t tileID;

		uint64_t GetSortingID() const
		{
			return (uint64_t(TextureID) << 32) | CreateTileID(tileID.GetTileX(), tileID.GetTileY(), tileID.GetMipLevel());
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

		operator bool() const { return reallocations.size() + allocations.size() + packedAllocations.size(); }
	};

	class TextureBlockAllocator
	{
	public:

		struct Block
		{
			TileID_t        tileID;
			ResourceHandle  resource            = InvalidHandle;
			uint8_t         state               = EBlockState::Free;
			uint64_t        staleFrameCount     = 0;
			uint32_t        blockID;

			operator uint64_t() const
			{
				return ((uint64_t)resource.to_uint()) << 32 | tileID;
			}

			operator gpuTileID() const
			{
				return
					gpuTileID{
						.TextureID  = resource.to_uint(),
						.tileID     = tileID };
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
		BlockAllocation     AllocateBlock       (TileID_t, iAllocator* allocator);

		const static uint32_t   blockSize   = 64 * KILOBYTE;
		uint32_t                last        = 0;

		Vector<Block>           free;
		Vector<Block>           stale;
		Vector<Block>           inuse;

		iAllocator*             allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI TextureStreamingEngine
	{
	public:
		TextureStreamingEngine(RenderSystem&, iAllocator* IN_allocator = SystemAllocator, const TextureCacheDesc& desc    = {});


		~TextureStreamingEngine();


		void TextureFeedbackPass(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			CameraHandle					camera,
			uint2							renderTargetWH,
			BrushConstants&				constants,
			GatherPassesTask&				passes,
			GatherSkinnedTask&				skinnedModelsGather,
			ReserveConstantBufferFunction&	reserveCB,
			ReserveVertexBufferFunction&	reserveVB);


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

			iAllocator* allocator	= nullptr;
		};


		void LoadLowestLevel(ResourceHandle textureResource, CopyContextHandle copyQueue);

		gpuTileList		UpdateTileStates	(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator);
		BlockAllocation	AllocateTiles		(const gpuTileID* begin, const gpuTileID* end);

		void						BindAsset			(const AssetHandle textureAsset, const ResourceHandle  resource);
		std::optional<AssetHandle>	GetResourceAsset	(const ResourceHandle  resource) const;

		void PostUpdatedTiles(const BlockAllocation& blocks, iAllocator& threadLocalAllocator);
		void MarkUpdateCompleted() { updateInProgress = false; taskInProgress = false; }

		float debug_GetPassTime()	const { return passTime; }
		float debug_GetUpdateTime()	const { return updateTime; }

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
		std::atomic_bool        taskStarted			= false;

		ReadBackResourceHandle  feedbackReturnBuffer; // CPU + GPU

		Vector<MappedAsset>     mappedAssets;
		TextureBlockAllocator	textureBlockAllocator;
		DeviceHeapHandle        heap;

		QueryHandle             timeStats;

		iAllocator*             allocator;
		const TextureCacheDesc	settings;

		float passTime      = 0;
		float updateTime    = 0;
	};


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2022 Robert May

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
