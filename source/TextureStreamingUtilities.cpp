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


	std::optional<DDSDecompressor*> CreateDDSDecompressor(ReadContext& readCtx, const uint32_t MIPlevel, AssetHandle asset, iAllocator* allocator)
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


	ID3D12PipelineState* CreateTextureFeedbackPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("Forward_VS",				"vs_6_5", "assets\\shaders\\forwardRender.hlsl");
		auto PShader = RS->LoadShader("TextureFeedback_PS",		"ps_6_5", "assets\\shaders\\TextureFeedback.hlsl");

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
		PSO_Desc.pRootSignature			= RS->Library.RSDefault;
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
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackAnimatedPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ForwardSkinned_VS",  "vs_6_0", "assets\\shaders\\forwardRender.hlsl");
		auto PShader = RS->LoadShader("TextureFeedback_PS", "ps_6_0", "assets\\shaders\\TextureFeedback.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,	5, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // enable conservative rast

		if(RS->features.conservativeRast == AvailableFeatures::ConservativeRast_AVAILABLE)
			Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthEnable      = true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = { 0 };
		PSO_Desc.pRootSignature			= RS->Library.RSDefault;
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
		auto computeShader = RS->LoadShader(
			"CompressBlocks", "cs_6_5",
			"assets\\shaders\\TextureFeedbackCompressor.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackBlockSizePreFixSum(RenderSystem* RS)
	{
		auto computeShader = RS->LoadShader(
			"PreFixSumBlockSizes", "cs_6_5",
			"assets\\shaders\\TextureFeedbackBlockPreFixSum.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackMergeBlocks(RenderSystem* RS)
	{
		auto computeShader = RS->LoadShader(
			"MergeBlocks", "cs_6_5",
			"assets\\shaders\\TextureFeedbackMergeBlocks.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTextureFeedbackSetBlockSizes(RenderSystem* RS)
	{
		auto computeShader = RS->LoadShader(
			"SetBlockCounters", "cs_6_5",
			"assets\\shaders\\TextureFeedbackMergeBlocks.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader,
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(
			&desc,
			IID_PPV_ARGS(&PSO));

		return PSO;
	}


	/************************************************************************************************/

	static const uint64_t FeedbackBufferSize = sizeof(uint2) * 1024 * 256 + 1024;


	TextureStreamingEngine::TextureStreamingEngine(
				RenderSystem&   IN_renderSystem,
				iAllocator*     IN_allocator,
				const TextureCacheDesc& desc) : 
			allocator				{ IN_allocator		},
			textureBlockAllocator	{ desc.textureCacheSize / desc.blockSize,   IN_allocator },
			renderSystem			{ IN_renderSystem	},
			settings				{ desc				},
			feedbackReturnBuffer	{ IN_renderSystem.CreateReadBackBuffer(2 * MEGABYTE) },
			heap					{ IN_renderSystem.CreateHeap(desc.textureCacheSize, 0) },
			mappedAssets			{ IN_allocator },
			timeStats				{ IN_renderSystem.CreateTimeStampQuery(512) }
	{
		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKPASS,
			{ &renderSystem.Library.RSDefault, CreateTextureFeedbackPassPSO });

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKANIMATEDPASS,
			{ &renderSystem.Library.RSDefault, CreateTextureFeedbackAnimatedPassPSO });

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKCOMPRESSOR,
			{ &renderSystem.Library.RSDefault, CreateTextureFeedbackCompressorPSO });

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKPREFIXSUMBLOCKSIZES,
			{ &renderSystem.Library.RSDefault, CreateTextureFeedbackBlockSizePreFixSum });

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKMERGEBLOCKS,
			{ &renderSystem.Library.RSDefault, CreateTextureFeedbackMergeBlocks });

		renderSystem.RegisterPSOLoader(
			TEXTUREFEEDBACKSETBLOCKSIZES,
			{ &renderSystem.Library.RSDefault, CreateTextureFeedbackSetBlockSizes });

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
		renderSystem.QueuePSOLoad(TEXTUREFEEDBACKANIMATEDPASS);

		renderSystem.QueuePSOLoad(TEXTUREFEEDBACKCOMPRESSOR);
		renderSystem.QueuePSOLoad(TEXTUREFEEDBACKPREFIXSUMBLOCKSIZES);
		renderSystem.QueuePSOLoad(TEXTUREFEEDBACKMERGEBLOCKS);
	}


	/************************************************************************************************/


	TextureStreamingEngine::~TextureStreamingEngine()
	{
		updateInProgress = false;

		renderSystem.SetReadBackEvent(
			feedbackReturnBuffer,
			[](ReadBackResourceHandle resource){});

		while (taskInProgress);

		renderSystem.WaitforGPU(); // Flush any pending reads
		renderSystem.FlushPendingReadBacks();

		renderSystem.ReleaseReadBack(feedbackReturnBuffer);
	}


	/************************************************************************************************/


	void TextureStreamingEngine::TextureFeedbackPass(
			UpdateDispatcher&				dispatcher,
			FrameGraph&						frameGraph,
			CameraHandle					camera,
			uint2							renderTargetWH,
			GatherPassesTask&				sceneGather,
			GatherSkinnedTask&				skinnedModelsGather,
			ReserveConstantBufferFunction&	reserveCB,
			ReserveVertexBufferFunction&	reserveVB)
	{
		if (updateInProgress)
			return;

		updateInProgress = true;

		frameGraph.AddNode<TextureFeedbackPass_Data>(
			TextureFeedbackPass_Data{
				camera,
				sceneGather,
				skinnedModelsGather,
				reserveCB,
			},
			[&](FrameGraphNodeBuilder& builder, TextureFeedbackPass_Data& data)
			{
				builder.AddDataDependency(sceneGather);

				data.feedbackBuffers[0]			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4 * MEGABYTE), DRS_UAV);
				data.feedbackBuffers[1]			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4 * MEGABYTE), DRS_UAV);
				data.feedbackCounters			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 512), DRS_UAV);
				data.feedbackBlockSizes			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 512), DRS_UAV);
				data.feedbackBlockOffsets		= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 512), DRS_UAV);
				data.feedbackDepth				= builder.AcquireVirtualResource(GPUResourceDesc::DepthTarget({ 128, 128 }, DeviceFormat::D32_FLOAT), DRS_DEPTHBUFFERWRITE);

				data.readbackBuffer				= feedbackReturnBuffer;

				builder.SetDebugName(data.feedbackBuffers[0],	"feedbackBuffer0");
				builder.SetDebugName(data.feedbackBuffers[1],	"feedbackBuffer1");
				builder.SetDebugName(data.feedbackCounters,		"feedbackCounters");
				builder.SetDebugName(data.feedbackBlockSizes,	"feedbackBlockSizes");
				builder.SetDebugName(data.feedbackBlockOffsets,	"feedbackBlockOffsets");
				builder.SetDebugName(data.feedbackDepth,		"feedbackDepth");
			},
			[=](TextureFeedbackPass_Data& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

				ctx.BeginEvent_DEBUG("Texture Feedback");

				auto& brushes			= data.pvs.GetData().solid;
				auto& materials			= MaterialComponent::GetComponent();
				auto& skinnedBrushes	= data.skinnedModels.GetData().skinned;

				if (!brushes.size()) {
					updateInProgress = false;
					return;
				}


				ctx.SetPrimitiveTopology(EIT_TRIANGLELIST);
				ctx.ClearDepthBuffer(resources.GetRenderTarget(data.feedbackDepth), 1.0f);

				ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.feedbackDepth) });
				ctx.SetRenderTargets({}, true, resources.GetRenderTarget(data.feedbackDepth));

				ctx.DiscardResource(resources.Transition(data.feedbackBlockOffsets,	DRS_UAV, ctx));
				ctx.DiscardResource(resources.Transition(data.feedbackBlockSizes,	DRS_UAV, ctx));
				ctx.DiscardResource(resources.Transition(data.feedbackCounters,		DRS_UAV, ctx));
				ctx.DiscardResource(resources.Transition(data.feedbackBuffers[0],	DRS_UAV, ctx));
				ctx.DiscardResource(resources.Transition(data.feedbackBuffers[1],	DRS_UAV, ctx));

				struct alignas(512) constantBufferLayout
				{
					float		bias;
					float		padding[3];
					uint32_t	zeroBlock[256];
				} constants =
				{
					std::log2f(128.0f / renderTargetWH[0]),
					{ 0.0f }
				};

				struct alignas(512) animationConstantsLayout
				{
					float4x4 m[512];
				};

				memset(constants.zeroBlock, 0, sizeof(constants.zeroBlock));

				size_t totalDrawCallCount = 0;
				for (auto& brush : brushes)
				{
					auto end = brush->meshes.size();
					for (size_t I = 0; I < end; I++)
					{
						auto mesh			= brush->meshes[I];
						auto* triMesh		= GetMeshResource(mesh);
						totalDrawCallCount += triMesh->lods[brush.LODlevel[I]].subMeshes.size();
					}
				}
				for (auto& skinnedDraw : skinnedBrushes)
				{
					auto end = skinnedDraw.brush->meshes.size();
					for (size_t I = 0; I < end; I++)
					{
						auto mesh			= skinnedDraw.brush->meshes[I];
						const auto LODlevel = skinnedDraw.lodLevel[I];
						const auto brush	= skinnedDraw.brush;
						auto* triMesh		= GetMeshResource(skinnedDraw.brush->meshes[I]);

						totalDrawCallCount += triMesh->lods[LODlevel].subMeshes.size();
					}
				}

				const size_t bufferSize =
					AlignedSize<Brush::VConstantsLayout>() * totalDrawCallCount +
					AlignedSize<constantBufferLayout>() +
					AlignedSize<Camera::ConstantBuffer>();

				CBPushBuffer passConstantBuffer		{ data.reserveCB(bufferSize) };
				CBPushBuffer skinnedConstantsBuffer	{ data.reserveCB(sizeof(animationConstantsLayout) * skinnedBrushes.size()) };

				const auto passConstants	= ConstantBufferDataSet{ constants, passConstantBuffer };
				const auto cameraConstants	= ConstantBufferDataSet{ GetCameraConstants(camera), passConstantBuffer };

				ctx.ClearUAVBufferRange(resources.GetResource(data.feedbackCounters), 0, 4096);
				ctx.AddUAVBarrier(resources.GetResource(data.feedbackCounters));

				ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);

				DescriptorHeap uavHeap;
				uavHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);
				uavHeap.SetUAVStructured(
					ctx, 0,
					resources.Transition(data.feedbackBuffers[0], DRS_UAV, ctx),
					resources.Transition(data.feedbackCounters, DRS_UAV, ctx),
					sizeof(uint64_t), 0);

				ctx.SetGraphicsDescriptorTable(5, uavHeap);
				ctx.SetGraphicsConstantBufferView(0, cameraConstants);
				ctx.SetGraphicsConstantBufferView(2, passConstants);
				ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

				TriMesh*		prevMesh		= nullptr;
				MaterialHandle	prevMaterial	= InvalidHandle;

				ctx.TimeStamp(timeStats, 0);

				ctx.BeginEvent_DEBUG("Texture Feedback Pass");

				size_t element = 0;
				for (size_t J = element; J < brushes.size(); J++)
				{
					auto& brush = brushes[J];
					size_t end	= brush->meshes.size();
					for(size_t I = 0; I < end; I++)
					{
						auto* triMesh = GetMeshResource(brush->meshes[I]);

						ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACKPASS));

						if (triMesh != prevMesh)
						{
							prevMesh = triMesh;

							ctx.AddIndexBuffer(triMesh, brush.LODlevel[I]);
							ctx.AddVertexBuffers(
								triMesh,
								brush.LODlevel[I],
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

						const auto materialHandle	= brush->material;
						const auto& lod				= triMesh->lods[brush.LODlevel[I]];

						if (!materials[materialHandle].SubMaterials.empty())
						{
							const auto subMeshCount = lod.subMeshes.size();
							auto material = MaterialComponent::GetComponent()[brush->material];

							bool animated = false;
							if (animated)
							{
								animationConstantsLayout constants;
								auto skinnedConstants = ConstantBufferDataSet{ constants, passConstantBuffer };

								ctx.SetGraphicsConstantBufferView(2, passConstants);

								ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACKANIMATEDPASS));
							}
							else
								ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACKPASS));

							for (size_t I = 0; I < subMeshCount; I++)
							{
								auto& subMesh				= lod.subMeshes[I];
								const auto passMaterial		= materials[material.SubMaterials[I]];

								DescriptorHeap srvHeap;

								const auto& textures = passMaterial.Textures;

								srvHeap.Init(
									ctx,
									resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
									&allocator);

								auto constantData = brush->GetConstants();
								constantData.textureCount = textures.size();

								if(constantData.textureCount)
								{
									for (size_t I = 0; I < textures.size(); I++)
									{
										srvHeap.SetSRV(ctx, I, textures[I]);
										constantData.textureHandles[I] = uint4{ 256, 256, textures[I].to_uint() };
									}

									srvHeap.NullFill(ctx);

									const auto constants = ConstantBufferDataSet{ constantData, passConstantBuffer };

									ctx.SetGraphicsConstantBufferView(1, constants);
									ctx.SetGraphicsDescriptorTable(4, srvHeap);
									ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
								}
							}
						}
						else
						{
							if (materialHandle != prevMaterial)
							{
								const auto& textures = materials[materialHandle].Textures;

								if(textures.size())
								{
									DescriptorHeap srvHeap;
									srvHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), textures.size(), &allocator);
									ctx.SetGraphicsDescriptorTable(4, srvHeap);

									for (size_t I = 0; I < textures.size(); I++)
										srvHeap.SetSRV(ctx, I, textures[I]);
								}
							}

							const auto& textures = materials[materialHandle].Textures;

							prevMaterial = materialHandle;

							const auto constants = ConstantBufferDataSet{ brush->GetConstants(), passConstantBuffer };

							ctx.SetGraphicsConstantBufferView(1, constants);
							ctx.DrawIndexed(lod.GetIndexCount());
						}
					}
				}

				ctx.EndEvent_DEBUG();
				ctx.TimeStamp(timeStats, 1);

				auto CompressionPass = [&](auto Source, auto Destination)
				{
					ctx.BeginEvent_DEBUG("CompressBlock");

					DescriptorHeap uavCompressHeap;
					uavCompressHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 3, &allocator);
					uavCompressHeap.SetUAVBuffer	(ctx, 0, resources.Transition(data.feedbackCounters, DRS_UAV, ctx));
					uavCompressHeap.SetUAVStructured(ctx, 1, resources.GetResource(Source), sizeof(uint64_t));
					uavCompressHeap.SetUAVBuffer	(ctx, 2, resources.Transition(data.feedbackBlockSizes, DRS_UAV, ctx));

					const uint32_t blockCount = (MEGABYTE * 4) / (1024 * sizeof(uint64_t));

					ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);
					ctx.AddUAVBarrier();


					ctx.SetComputeDescriptorTable(5, uavCompressHeap);
					ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKCOMPRESSOR), { blockCount, 1, 1 });

					ctx.EndEvent_DEBUG();
					ctx.BeginEvent_DEBUG("PreFixSum");

					ctx.AddUAVBarrier(resources.GetResource(data.feedbackCounters));
					ctx.AddUAVBarrier(resources.GetResource(data.feedbackBlockSizes));

					// Prefix Sum
					DescriptorHeap srvPreFixSumHeap;
					srvPreFixSumHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 1, &allocator);
					srvPreFixSumHeap.SetStructuredResource(ctx, 0, resources.Transition(data.feedbackBlockSizes, DRS_NonPixelShaderResource, ctx), sizeof(uint32_t));

					DescriptorHeap uavPreFixSumHeap;
					uavPreFixSumHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);
					uavPreFixSumHeap.SetUAVStructured(ctx, 0, resources.Transition(data.feedbackBlockOffsets, DRS_UAV, ctx), sizeof(uint32_t));

					ctx.SetComputeDescriptorTable(4, srvPreFixSumHeap);
					ctx.SetComputeDescriptorTable(5, uavPreFixSumHeap);
					ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKPREFIXSUMBLOCKSIZES), { 1 , 1, 1 });

					ctx.EndEvent_DEBUG();
					ctx.BeginEvent_DEBUG("Merge");

					//  Merge partial blocks
					DescriptorHeap srvMergeHeap;
					srvMergeHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 3, &allocator);
					srvMergeHeap.SetStructuredResource(ctx, 0, resources.Transition(Source,						DRS_NonPixelShaderResource, ctx), sizeof(uint64_t));
					srvMergeHeap.SetStructuredResource(ctx, 1, resources.Transition(data.feedbackBlockSizes,	DRS_NonPixelShaderResource, ctx), sizeof(uint32_t));
					srvMergeHeap.SetStructuredResource(ctx, 2, resources.Transition(data.feedbackBlockOffsets,	DRS_NonPixelShaderResource, ctx), sizeof(uint32_t));

					DescriptorHeap uavMergeHeap;
					uavMergeHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 2, &allocator);
					uavMergeHeap.SetUAVStructured(ctx, 0, resources.Transition(Destination,			DRS_UAV, ctx), sizeof(uint64_t));
					uavMergeHeap.SetUAVBuffer(ctx, 1, resources.Transition(data.feedbackCounters,	DRS_UAV, ctx), sizeof(uint32_t));

					ctx.SetComputeDescriptorTable(4, srvMergeHeap);
					ctx.SetComputeDescriptorTable(5, uavMergeHeap);

					ctx.AddUAVBarrier();

					ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKMERGEBLOCKS), { blockCount , 1, 1 });
					ctx.AddUAVBarrier();
					ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKSETBLOCKSIZES), { 1, 1, 1 });

					ctx.AddUAVBarrier();
					ctx.EndEvent_DEBUG();
				};

				ctx.BeginEvent_DEBUG("Texture feedback stream compression");

				const size_t passCount = 4;
				for (size_t itr = 0; itr < passCount; itr++)
					CompressionPass(data.feedbackBuffers[itr % 2], data.feedbackBuffers[(itr + 1) % 2]);

				// Write out
				ctx.CopyBufferRegion(
					{   resources.GetDeviceResource(resources.Transition(data.feedbackCounters,					DRS_CopySrc, ctx)) ,
						resources.GetDeviceResource(resources.Transition(data.feedbackBuffers[passCount % 2],	DRS_CopySrc, ctx)) },
					{ 0, 0 },
					{   resources.GetDeviceResource(data.readbackBuffer),
						resources.GetDeviceResource(data.readbackBuffer) },
					{ 0, 64 },
					{ 8, 2048 * sizeof(uint2) },
					{ DRS_CopyDest, DRS_CopyDest },
					{ DRS_CopyDest, DRS_CopyDest });

				//ctx.ResolveQuery(timeStats, 0, 4, resources.GetObjectResource(data.readbackBuffer), 8);
				ctx.QueueReadBack(data.readbackBuffer);

				ctx.EndEvent_DEBUG();
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
		uint32_t	requestCount	= 0;
		gpuTileID*	requests		= nullptr;

		auto [buffer, bufferSize] = textureStreamEngine.renderSystem.OpenReadBackBuffer(resource);
		EXITSCOPE(textureStreamEngine.renderSystem.CloseReadBackBuffer(resource));

		std::chrono::high_resolution_clock Clock;
		auto Before = Clock.now();

		EXITSCOPE(
				auto After		= Clock.now();
				auto Duration	= std::chrono::duration_cast<std::chrono::microseconds>(After - Before);

				textureStreamEngine.updateTime = float(Duration.count()) / 1000.0f; );

		if (!buffer)
			return;
		
		uint32_t timePointA = 0;
		uint32_t timePointB = 0;

		memcpy(&requestCount, (char*)buffer, sizeof(requestCount));
		memcpy(&timePointA, (char*)buffer + 8, sizeof(timePointA));
		memcpy(&timePointB, (char*)buffer + 16, sizeof(timePointB));

		UINT64 timeStampFreq = 0;
		textureStreamEngine.renderSystem.GraphicsQueue->GetTimestampFrequency(&timeStampFreq);
		textureStreamEngine.passTime = float(timePointB - timePointA) / timeStampFreq * 1000;

		EXITSCOPE(threadLocalAllocator.free(requests));

		requestCount = Min(requestCount, 2048);

		if (requestCount)
		{
			const auto requestSize = requestCount * sizeof(uint2);
			requests = reinterpret_cast<gpuTileID*>(threadLocalAllocator.malloc(MEGABYTE));
			memcpy(requests, (std::byte*)buffer + 64, requestSize);
		}

		if (!requests || !requestCount)
			return;


		const auto lg_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool { return lhs.GetSortingID() <  rhs.GetSortingID(); };
		const auto eq_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool { return lhs.GetSortingID() == rhs.GetSortingID(); };

		FK_LOG_9("Request Size: %u / %u", requestCount, (bufferSize) / sizeof(uint2));

		std::sort(
			requests,
			requests + requestCount,
			lg_comparitor);

		auto end            = std::unique(requests, requests + requestCount, eq_comparitor);
		auto uniqueCount    = (size_t)(end - requests);
		size_t itr          = 0;

		while ((end - 1)->TextureID == -1 && uniqueCount) { end--; uniqueCount--; }

