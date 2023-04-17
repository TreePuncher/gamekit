#include "TextureStreamingUtilities.h"
#include "ProfilingUtilities.h"
#include "Graphics.h"
#include <range/v3/all.hpp>

namespace FlexKit
{   /************************************************************************************************/


	DDSInfo GetDDSInfo(AssetHandle assetID, ReadContext& ctx)
	{
		TextureResourceBlob resource;

		if (ReadAsset(ctx, assetID, &resource, sizeof(resource), 0) != RAC_OK)
			return {};

		return { (uint8_t)resource.mipLevels, resource.WH, resource.format };
	}


	/************************************************************************************************/


	DDSDecompressor::DDSDecompressor(ReadContext& readCtx, const uint32_t IN_mipLevel, AssetHandle IN_asset, iAllocator* IN_allocator) :
		allocator	{ IN_allocator },
		asset		{ IN_asset },
		mipLevel	{ IN_mipLevel }
	{
		TextureResourceBlob resource;

		FK_ASSERT(ReadAsset(readCtx, asset, &resource, sizeof(resource), 0) == RAC_OK);

		format			= resource.format;
		WH				= resource.WH;
		mipLevelOffset	= resource.mipOffsets[mipLevel] + sizeof(resource);

		auto mipSize = resource.mipOffsets[mipLevel + 1] - resource.mipOffsets[mipLevel];

		if (mipSize < 64 * 1024) {
			buffer = (char*)IN_allocator->malloc(mipSize);
			memset(buffer, 0xCD, mipSize);
			readCtx.Read(buffer, mipSize, mipLevelOffset);
		}
	}


	DDSDecompressor::~DDSDecompressor()
	{
		if(buffer)
			allocator->free(buffer);
	}


	/************************************************************************************************/


	UploadReservation DDSDecompressor::ReadTile(ReadContext& readCtx, const TileID_t id, const uint2 TileSize, CopyContext& ctx)
	{
		const auto reservation		= ctx.Reserve(64 * KILOBYTE);
		const auto blockSize		= BlockSize(format);
		const auto blocksX			= 256 / 4;
		const auto localRowPitch	= blocksX * blockSize;
		const auto blocksY			= GetTileByteSize() / localRowPitch;

		const auto textureRowWidth = (WH[0] >> mipLevel) / 4 * blockSize;


		const auto tileX = id.GetTileX();
		const auto tileY = id.GetTileY();

		for (size_t row = 0; row < blocksY; ++row)
			readCtx.Read(
				reservation.buffer + row * localRowPitch,
				localRowPitch,
				mipLevelOffset + row * textureRowWidth + textureRowWidth * tileY * blocksY + localRowPitch * tileX);

		return reservation;
	}


	/************************************************************************************************/


	UploadReservation DDSDecompressor::Read(ReadContext& readCtx, const uint2 WH, CopyContext& ctx)
	{
		const auto blockSize				= BlockSize(format);
		const size_t destinationRowPitch	= Max(blockSize * WH[0]/4, 256);
		const size_t rowCount				= Max(WH[1] / 4, 1);
		const size_t columnCount			= Max(WH[0] / 4, 1);
		const size_t blockCount				= rowCount * columnCount;
		const size_t allocationSize			= rowCount * destinationRowPitch;
		const auto   reservation			= ctx.Reserve(allocationSize, 512);
		const size_t sourceRowPitch			= blockSize * columnCount;

		if (buffer)
		{
			for (size_t row = 0; row < rowCount; row++) {
				const auto sourceOffset = (sourceRowPitch * row);
				memcpy(
					reservation.buffer + row * destinationRowPitch,
					buffer + sourceOffset,
					sourceRowPitch);
			}
		}
		else
			ReadAsset(
				readCtx,
				asset,
				reservation.buffer,
				blockCount * blockSize,
				mipLevelOffset);

		return reservation;
	}


	/************************************************************************************************/


	uint2 DDSDecompressor::GetTextureWH() const
	{
		return WH;
	}


	/************************************************************************************************/


	DDSDecompressor* CreateDDSDecompressor(ReadContext& readCtx, const uint32_t MIPlevel, AssetHandle asset, iAllocator* allocator)
	{
		return &allocator->allocate<DDSDecompressor>(readCtx, MIPlevel, asset, allocator);
	}


	/************************************************************************************************/


	TextureBlockAllocator::TextureBlockAllocator(const size_t blockCount, iAllocator* IN_allocator) :
		free			{ IN_allocator },
		stale			{ IN_allocator },
		inuse			{ IN_allocator }
	{
		for (size_t I = 0; I < blockCount; ++I)
			free.emplace_back(
				Block
				{
					.tileID					= uint32_t(-1),
					.resource				= InvalidHandle,
					.state					= EBlockState::Free,
					.staleFrameCount		= 0,
					.blockID				= uint32_t(I),
				});
	}


	/************************************************************************************************/


	ID3D12PipelineState* TextureStreamingEngine::CreateTextureFeedbackPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("Forward_VS",				"vs_6_0", R"(assets\shaders\TextureFeedback\TextureFeedback_VS.hlsl)");
		auto PShader = RS->LoadShader("TextureFeedback_PS",		"ps_6_5", R"(assets\shaders\TextureFeedback\TextureFeedback.hlsl)", ShaderOptions{ .enable16BitTypes = true, .hlsl2021 = true });

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // enable conservative rast

		if(RS->features.conservativeRast == AvailableFeatures::ConservativeRast_AVAILABLE)
			Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthEnable      = true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = { 0 };
		PSO_Desc.pRootSignature			= feedbackPassRootSignature;
		PSO_Desc.VS						= VShader;
		PSO_Desc.PS						= PShader;
		PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask				= UINT_MAX;
		PSO_Desc.RasterizerState		= Rast_Desc;
		PSO_Desc.DepthStencilState		= Depth_Desc;
		PSO_Desc.InputLayout			= { InputElements, 4 };
		PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets		= 0;
		PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
		PSO_Desc.SampleDesc				= { 1, 0 };

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create Texture feedback PS");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* TextureStreamingEngine::CreateTextureFeedbackAnimatedPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ForwardSkinned_VS", "vs_6_0", R"(assets\shaders\TextureFeedback\TextureFeedback_VS.hlsl)");
		auto PShader = RS->LoadShader("TextureFeedback_PS", "ps_6_2", R"(assets\shaders\TextureFeedback\TextureFeedback.hlsl)", ShaderOptions{ .enable16BitTypes = true, .hlsl2021 = true });

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,	5, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "BLENDPOS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	6, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDNORM",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	7, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDTAN",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	8, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // enable conservative rast

		if(RS->features.conservativeRast == AvailableFeatures::ConservativeRast_AVAILABLE)
			Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthEnable      = true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = { 0 };
		PSO_Desc.pRootSignature			= feedbackPassRootSignature;
		PSO_Desc.VS						= VShader;
		PSO_Desc.PS						= PShader;
		PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		PSO_Desc.SampleMask				= UINT_MAX;
		PSO_Desc.RasterizerState		= Rast_Desc;
		PSO_Desc.DepthStencilState		= Depth_Desc;
		PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
		PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PSO_Desc.NumRenderTargets		= 0;
		PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
		PSO_Desc.SampleDesc				= { 1, 0 };

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackCompressorPSO(RenderSystem* RS)
	{
		const char* file = RS->vendorID == DeviceVendor::AMD ?
				"assets\\shaders\\TextureFeedback\\TextureFeedbackCompressor_AMD.hlsl" :
				"assets\\shaders\\TextureFeedback\\TextureFeedbackCompressor.hlsl";

		auto computeShader = RS->LoadShader(
			"CompressBlocks", "cs_6_7", file);

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create Texture feedback compressor shader");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackBlockSizePreFixSum(RenderSystem* RS)
	{
		auto computeShader = RS->LoadShader(
			"PreFixSumBlockSizes", "cs_6_5",
			"assets\\shaders\\TextureFeedback\\TextureFeedbackBlockPreFixSum.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create Texture feedback prefix sum shader");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackMergeBlocks(RenderSystem* RS)
	{
		auto computeShader = RS->LoadShader(
			"MergeBlocks", "cs_6_5",
			"assets\\shaders\\TextureFeedback\\TextureFeedbackMergeBlocks.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create Merge Block shader");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackSetBlockSizes(RenderSystem* RS)
	{
		auto computeShader = RS->LoadShader(
			"SetBlockCounters", "cs_6_5",
			"assets\\shaders\\TextureFeedback\\TextureFeedbackMergeBlocks.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}

	/************************************************************************************************/


	FeedbackTable::FeedbackTable(iAllocator& IN_allocator) :
		allocator	{ IN_allocator },
		mappings	{ IN_allocator }
	{
		mappings.resize(8);

		for (auto&& [handle, offset, size] : mappings)
		{
			handle = InvalidHandle;
			offset = -1;
			size = -1;
		}
	}


	/************************************************************************************************/


	uint32_t FeedbackTable::_GetTextureOffset(ResourceHandle handle)
	{
		auto idx = hashFN(handle ^ mappings.size()) % mappings.size();

		for (size_t i = 0; i < 4; i++)
		{
			auto&& [key, offset, size] = mappings[(idx + i) % mappings.size()];

			if (key == handle)
				return offset;
		}

		return -1;
	}


	/************************************************************************************************/


	uint32_t FeedbackTable::GetTextureOffset(ResourceHandle handle)
	{
		if (!textureCount)
			return -1;

		std::shared_lock lock{ m };

		return _GetTextureOffset(handle);
	}


	/************************************************************************************************/


	void FeedbackTable::Expand()
	{
		Vector<std::tuple<ResourceHandle, uint32_t, uint32_t>>	old = std::move(mappings);

		mappings.resize(old.size() * 2);
		for (auto&& [handle, offset, size] : mappings)
		{
			handle	= InvalidHandle;
			offset	= -1;
			size	= -1;
		}


		for (auto&& [handle, offset, size] : old)
			_InsertItem(handle, offset, size);
	}


	/************************************************************************************************/


	void FeedbackTable::_InsertItem(ResourceHandle handle, uint32_t IN_offset, uint32_t IN_size)
	{
		if (handle != InvalidHandle)
		{
			auto idx = hashFN(handle ^ mappings.size()) % mappings.size();

			const size_t end = mappings.size();
			for (size_t i = 0; i < end; i++)
			{
				auto&& [key, offset, size] = mappings[(idx + i) % mappings.size()];

				if (key == InvalidHandle)
				{
					key		= handle;
					offset	= IN_offset;
					size	= IN_size;
					textureCount++;
					return;
				}
			}

			DebugBreak();
		}
	}


	/************************************************************************************************/


	void FeedbackTable::InsertItem(ResourceHandle handle, uint32_t offset, uint32_t size)
	{
		std::unique_lock lock{ m };

		if (_GetTextureOffset(handle) != -1)
			return;

		if (textureCount + 1 > mappings.size() * .75f)
			Expand();

		_InsertItem(handle, offset, size);
	}


	/************************************************************************************************/


	void FeedbackTable::Clear()
	{
		memset(mappings.data(), 0xff, mappings.ByteSize());
		textureCount = 0;
	}


	/************************************************************************************************/


	static const uint64_t FeedbackBufferSize = sizeof(uint2) * 1024 * 256 + 1024;


	TextureStreamingEngine::TextureStreamingEngine(
				RenderSystem&	IN_renderSystem,
				iAllocator*		IN_allocator,
				const TextureCacheDesc& desc) : 
			allocator					{ IN_allocator		},
			textureBlockAllocator		{ desc.textureCacheSize / desc.blockSize,   IN_allocator },
			renderSystem				{ IN_renderSystem	},
			settings					{ desc				},
			feedbackReturnBuffer		{ IN_renderSystem.CreateReadBackBuffer(2 * MEGABYTE) },
			heap						{ IN_renderSystem.CreateHeap(desc.textureCacheSize, 0) },
			mappedAssets				{ IN_allocator },
			timeStats					{ IN_renderSystem.CreateTimeStampQuery(512) },
			feedbackPassRootSignature	{ IN_allocator },
			sortingRootSignature		{ IN_allocator },
			pendingResults				{ { *IN_allocator }, 0 }
	{
		feedbackPassRootSignature.AllowIA = true;

		DesciptorHeapLayout<1> srvHeap;
		srvHeap.SetParameterAsSRV(0, 0, -1);
		FK_ASSERT(srvHeap.Check());

		feedbackPassRootSignature.SetParameterAsCBV(0, 0);
		feedbackPassRootSignature.SetParameterAsCBV(1, 1, 0);
		feedbackPassRootSignature.SetParameterAsUINT(2, 17, 2, 0, PIPELINE_DEST_PS);
		feedbackPassRootSignature.SetParameterAsUAV(3, 0, 0, PIPELINE_DEST_PS);
		feedbackPassRootSignature.SetParameterAsSRV(4, 0, 0, PIPELINE_DEST_VS);
		feedbackPassRootSignature.SetParameterAsDescriptorTable(5, srvHeap, -1, PIPELINE_DEST_PS);

		feedbackPassRootSignature.Build(IN_renderSystem, IN_allocator);
		SETDEBUGNAME(feedbackPassRootSignature, "textureFeedbackPassSignature");

		sortingRootSignature.AllowIA = true;
		sortingRootSignature.SetParameterAsUINT(0, 16, 0, 0);
		sortingRootSignature.SetParameterAsSRV(1, 0);
		sortingRootSignature.SetParameterAsSRV(2, 1);
		sortingRootSignature.SetParameterAsUAV(3, 0, 0);

		auto res2 = sortingRootSignature.Build(IN_renderSystem, IN_allocator);
		FK_ASSERT(res2, "Failed to create root signature!");

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKPASS,
			{ &feedbackPassRootSignature, [&](auto rs) { return CreateTextureFeedbackPassPSO(rs); } });

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKANIMATEDPASS,
			{ &feedbackPassRootSignature, [&](auto rs) { return CreateTextureFeedbackAnimatedPassPSO(rs); } });

		renderSystem.SetReadBackEvent(
			feedbackReturnBuffer,
			[&, textureStreamingEngine = this](ReadBackResourceHandle resource)
			{
				if (!updateInProgress)
					return;

				taskInProgress	= true;
				taskStarted		= false;
				auto& task		= allocator->allocate<TextureStreamUpdate>(resource, *this, allocator);
				renderSystem.threads.AddBackgroundWork(task);
			});

		renderSystem.QueuePSOLoad(TEXTUREFEEDBACKPASS);
		renderSystem.QueuePSOLoad(TEXTUREFEEDBACKANIMATEDPASS);
	}


	/************************************************************************************************/


	TextureStreamingEngine::~TextureStreamingEngine()
	{
		updateInProgress = false;

		renderSystem.SetReadBackEvent(
			feedbackReturnBuffer,
			[](ReadBackResourceHandle resource){});

		if(taskStarted)
			while (taskInProgress);

		renderSystem.WaitforGPU(); // Flush any pending reads
		renderSystem.FlushPendingReadBacks();

		renderSystem.ReleaseReadBack(feedbackReturnBuffer);
	}


	/************************************************************************************************/

	constexpr uint32_t p			= 8;
	constexpr uint32_t blockCount	= 2048;
	constexpr uint32_t bufferSize	= 1024 * blockCount;


	struct FeedbackPassConstants
	{
		float		bias;
		uint32_t	offsets[16];
	};

	
	struct TextureFeedbackPass_Data
	{
		CameraHandle                    camera;
		const GatherPassesTask&         pvs;
		ReserveConstantBufferFunction   reserveCB;

		FrameResourceHandle             feedbackBuffers[2];

		FrameResourceHandle             feedbackDepth;
		FrameResourceHandle             feedbackCounters;
		FrameResourceHandle             feedbackBlockSizes;
		FrameResourceHandle             feedbackBlockOffsets;

		ReadBackResourceHandle          readbackBuffer;
	};

	size_t GetTextureBlockSize(ResourceHandle handle, RenderSystem& renderSystem)
	{
		auto mip0TileCount	= renderSystem.GetTextureTilingWH(handle, 0);

		return size_t(mip0TileCount.Product());
	}

	void TextureStreamingEngine::TextureFeedbackPass(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			CameraHandle					camera,
			uint2							renderTargetWH,
			BrushConstants&					brushConstants,
			GatherPassesTask&				passes,
			const ResourceAllocation&		animationResources,
			ReserveConstantBufferFunction&	reserveCB,
			ReserveVertexBufferFunction&	reserveVB,
			iAllocator&						tempAllocator)
	{
		if (updateInProgress)
			return;

		updateInProgress = true;

		auto initiateFeedbackPass = frameGraph.AddNode<TextureFeedbackPass_Data>(
			TextureFeedbackPass_Data{
				camera,
				passes,
				reserveCB,
			},
			[&](FrameGraphNodeBuilder& builder, TextureFeedbackPass_Data& data)
			{
				builder.AddDataDependency(passes);

				auto depthBufferDesc = GPUResourceDesc::DepthTarget({ 128, 128 }, DeviceFormat::D32_FLOAT);
				depthBufferDesc.denyShaderUsage = true;

				data.feedbackBuffers[0]			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4 * MEGABYTE), DASCommon, VirtualResourceScope::Frame);
				data.feedbackBuffers[1]			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4 * MEGABYTE), DASUAV, VirtualResourceScope::Frame);
				data.feedbackCounters			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 512), DASUAV, VirtualResourceScope::Frame);
				data.feedbackBlockSizes			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 512), DASUAV, VirtualResourceScope::Frame);
				data.feedbackBlockOffsets		= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 512), DASUAV, VirtualResourceScope::Frame);
				data.feedbackDepth				= builder.AcquireVirtualResource(depthBufferDesc, DASDEPTHBUFFERWRITE, VirtualResourceScope::Frame);
			},
			[=](TextureFeedbackPass_Data& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ctx.ClearDepthBuffer(resources.GetResource(data.feedbackDepth), 1.0f);
				ctx.ClearUAVBufferRange(resources.GetResource(data.feedbackCounters), 0, 4096);
				ctx.ClearUAVBufferRange(resources.UAV(data.feedbackBuffers[0], ctx), 0, MEGABYTE);
			});


		auto getStaticPass		= [&passTable = passes.GetData()]() -> std::span<const PVEntry> { return passTable.GetPass(GBufferPassID); };
		auto getAnimatedPass	= [&passTable = passes.GetData()]() -> std::span<const PVEntry> { return passTable.GetPass(GBufferAnimatedPassID); };


		PassDescription<TextureFeedbackPass_Data> staticPass =
		{
			.sharedData			= {
				.camera			= camera,
				.pvs			= passes,
				.reserveCB		= reserveCB,
			},
			.getPVS				= getStaticPass,
		};

		PassDescription<TextureFeedbackPass_Data> animatedPass =
		{
			.sharedData			= {
				.camera			= camera,
				.pvs			= passes,
				.reserveCB		= reserveCB,
			},
			.getPVS				= getAnimatedPass,
		};


		auto staticPassSetupFn = [&](FrameGraphNodeBuilder& builder, TextureFeedbackPass_Data& data)
		{
			auto depthBufferDesc			= GPUResourceDesc::DepthTarget({ 128, 128 }, DeviceFormat::D32_FLOAT);
			depthBufferDesc.denyShaderUsage = true;

			builder.ReadConstantBuffer(brushConstants.constants);

			data.feedbackBuffers[0]			= builder.WriteTransition(initiateFeedbackPass.feedbackBuffers[0], DASUAV);
			data.feedbackCounters			= builder.WriteTransition(initiateFeedbackPass.feedbackCounters, DASUAV);
			data.feedbackDepth				= builder.WriteTransition(initiateFeedbackPass.feedbackDepth, DASDEPTHBUFFERWRITE);
		};

		auto& feedbackPassRootSignature = this->feedbackPassRootSignature;
		auto& feedbackTable				= pendingResults;

		auto staticPassDrawFN = [&brushConstants, renderTargetWH, &feedbackPassRootSignature, &feedbackTable](const auto begin, const auto end, std::span<const PVEntry> pvs, TextureFeedbackPass_Data& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
		{
			ctx.BeginEvent_DEBUG("Texture feedback pass");

			const float bias		 = std::log2f(128.0f / renderTargetWH[0]);

			auto& materials			= MaterialComponent::GetComponent();
			auto& constantBuffer	= brushConstants.GetConstantBuffer();
			auto constants			= FlexKit::CreateCBIterator<Brush::VConstantsLayout>(constantBuffer);

			const size_t bufferSize = AlignedSize<Camera::ConstantBuffer>();

			CBPushBuffer passConstantBuffer		{ data.reserveCB(bufferSize) };

			auto cameraConstantValues	= GetCameraConstants(data.camera);

			// Widen FOV to avoid some pop in when rotating camera
			cameraConstantValues.FOV *= 1.2f;
			cameraConstantValues = CalculateCameraConstants(
				cameraConstantValues.AspectRatio,
				cameraConstantValues.FOV,
				cameraConstantValues.MinZ,
				cameraConstantValues.MaxZ,
				cameraConstantValues.ViewI.Transpose(),
				cameraConstantValues.View.Transpose());

			const auto cameraConstants	= ConstantBufferDataSet{ cameraConstantValues, passConstantBuffer };

			ctx.SetRootSignature(feedbackPassRootSignature);

			ctx.SetScissorAndViewports({ resources.GetResource(data.feedbackDepth) });
			ctx.SetRenderTargets({}, true, resources.GetResource(data.feedbackDepth));

			ctx.SetGraphicsConstantBufferView(0, cameraConstants);
			ctx.SetGraphicsUnorderedAccessView(3, resources.GetResource(data.feedbackBuffers[0]));
			ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

			ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACKPASS));

			TriMeshHandle	prevMesh	= InvalidHandle;
			TriMesh*		triMesh		= nullptr;

			for (auto itr = begin; itr < end; itr++)
			{
				ctx.BeginEvent_DEBUG("Draw entity");

				auto submissionID				= itr->submissionID;
				auto constantsBegin				= brushConstants.entityTable[submissionID];
				const auto& material			= materials[itr->brush->material];

				for (size_t I = 0; I < itr->brush->meshes.size(); I++)
				{
					ctx.BeginEvent_DEBUG("Draw sub-mesh");

					auto triMeshHandle = itr->brush->meshes[I];
					if (triMeshHandle != prevMesh)
					{
						prevMesh = triMeshHandle;

						triMesh = GetMeshResource(triMeshHandle);

						ctx.AddIndexBuffer(triMesh, itr->LODlevel[I]);
						ctx.AddVertexBuffers(
							triMesh,
							itr->LODlevel[I],
							{
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							}
						);
					}

					const auto& lod = triMesh->lods[itr->LODlevel[I]];

					size_t subMeshCount = lod.subMeshes.size();
					if (subMeshCount == 1)
					{
						const auto& textureDescriptors = material.textureDescriptors;

						if (textureDescriptors.size != 0)
						{
							FeedbackPassConstants passConstants{ .bias = bias };

							for (auto [idx, texture] : zip(iota(0), material.Textures))
							{
								uint32_t offset = feedbackTable.table.GetTextureOffset(texture);

								if (offset == -1)
								{
									uint32_t blockSize	= GetTextureBlockSize(texture, resources.renderSystem);
									offset				= feedbackTable.Reserve(blockSize);
									feedbackTable.table.InsertItem(texture, offset, blockSize);
								}

								passConstants.offsets[idx] = offset;
							}

							ctx.SetGraphicsConstantBufferView(1, constants[constantsBegin + I]);
							ctx.SetGraphicsConstantValue(2, 17, &passConstants);
							ctx.SetGraphicsDescriptorTable(5, textureDescriptors);
							ctx.DrawIndexed(lod.GetIndexCount());
						}
					}
					else
					{
						for (size_t itr = 0; itr < subMeshCount; itr++)
						{
							const auto subMesh		= lod.subMeshes[itr];
							const auto& subMaterial	= materials[material.SubMaterials[itr]];

							if (subMaterial.textureDescriptors.size != 0)
							{
								FeedbackPassConstants passConstants{ .bias = bias };

								for (auto [idx, texture] : zip(iota(0), subMaterial.Textures))
								{
									uint32_t offset = feedbackTable.table.GetTextureOffset(texture);

									if (offset == -1)
									{
										uint32_t blockSize	= GetTextureBlockSize(texture, resources.renderSystem);
										offset				= feedbackTable.Reserve(blockSize);
										feedbackTable.table.InsertItem(texture, offset, blockSize);
									}

									passConstants.offsets[idx] = offset;
								}

								ctx.SetGraphicsConstantBufferView(1, constants[constantsBegin + itr]);
								ctx.SetGraphicsConstantValue(2, 1, &passConstants.bias, 0);
								ctx.SetGraphicsConstantValue(2, 4, &passConstants.offsets, 1);
								ctx.SetGraphicsDescriptorTable(5, subMaterial.textureDescriptors);
								ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
							}
						}
					}

					ctx.EndEvent_DEBUG();
				}

				ctx.EndEvent_DEBUG();
			}

			ctx.EndEvent_DEBUG();
		};

		auto animatedPassSetupFn = [&](FrameGraphNodeBuilder& builder, TextureFeedbackPass_Data& data)
		{
			auto depthBufferDesc			= GPUResourceDesc::DepthTarget({ 128, 128 }, DeviceFormat::D32_FLOAT);
			depthBufferDesc.denyShaderUsage = true;

			builder.ReadConstantBuffer(brushConstants.constants);
			builder.AddNodeDependency(animationResources.node);

			data.feedbackBuffers[0]			= builder.WriteTransition(initiateFeedbackPass.feedbackBuffers[0], DASUAV);
			data.feedbackCounters			= builder.WriteTransition(initiateFeedbackPass.feedbackCounters, DASUAV);
			data.feedbackDepth				= builder.WriteTransition(initiateFeedbackPass.feedbackDepth, DASDEPTHBUFFERWRITE);
		};

		auto animatedPassDrawFN = [&brushConstants, renderTargetWH, &feedbackPassRootSignature, &animationResources, &feedbackTable](const auto begin, const auto end, std::span<const PVEntry> pvs, TextureFeedbackPass_Data& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
		{
			ctx.BeginEvent_DEBUG("Texture feedback pass");

			const float bias = std::log2f(128.0f / renderTargetWH[0]);

			auto& materials			= MaterialComponent::GetComponent();
			auto& constantBuffer	= brushConstants.GetConstantBuffer();
			auto constants			= FlexKit::CreateCBIterator<Brush::VConstantsLayout>(constantBuffer);
			const size_t bufferSize = AlignedSize<Camera::ConstantBuffer>();

			CBPushBuffer passConstantBuffer		{ data.reserveCB(bufferSize) };

			auto cameraConstantValues	= GetCameraConstants(data.camera);

			// Widen FOV to avoid some pop in when rotating camera
			cameraConstantValues.FOV *= 1.2f;
			cameraConstantValues = CalculateCameraConstants(
				cameraConstantValues.AspectRatio,
				cameraConstantValues.FOV,
				cameraConstantValues.MinZ,
				cameraConstantValues.MaxZ,
				cameraConstantValues.ViewI.Transpose(),
				cameraConstantValues.View.Transpose());

			const auto cameraConstants	= ConstantBufferDataSet{ cameraConstantValues, passConstantBuffer };

			ctx.SetRootSignature(feedbackPassRootSignature);

			ctx.SetScissorAndViewports({ resources.GetResource(data.feedbackDepth) });
			ctx.SetRenderTargets({}, true, resources.GetResource(data.feedbackDepth));

			ctx.SetGraphicsConstantBufferView(0, cameraConstants);
			ctx.SetGraphicsUnorderedAccessView(3, resources.GetResource(data.feedbackBuffers[0]));
			ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

			ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACKANIMATEDPASS));

			TriMeshHandle	prevMesh	= InvalidHandle;
			TriMesh*		triMesh		= nullptr;

			for (auto itr = begin; itr < end; itr++)
			{
				ctx.BeginEvent_DEBUG("Draw entity");

				auto submissionID		= itr->submissionID;
				auto constantsBegin		= brushConstants.entityTable[submissionID];
				const auto& material	= materials[itr->brush->material];
				auto animationPose		= resources.GetResource(animationResources.handles[std::distance(pvs.begin(), itr)]);
				
				for (size_t I = 0; I < itr->brush->meshes.size(); I++)
				{
					ctx.BeginEvent_DEBUG("Draw sub-mesh");

					auto triMeshHandle = itr->brush->meshes[I];
					if (triMeshHandle != prevMesh)
					{
						prevMesh = triMeshHandle;

						triMesh = GetMeshResource(triMeshHandle);

						ctx.AddIndexBuffer(triMesh, itr->LODlevel[I]);
						ctx.AddVertexBuffers(
							triMesh,
							itr->LODlevel[I],
							{
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
							}
						);
					}

					const auto& lod = triMesh->lods[itr->LODlevel[I]];

					ctx.SetGraphicsShaderResourceView(4, animationPose);

					size_t subMeshCount = lod.subMeshes.size();
					if (subMeshCount == 1)
					{
						const auto& textureDescriptors = material.textureDescriptors;

						if (textureDescriptors.size != 0)
						{
							FeedbackPassConstants passConstants{ .bias = bias };

							for (auto [idx, texture] : zip(iota(0), material.Textures))
							{
								uint32_t offset = feedbackTable.table.GetTextureOffset(texture);

								if (offset == -1)
								{
									uint32_t blockSize	= GetTextureBlockSize(texture, resources.renderSystem);
									offset				= feedbackTable.Reserve(blockSize);
									feedbackTable.table.InsertItem(texture, offset, blockSize);
								}

								passConstants.offsets[idx] = offset;
							}

							ctx.SetGraphicsConstantBufferView(1, constants[constantsBegin + I]);
							ctx.SetGraphicsConstantValue(2, 17, &passConstants);
							ctx.SetGraphicsDescriptorTable(5, textureDescriptors);
							ctx.DrawIndexed(lod.GetIndexCount());
						}
					}
					else
					{
						for (size_t itr = 0; itr < subMeshCount; itr++)
						{
							const auto subMesh		= lod.subMeshes[itr];
							const auto& subMaterial	= materials[material.SubMaterials[itr]];

							if (subMaterial.textureDescriptors.size != 0)
							{
								FeedbackPassConstants passConstants{ .bias = bias };

								for (auto [idx, texture] : zip(iota(0), subMaterial.Textures))
								{
									uint32_t offset = feedbackTable.table.GetTextureOffset(texture);

									if (offset == -1)
									{
										uint32_t blockSize	= GetTextureBlockSize(texture, resources.renderSystem);
										offset				= feedbackTable.Reserve(blockSize);
										feedbackTable.table.InsertItem(texture, offset, blockSize);
									}

									passConstants.offsets[idx] = offset;
								}

								ctx.SetGraphicsConstantBufferView(1, constants[constantsBegin + itr]);
								ctx.SetGraphicsConstantValue(2, 17, &passConstants);
								ctx.SetGraphicsDescriptorTable(5, subMaterial.textureDescriptors);
								ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
							}
						}
					}

					ctx.EndEvent_DEBUG();
				}

				ctx.EndEvent_DEBUG();
			}

			ctx.EndEvent_DEBUG();
		};

		frameGraph.AddPass(staticPass, staticPassSetupFn, staticPassDrawFN);
		frameGraph.AddPass(animatedPass, animatedPassSetupFn, animatedPassDrawFN);

		auto readBackNode = frameGraph.AddNode<TextureFeedbackPass_Data>(
			TextureFeedbackPass_Data{
				.camera			= camera,
				.pvs			= passes,
				.reserveCB		= reserveCB,
			},
			[&](FrameGraphNodeBuilder& builder, TextureFeedbackPass_Data& data)
			{
				builder.AddDataDependency(passes);

				auto depthBufferDesc = GPUResourceDesc::DepthTarget({ 128, 128 }, DeviceFormat::D32_FLOAT);
				depthBufferDesc.denyShaderUsage = true;

				data.feedbackBuffers[0]			= builder.WriteTransition(initiateFeedbackPass.feedbackBuffers[0],		DASCopySrc);
				data.feedbackBuffers[1]			= builder.WriteTransition(initiateFeedbackPass.feedbackBuffers[1],		DASUAV);
				data.feedbackCounters			= builder.WriteTransition(initiateFeedbackPass.feedbackCounters,		DASUAV);
				data.feedbackBlockSizes			= builder.WriteTransition(initiateFeedbackPass.feedbackBlockSizes,		DASUAV);
				data.feedbackBlockOffsets		= builder.WriteTransition(initiateFeedbackPass.feedbackBlockOffsets,	DASUAV);

				builder.ReadBack(feedbackReturnBuffer);
				data.readbackBuffer = feedbackReturnBuffer;
			},
			[=, &feedbackTable](TextureFeedbackPass_Data& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ctx.BeginEvent_DEBUG("Copy Out Results");

				// Write out
				auto src = resources.CopySrc(data.feedbackBuffers[0], ctx);
				auto dst = resources.GetDeviceResource(data.readbackBuffer);

				ctx.CopyBufferRegion(dst, src, feedbackTable.offsets * 4);

				ctx.QueueReadBack(data.readbackBuffer);

				ctx.EndEvent_DEBUG();
			});
	}

	/************************************************************************************************/


	TextureStreamingEngine::TextureStreamUpdate::TextureStreamUpdate(
		ReadBackResourceHandle		IN_resource,
		TextureStreamingEngine&		IN_textureStreamEngine,
		iAllocator*					IN_allocator) :
			iWork				{ IN_allocator },
			textureStreamEngine	{ IN_textureStreamEngine },
			allocator			{ IN_allocator  },
			resource			{ IN_resource   } {}


	/************************************************************************************************/


	void TextureStreamingEngine::TextureStreamUpdate::Run(iAllocator& threadLocalAllocator)
	{
		textureStreamEngine.taskStarted = true;

		Vector<uint32_t> results{ threadLocalAllocator };
		results.resize(textureStreamEngine.pendingResults.offsets);

		auto [buffer, bufferSize] = textureStreamEngine.renderSystem.OpenReadBackBuffer(resource);
		EXITSCOPE(textureStreamEngine.renderSystem.CloseReadBackBuffer(resource));
		EXITSCOPE(textureStreamEngine.updateInProgress = false);
		EXITSCOPE(textureStreamEngine.pendingResults.Reset());

		std::chrono::high_resolution_clock Clock;
		auto Before = Clock.now();

		EXITSCOPE(
			auto After = Clock.now();
		auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(After - Before);

		textureStreamEngine.updateTime = float(Duration.count()) / 1000.0f; );

		if (!buffer)
			return;

		memcpy(results.data(), buffer, results.ByteSize());

		const auto lg_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool { return lhs.GetSortingID() < rhs.GetSortingID(); };
		const auto eq_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool { return lhs.GetSortingID() == rhs.GetSortingID(); };

		Vector<gpuTileID> requests{ threadLocalAllocator };
		requests.reserve(4096);

		Vector<std::pair<size_t, size_t>> debug{ threadLocalAllocator };

		for (const auto& [texture, offset, size] : textureStreamEngine.pendingResults.table.mappings)
		{
			if (texture != InvalidHandle)
			{
				const uint2 WH				= textureStreamEngine.renderSystem.GetTextureWH(texture);
				const uint2 XY				= textureStreamEngine.renderSystem.GetTextureTilingWH(texture, 0);
				const auto packedBlockInfo	= textureStreamEngine.renderSystem.GetPackedTileInfo(texture);
				const auto mipCount			= textureStreamEngine.renderSystem.GetTextureMipCount(texture);
				const auto max				= XY.Product();

				debug.emplace_back(offset, size);

				for(size_t y = 0; y < XY[1]; y++)
				{
					for (size_t x = 0; x < XY[0]; x++)
					{
						auto sampleOffset			= x + y * XY[0];

						if (max <= sampleOffset)
							DebugBreak();

						auto temp					= results[offset + sampleOffset];
						bool packedPushed			= false;

						if (temp != 0)
						{

							for (size_t i = 0; i <= packedBlockInfo.startingLevel; i++)
							{
								if ((temp & (0x01 << i)) != 0)
								{
									if ((0x01 << (mipCount - i - 1)) < 256)
									{
										const gpuTileID newTile = {
										.TextureID = (uint32_t)texture,
										.tileID = TileID_t{
											.segments = {
												.ytile = 0,
												.xtile = 0,
												.mipLevel = (uint32_t)packedBlockInfo.startingLevel,
												.packed = true,
												}
											}
										};

										if (!packedPushed)
										{
											packedPushed = true;
											requests.push_back(newTile);
										}
									}
									else
									{
										const uint2 mipXY = XY >> i;

										const gpuTileID newTile = {
											.TextureID	= (uint32_t)texture,
											.tileID		= TileID_t{
												.segments = {
													.ytile		= (unsigned int)y,
													.xtile		= (unsigned int)x,
													.mipLevel	= (unsigned int)i,
													.packed		= false,
													}
												}
										};

										if (mipXY[0] > x && mipXY[1] > y)
											requests.push_back(newTile);
									}
								}
							}
						}
					}
				}
			}
		}

		const auto stateUpdateRes	= textureStreamEngine.UpdateTileStates(requests.begin(), requests.end(), &threadLocalAllocator);
		const auto blockAllocations	= textureStreamEngine.AllocateTiles(stateUpdateRes.begin(), stateUpdateRes.end());

		if(blockAllocations)
			textureStreamEngine.PostUpdatedTiles(blockAllocations, threadLocalAllocator);
	}


	/************************************************************************************************/


	void TextureStreamingEngine::BindAsset(const AssetHandle textureAsset, const ResourceHandle  resource)
	{
		const auto comparator =
			[](const MappedAsset& lhs, const MappedAsset& rhs)
			{
				return lhs.GetResourceID() < rhs.GetResourceID();
			};

		const auto res = std::lower_bound(
			std::begin(mappedAssets),
			std::end(mappedAssets),
			MappedAsset{ resource },
			comparator);

		if (res == std::end(mappedAssets) || res->resource != resource)
			mappedAssets.push_back(MappedAsset{ resource, textureAsset });
		else
			res->textureAsset = textureAsset;

		std::sort(std::begin(mappedAssets), std::end(mappedAssets), comparator);
	}


	/************************************************************************************************/


	std::optional<AssetHandle> TextureStreamingEngine::GetResourceAsset(const ResourceHandle  resource) const
	{
		auto comparator =
			[](const MappedAsset& lhs, const MappedAsset& rhs)
			{
				return lhs.GetResourceID() < rhs.GetResourceID();
			};

		auto res = std::lower_bound(
			std::begin(mappedAssets),
			std::end(mappedAssets),
			MappedAsset{ resource },
			comparator);

		if ((res != std::end(mappedAssets) && res->GetResourceID() == resource))
			return std::optional<AssetHandle>{ res->textureAsset };
		else
			return {};
	}


	/************************************************************************************************/


	void TextureStreamingEngine::PostUpdatedTiles(const BlockAllocation& blockChanges, iAllocator& threadLocalAllocator)
	{
		if (blockChanges.allocations.size() == 0 &&
			blockChanges.packedAllocations.size() &&
			blockChanges.reallocations.size()) return;

		auto ctxHandle		= renderSystem.OpenUploadQueue();
		auto& ctx			= renderSystem._GetCopyContext(ctxHandle);
		auto uploadQueue	= renderSystem._GetCopyQueue();
		auto deviceHeap		= renderSystem.GetDeviceResource(heap);

		Vector<ResourceHandle>	updatedTextures		= { &threadLocalAllocator  };
		ResourceHandle			prevResource		= InvalidHandle;
		TextureStreamContext	streamContext		= { &threadLocalAllocator };
		uint2					blockSize			= { 256, 256 };
		TileMapList				mappings			= { &threadLocalAllocator };

		Vector<ResourceHandle>		incompatibleResources	= { &threadLocalAllocator  };
		Vector<DeviceAccessState>	incompatibleState		= { &threadLocalAllocator  };

		// Process reallocated blocks
		auto reallocatedResourceList = blockChanges.reallocations;

		auto cmp_less	= [&](auto& lhs, auto& rhs) { return lhs.resource < rhs.resource; };
		auto cmp_eql	= [&](auto& lhs, auto& rhs) { return lhs.resource == rhs.resource; };
		auto cmp_mipGtr = [&](auto& lhs, auto& rhs) { return lhs.tileID.GetMipLevel() > rhs.tileID.GetMipLevel(); };


		std::sort(
			std::begin(reallocatedResourceList),
			std::end(reallocatedResourceList),
			cmp_less);

		reallocatedResourceList.erase(
			std::unique(
				std::begin(reallocatedResourceList),
				std::end(reallocatedResourceList),
				cmp_eql),
			std::end(reallocatedResourceList));

		for (const auto& block : reallocatedResourceList)
		{
			TileMapList mappings{ allocator };

			const auto resource = block.resource;
			const auto blocks	= filter(
				blockChanges.reallocations,
				[&](auto& block)
				{
					return block.resource == resource;
				});

			for (const AllocatedBlock& block : blocks)
			{
				const TileMapping mapping = {
					.tileID		= block.tileID,
					.heap		= heap,
					.state		= TileMapState::Null,
					.heapOffset	= block.tileIdx,
				};

				mappings.push_back(mapping);
			}

			renderSystem.UpdateTextureTileMappings(resource, mappings);
			updatedTextures.push_back(resource);
		}

		// process newly allocated blocks
		auto allocatedResourceList = blockChanges.allocations;

		std::sort(
			std::begin(allocatedResourceList),
			std::end(allocatedResourceList),
			cmp_less);

		allocatedResourceList.erase(
			std::unique(
				std::begin(allocatedResourceList),
				std::end(allocatedResourceList),
				cmp_eql),
			std::end(allocatedResourceList));

		for (const auto& block : allocatedResourceList)
		{
			const auto resource = block.resource;
			auto asset = GetResourceAsset(resource);
			if (!asset) // Skipped unmapped blocks
				continue;

			const auto deviceResource   = renderSystem.GetDeviceResource(block.resource);

			const auto blocks = [&]
			{
				auto blocks = filter(
					blockChanges.allocations,
					[&](auto& block)
					{
						return block.resource == resource;
					});

				std::sort(
					std::begin(blocks),
					std::end(blocks),
					cmp_mipGtr);

				return blocks;
			}();

			TileMapList mappings{ &threadLocalAllocator };
			for (const AllocatedBlock& block : blocks)
			{
				const auto level = block.tileID.GetMipLevel();
				if (asset && !streamContext.Open(level, asset.value()))
				{
					FK_LOG_ERROR("FAILED TO OPEN STREAM CONTEXT!");
					continue;
				}

				mappings.push_back(
					TileMapping{
						block.tileID,
						heap,
						TileMapState::Updated,
						block.tileIdx
					});

				const auto tile = streamContext.ReadTile(block.tileID, blockSize, ctx);

				FK_LOG_9("CopyTile to tile index: %u, tileID { %u, %u, %u }", block.tileIdx, block.tileID.GetTileX(), block.tileID.GetTileY(), block.tileID.GetMipLevel());

				ctx.CopyTile(
					deviceResource,
					block.tileID,
					block.tileIdx,
					tile);
			}

			renderSystem.UpdateTextureTileMappings(resource, mappings);
			updatedTextures.push_back(resource);
		}


		auto packedAllocations = blockChanges.packedAllocations;
		// process newly allocated tiled blocks
		std::sort(
			std::begin(packedAllocations),
			std::end(packedAllocations),
			[&](auto& lhs, auto& rhs)
			{
				return lhs.resource < rhs.resource;
			});

		for (auto& packedBlock : packedAllocations)
		{
			const auto resource			= packedBlock.resource;
			const auto asset			= GetResourceAsset(resource);

			if (!asset) {
				continue;
			}

			const auto deviceResource	= renderSystem.GetDeviceResource(resource);
			const auto packedBlockInfo	= renderSystem.GetPackedTileInfo(deviceResource);

			const auto startingLevel	= packedBlockInfo.startingLevel;
			const auto endingLevel		= packedBlockInfo.endingLevel;

			TileMapList mappings{ &threadLocalAllocator };

			mappings.push_back(
				TileMapping{
					packedBlock.tileID,
					heap,
					TileMapState::Updated,
					packedBlock.tileIdx
				});


			for (auto level = startingLevel; level < endingLevel; level++)
			{
				if (!streamContext.Open(level, asset.value()))
					continue;

				const auto MIPLevelInfo = GetMIPLevelInfo(level, streamContext.WH(), streamContext.Format());

				const TileID_t tileID	= CreateTileID( 0, 0, level );
				const auto tile			= streamContext.Read(MIPLevelInfo.WH, ctx);

				ctx.CopyTextureRegion(deviceResource, level, { 0, 0, 0 }, tile, MIPLevelInfo.WH, streamContext.Format());
			}

			renderSystem.UpdateTextureTileMappings(resource, mappings);
			updatedTextures.push_back(resource);
			mappings.clear();
		}

		// Submit Texture tiling changes
		std::sort(std::begin(updatedTextures), std::end(updatedTextures));

		updatedTextures.erase(
			std::unique(
				std::begin(updatedTextures),
				std::end(updatedTextures)),
			std::end(updatedTextures));

		renderSystem.SyncUploadTo(renderSystem.SyncDirectPoint());
		renderSystem.SyncUploadTo(renderSystem.SyncUploadPoint());
		renderSystem.SubmitTileMappings(updatedTextures, &threadLocalAllocator);
		renderSystem.SubmitUploadQueues(&ctxHandle, 1);
		renderSystem.SyncDirectTicket();
		renderSystem.SyncDirectTo(renderSystem.SyncUploadTicket());
	}


	/************************************************************************************************/


	Vector<gpuTileID> TextureBlockAllocator::UpdateTileStates(std::span<const gpuTileID> blocks, iAllocator* allocator)
	{
		auto pred = [](const uint64_t lhs, const uint64_t rhs) -> bool
		{
			return lhs < rhs;
		};

		std::sort(inuse.begin(), inuse.end(), pred);

		auto findInuse = [&](const gpuTileID gpuTile) -> TextureBlockAllocator::Block*
		{
			for (auto& tile : inuse)
				if (tile.tileID.bytes == gpuTile.tileID.bytes &&
					tile.resource == gpuTile.TextureID)
						return &tile;

			return nullptr;
		};

		auto findstale = [&](const gpuTileID gpuTile) -> TextureBlockAllocator::Block*
		{
			for (auto& tile : stale)
				if (tile.tileID.bytes == gpuTile.tileID.bytes &&
					tile.resource == gpuTile.TextureID)
						return &tile;

			return nullptr;
		};

		for (auto& itr : inuse)
			itr.staleFrameCount++;

		Vector<gpuTileID> allocationsNeeded{ allocator };
		for (auto& block : blocks)
		{
			if (ResourceHandle{ block.TextureID } == InvalidHandle)
				continue;

			if (auto res = findInuse(block); res) {
				res->state				= TextureBlockAllocator::EBlockState::InUse;
				res->staleFrameCount	= 0;
			}
			else if (auto res = findstale(block); res)
			{
				auto staleTile = *res;
				stale.remove_unstable(res);

				staleTile.state				= TextureBlockAllocator::EBlockState::InUse;
				staleTile.staleFrameCount	= 0;

				inuse.push_back(staleTile);
			}
			else
				allocationsNeeded.push_back(block);
		}

		for (auto& tile : stale)
			tile.staleFrameCount++;

		for (auto& tile : inuse)
			if(tile.staleFrameCount > 120)
				stale.push_back(tile);

		inuse.erase(
			std::remove_if(inuse.begin(), inuse.end(),
				[](auto& tile)
				{
					return (tile.staleFrameCount > 120);
				})
			, inuse.end());

		std::sort(allocationsNeeded.begin(), allocationsNeeded.end(),
			[](auto& lhs, auto& rhs)
			{
				return lhs > rhs;
			});

		return allocationsNeeded;
	}


	/************************************************************************************************/


	BlockAllocation TextureBlockAllocator::AllocateBlocks(std::span<const gpuTileID> blocks, iAllocator* allocator)
	{
		AllocatedBlockList reallocatedBlocks{ allocator };

		std::sort(inuse.begin(), inuse.end(),
			[&](const Block& lhs, const Block& rhs)
			{
				return !(lhs < rhs);
			});

		std::sort(stale.begin(), stale.end(),
			[&](const Block& lhs, const Block& rhs)
			{
				return lhs.staleFrameCount < rhs.staleFrameCount;
			});

		Vector<TextureBlockAllocator::Block> usedBlocks	{ allocator };

		std::for_each(blocks.begin(), blocks.end(),
			[&](gpuTileID tile)
			{
				if (tile.tileID.bytes == -1)
					return;

				if (ResourceHandle{ tile.TextureID } == InvalidHandle)
					return;

				if (free.size())
				{
					auto block		= free.pop_back();
					block.tileID	= tile.tileID;
					block.resource	= ResourceHandle{ tile.TextureID };
					block.state		= EBlockState::InUse;
					usedBlocks.push_back(block);

					return;
				}

				if (stale.size())
				{
					for (auto itr = stale.end() - 1; itr > stale.begin(); itr--)
					{
						auto block = *itr;
						if (block.tileID.packed() || block.tileID.GetMipLevel() > tile.tileID.GetMipLevel())
							continue;

						AllocatedBlock reallocation{
							.tileID		= block.tileID,
							.resource	= block.resource,
							.offset		= block.blockID * 64 * KILOBYTE,
							.tileIdx	= block.blockID
						};

						block.tileID	= tile.tileID;
						block.resource	= ResourceHandle{ tile.TextureID };
						block.state		= EBlockState::InUse;
						block.staleFrameCount = 0;

						reallocatedBlocks.push_back(reallocation);
						usedBlocks.push_back(block);
						stale.remove_unstable(itr);

						return;
					}
				}

				if (inuse.size())
				{
					for (auto itr = inuse.end() - 1; itr > inuse.begin(); itr--)
					{
						auto block = *itr;

						if (block.resource != tile.TextureID)
							continue;

						if (block.tileID.packed() || block.tileID.GetMipLevel() > tile.tileID.GetMipLevel() || block.staleFrameCount <= 10)
							continue;

						AllocatedBlock reallocation{
							.tileID		= block.tileID,
							.resource	= block.resource,
							.offset		= block.blockID * 64 * KILOBYTE,
							.tileIdx	= block.blockID
						};

						block.tileID	= tile.tileID;
						block.resource	= ResourceHandle{ tile.TextureID };
						block.state		= EBlockState::InUse;
						block.staleFrameCount = 0;

						reallocatedBlocks.push_back(reallocation);
						usedBlocks.push_back(block);
						inuse.remove_unstable(itr);

						return;
					}
				}
			});

		AllocatedBlockList allocatedBlocks{ allocator };
		AllocatedBlockList packedBlocks{ allocator };

		for(auto tile : usedBlocks)
		{
			 AllocatedBlock allocation{
				.tileID		= tile.tileID,
				.resource	= ResourceHandle{ tile.resource },
				.offset		= tile.blockID * 64 * KILOBYTE,
				.tileIdx	= tile.blockID
			};

			inuse.push_back(tile);

			if (tile.tileID.GetMipLevel() > 15)
				DebugBreak();

			if (tile.tileID.packed())
				packedBlocks.push_back(allocation);
			else
				allocatedBlocks.push_back(allocation);
		}

		return {
			std::move(reallocatedBlocks),
			std::move(allocatedBlocks),
			std::move(packedBlocks) };
	}


	/************************************************************************************************/


	BlockAllocation TextureStreamingEngine::AllocateTiles(const gpuTileID* begin, const gpuTileID* end)
	{
		return textureBlockAllocator.AllocateBlocks({ begin, end }, allocator);
	}


	/************************************************************************************************/


	void TextureStreamingEngine::LoadLowestLevel(ResourceHandle textureResource, CopyContextHandle copyQueue)
	{
		auto& copyCtx				= renderSystem.copyEngine.copyContexts[copyQueue];
		const auto deviceResource	= renderSystem.GetDeviceResource(textureResource);
		const auto packedBlockInfo	= renderSystem.GetPackedTileInfo(deviceResource);
		const auto mipCount			= renderSystem.GetTextureMipCount(textureResource);

		auto tileID					= CreatePackedID();
		tileID.segments.mipLevel	= packedBlockInfo.startingLevel;

		const gpuTileID gpuID		= { textureResource, tileID };
		auto allocation				= textureBlockAllocator.AllocateBlocks({ &gpuID, &gpuID + 1 }, allocator);

		if (!allocation.packedAllocations.size())
			return;

		TextureStreamContext	streamContext = { allocator };
		auto asset = GetResourceAsset(textureResource);

		if (!asset.has_value())
			return;


		for (auto level = packedBlockInfo.startingLevel; level < packedBlockInfo.endingLevel; level++)
		{
			if (!streamContext.Open(level, asset.value()))
				continue;

			const auto MIPLevelInfo = GetMIPLevelInfo(level, streamContext.WH(), streamContext.Format());

			const TileID_t tileID	= CreateTileID( 0, 0, level );
			const auto tile			= streamContext.Read(MIPLevelInfo.WH, copyCtx);

			copyCtx.CopyTextureRegion(deviceResource, level, { 0, 0, 0 }, tile, MIPLevelInfo.WH, streamContext.Format());
		}

		const TileMapping mapping[] = { {
			.tileID		= tileID,
			.heap		= heap,
			.state		= TileMapState::InUse,
			.heapOffset	= allocation.packedAllocations.front().tileIdx,
		} };

		renderSystem.UpdateTextureTileMappings(textureResource, { mapping, 1 });
		renderSystem.SubmitTileMappings({ &textureResource, &textureResource + 1 }, allocator);
	}


	/************************************************************************************************/


	gpuTileList TextureStreamingEngine::UpdateTileStates(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator)
	{
		return textureBlockAllocator.UpdateTileStates({ begin, end }, allocator);
	}


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2023 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