#if 0// Doesn't seem to be an issue at the moment?
		// TODO: MORE TESTING NEEDED HERE. 
		if (uniqueCount && (requests + uniqueCount - 1)->TextureID > 0xff)
			DebugBreak();
#endif   

		for (;itr < uniqueCount; ++itr)
		{
			const auto tile				= requests[itr];
			const auto textureHandle	= ResourceHandle{ tile.TextureID };

			if (!textureStreamEngine.GetResourceAsset(textureHandle).has_value())
				continue;

			if(!tile.IsPacked())
			{
				const auto mipLevel	= tile.tileID.GetMipLevel() + 1;
				const auto tileX	= tile.tileID.GetTileX() / 2;
				const auto tileY	= tile.tileID.GetTileY() / 2;

				auto mipCount		= textureStreamEngine.renderSystem.GetTextureMipCount(textureHandle);
				auto WH				= textureStreamEngine.renderSystem.GetTextureWH(textureHandle);
				auto tileWH			= textureStreamEngine.renderSystem.GetTextureTilingWH(textureHandle, tile.tileID.GetMipLevel());
				auto tileSize		= GetFormatTileSize(textureStreamEngine.renderSystem.GetTextureFormat(textureHandle));
				size_t packBegin	= log2(Min(WH[0], WH[1])) - log2(Min(tileSize[0], tileSize[1])) + 1;

				if(packBegin > mipLevel){
					const gpuTileID newTile = {
						.TextureID  = tile.TextureID,
						.tileID     = TileID_t{
							.segments = {
								.ytile      = tileY,
								.xtile      = tileX,
								.mipLevel   = mipLevel,
								.packed     = false,
							}
						}
					};

					requests[uniqueCount++] = newTile;
				}
				else
				{
					const gpuTileID newTile = {
						.TextureID  = tile.TextureID,
						.tileID     = TileID_t{
							.segments = {
								.ytile      = 0,
								.xtile      = 0,
								.mipLevel   = (uint)packBegin,
								.packed     = true
							}
						}
					};

					requests[uniqueCount++] = newTile;
				}
			}
		}

		std::sort(requests, requests + uniqueCount, lg_comparitor);

		end            = std::unique(requests, requests + uniqueCount, eq_comparitor);
		uniqueCount    = (size_t)(end - requests);

		const auto stateUpdateRes     = textureStreamEngine.UpdateTileStates(requests, requests + uniqueCount, &threadLocalAllocator);
		const auto blockAllocations   = textureStreamEngine.AllocateTiles(stateUpdateRes.begin(), stateUpdateRes.end());

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

		/*
		return (res != std::end(mappedAssets) && res->GetResourceID() == resource) ?
			std::optional<AssetHandle>{ res->textureAsset } :
			std::optional<AssetHandle>{};
			*/

		if ((res != std::end(mappedAssets) && res->GetResourceID() == resource))
			return std::optional<AssetHandle>{ res->textureAsset };
		else
		//    __debugbreak();
			return {};
	}


	/************************************************************************************************/


	void TextureStreamingEngine::PostUpdatedTiles(const BlockAllocation& blockChanges, iAllocator& threadLocalAllocator)
	{
		if (blockChanges.allocations.size() == 0 &&
			blockChanges.packedAllocations.size() &&
			blockChanges.reallocations.size()) return;

		auto ctxHandle      = renderSystem.OpenUploadQueue();
		auto& ctx           = renderSystem._GetCopyContext(ctxHandle);
		auto uploadQueue    = renderSystem._GetCopyQueue();
		auto deviceHeap     = renderSystem.GetDeviceResource(heap);

		Vector<ResourceHandle>  updatedTextures     = { &threadLocalAllocator  };
		ResourceHandle          prevResource        = InvalidHandle;
		TextureStreamContext    streamContext       = { &threadLocalAllocator };
		uint2                   blockSize           = { 256, 256 };
		TileMapList             mappings            = { &threadLocalAllocator };

		Vector<ResourceHandle>          incompatibleResources   = { &threadLocalAllocator  };
		Vector<DeviceResourceState>     incompatibleState       = { &threadLocalAllocator  };

		// Process reallocated blocks
		auto reallocatedResourceList = blockChanges.reallocations;

		std::sort(
			std::begin(reallocatedResourceList),
			std::end(reallocatedResourceList),
			[&](auto& lhs, auto& rhs)
			{
				return lhs.resource < rhs.resource;
			});

		reallocatedResourceList.erase(
			std::unique(
				std::begin(reallocatedResourceList),
				std::end(reallocatedResourceList),
				[&](auto& lhs, auto& rhs)
				{
					return lhs.resource == rhs.resource;
				}),
			std::end(reallocatedResourceList));

		for (const auto& block : reallocatedResourceList)
		{
			TileMapList mappings{ allocator };

			const auto resource = block.resource;
			const auto blocks = filter(
				blockChanges.reallocations,
				[&](auto& block)
				{
					return block.resource == resource;
				});

			for (const AllocatedBlock& block : blocks)
			{
				const TileMapping mapping = {
					.tileID     = block.tileID,
					.heap       = heap,
					.state      = TileMapState::Null,
					.heapOffset = block.tileIdx,
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
			[&](auto& lhs, auto& rhs)
			{
				return lhs.resource < rhs.resource;
			});

		allocatedResourceList.erase(
			std::unique(
				std::begin(allocatedResourceList),
				std::end(allocatedResourceList),
				[&](auto& lhs, auto& rhs)
				{
					return lhs.resource == rhs.resource;
				}),
			std::end(allocatedResourceList));

		for (const auto& block : allocatedResourceList)
		{
			const auto resource = block.resource;
			auto asset = GetResourceAsset(resource);
			if (!asset) // Skipped unmapped blocks
				continue;

			const auto deviceResource   = renderSystem.GetDeviceResource(block.resource);
			const auto resourceState    = renderSystem.GetObjectState(block.resource);

			if (resourceState != DeviceResourceState::DRS_Common)
			{
				incompatibleResources.push_back(block.resource);
				incompatibleState.push_back(resourceState);
			}

			if (resourceState != DeviceResourceState::DRS_CopyDest)
				ctx.Barrier(
					deviceResource,
					DRS_Common,
					DRS_CopyDest);

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
					[&](auto& lhs, auto& rhs)
					{
						return lhs.tileID.GetMipLevel() > rhs.tileID.GetMipLevel();
					});

				return blocks;
			}();

			TileMapList mappings{ &threadLocalAllocator };
			for (const AllocatedBlock& block : blocks)
			{
				const auto level = block.tileID.GetMipLevel();
				if (!streamContext.Open(level, asset.value()))
					continue;

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


			ctx.Barrier(
				deviceResource,
				DeviceResourceState::DRS_CopyDest,
				DeviceResourceState::DRS_Common);

			renderSystem.SetObjectState(resource, DeviceResourceState::DRS_Common);
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
			const auto resource         = packedBlock.resource;
			const auto asset            = GetResourceAsset(resource);

			if (!asset) {
				continue;
			}

			const auto deviceResource   = renderSystem.GetDeviceResource(resource);
			const auto resourceState    = renderSystem.GetObjectState(packedBlock.resource);

			const auto packedBlockInfo  = ctx.GetPackedTileInfo(deviceResource);

			const auto startingLevel    = packedBlockInfo.startingLevel;
			const auto endingLevel      = packedBlockInfo.endingLevel;

			if (resourceState != DRS_Common)
			{
				incompatibleResources.push_back(resource);
				incompatibleState.push_back(resourceState);
			}

			if (resourceState != DRS_CopyDest)
				ctx.Barrier(
					deviceResource,
					DRS_Common,
					DRS_CopyDest);


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

				const TileID_t tileID   = CreateTileID( 0, 0, level );
				const auto tile         = streamContext.Read(MIPLevelInfo.WH, ctx);

				ctx.CopyTextureRegion(deviceResource, level, { 0, 0, 0 }, tile, MIPLevelInfo.WH, streamContext.Format());
			}


			ctx.Barrier(
				deviceResource,
				DeviceResourceState::DRS_CopyDest,
				DeviceResourceState::DRS_Common);

			renderSystem.SetObjectState(resource, DeviceResourceState::DRS_Common);
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

		std::optional<SyncPoint> sync;

		if (incompatibleState.size())
		{
			auto& gfxCtx = renderSystem.GetCommandList();

			for (size_t I = 0; I < incompatibleState.size(); I++)
			{
				auto resource   = incompatibleResources[I];
				auto state      = incompatibleState[I];
				gfxCtx._AddBarrier(renderSystem.GetDeviceResource(resource), state, DRS_Common);
			}

			gfxCtx.FlushBarriers();

			Vector<Context*> gfxContexts{ &threadLocalAllocator };
			gfxContexts.push_back(&gfxCtx);
			sync = renderSystem.Submit(gfxContexts);
		}

		Vector<Barrier> barriers{ &threadLocalAllocator };
		for (auto texture : updatedTextures)
		{
			Barrier b = {
				.handle         = texture,
				.type           = Barrier::ResourceType::HNDL,
				.beforeState    = DRS_Common,
				.afterState     = DRS_PixelShaderResource,
			};

			//barriers.emplace_back(b);
		}

		renderSystem.UpdateTileMappings(updatedTextures.begin(), updatedTextures.end(), &threadLocalAllocator);
		renderSystem.SubmitUploadQueues(SYNC_Graphics, &ctxHandle, 1, sync);
		//renderSystem._InsertBarrier(barriers);
	}


	/************************************************************************************************/


	Vector<gpuTileID> TextureBlockAllocator::UpdateTileStates(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator)
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
		for (const gpuTileID* itr = begin; itr < end; itr++)
		{
			if (ResourceHandle{ itr->TextureID } == InvalidHandle)
				continue;

			if (auto res = findInuse(*itr); res) {
				res->state              = TextureBlockAllocator::EBlockState::InUse;
				res->staleFrameCount    = 0;
			}
			else if (auto res = findstale(*itr); res)
			{
				auto staleTile = *res;
				stale.remove_unstable(res);

				staleTile.state             = TextureBlockAllocator::EBlockState::InUse;
				staleTile.staleFrameCount   = 0;

				inuse.push_back(staleTile);
			}
			else
				allocationsNeeded.push_back(*itr);
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


	BlockAllocation TextureBlockAllocator::AllocateBlocks(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator)
	{
		AllocatedBlockList allocatedBlocks      { allocator };
		AllocatedBlockList reallocatedBlocks    { allocator };
		AllocatedBlockList packedBlocks         { allocator };

		std::sort(inuse.begin(), inuse.end(),
			[&](const auto lhs, const auto rhs)
			{
				return !(lhs < rhs);
			});

		std::sort(stale.begin(), stale.end(),
			[&](auto lhs, auto rhs)
			{
				return lhs.staleFrameCount < rhs.staleFrameCount;
			});

		Vector<TextureBlockAllocator::Block> usedBlocks{ allocator };

		std::for_each(begin, end,
			[&](gpuTileID tile)
			{
				if (tile.tileID.bytes == -1)
					return;

				if (ResourceHandle{ tile.TextureID } == InvalidHandle)
					return;

				if (free.size())
				{
					auto block      = free.pop_back();
					block.tileID    = tile.tileID;
					block.resource  = ResourceHandle{ tile.TextureID };
					block.state     = EBlockState::InUse;
					usedBlocks.push_back(block);

					return;
				}

				if (stale.size())
				{
					auto block  = stale.pop_back();
					AllocatedBlock reallocation{
						.tileID     = block.tileID,
						.resource   = block.resource,
						.offset     = block.blockID * 64 * KILOBYTE,
						.tileIdx    = block.blockID
					};

					reallocatedBlocks.push_back(reallocation);

					block.tileID            = tile.tileID;
					block.resource          = ResourceHandle{ tile.TextureID };
					block.state             = EBlockState::InUse;
					block.staleFrameCount   = 0;
					usedBlocks.push_back(block);

					return;
				}

				if (inuse.size())
				{
					auto block  = inuse.pop_back();

					if (block.tileID.packed() || block.tileID.GetMipLevel() >= tile.tileID.GetMipLevel())
					{
						inuse.push_back(block);
						return;
					}
					AllocatedBlock reallocation{
						.tileID     = block.tileID,
						.resource   = block.resource,
						.offset     = block.blockID * 64 * KILOBYTE,
						.tileIdx    = block.blockID
					};

					reallocatedBlocks.push_back(reallocation);

					block.tileID            = tile.tileID;
					block.resource          = ResourceHandle{ tile.TextureID };
					block.state             = EBlockState::InUse;
					block.staleFrameCount   = 0;

					usedBlocks.push_back(block);

					return;
				}
			});


		for(auto tile : usedBlocks)
		{
			 AllocatedBlock allocation{
				.tileID     = tile.tileID,
				.resource   = ResourceHandle{ tile.resource },
				.offset     = tile.blockID * 64 * KILOBYTE,
				.tileIdx    = tile.blockID
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
		return textureBlockAllocator.AllocateBlocks(begin, end, allocator);
	}


	/************************************************************************************************/


	gpuTileList TextureStreamingEngine::UpdateTileStates(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator)
	{
		return textureBlockAllocator.UpdateTileStates(begin, end, allocator);
	}


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

THE SOTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
