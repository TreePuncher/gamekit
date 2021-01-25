#include "TextureStreamingUtilities.h"
#include "ProfilingUtilities.h"
#include "Graphics.h"

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
        allocator   { IN_allocator },
        asset       { IN_asset },
        mipLevel    { IN_mipLevel }
    {
        TextureResourceBlob resource;

        FK_ASSERT(ReadAsset(readCtx, asset, &resource, sizeof(resource), 0) == RAC_OK);

        format          = resource.format;
        WH              = resource.WH;
        mipLevelOffset  = resource.mipOffsets[mipLevel] + sizeof(resource);

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
        const auto reservation    = ctx.Reserve(64 * KILOBYTE);
        const auto blockSize      = BlockSize(format);
        const auto blocksX        = 256 / 4;
        const auto localRowPitch  = blocksX * blockSize;
        const auto blocksY        = GetTileByteSize() / localRowPitch;

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
        const auto blockSize                = BlockSize(format);
        const size_t destinationRowPitch    = Max(blockSize * WH[0]/4, 256);
        const size_t rowCount               = Max(WH[1] / 4, 1);
        const size_t columnCount            = Max(WH[0] / 4, 1);
        const size_t blockCount             = rowCount * columnCount;
        const size_t allocationSize         = rowCount * destinationRowPitch;
        const auto   reservation            = ctx.Reserve(allocationSize, 512);
        const size_t sourceRowPitch         = blockSize * columnCount;

        /*
        for (size_t row = 0; row < rowCount; row++)
            ReadAsset(
                readCtx,
                asset,
                reservation.buffer + row * destinationRowPitch,
                columnCount * blockSize,
                mipLevelOffset + (row * sourceRowPitch));
        */
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
        blockTable      { IN_allocator },
        allocator       { IN_allocator }
    {
        blockTable.reserve(blockCount);

        for (size_t I = 0; I < blockCount; ++I)
            blockTable.emplace_back(
                Block
                {
                    (uint32_t)-1,
                    InvalidHandle_t,
                    0, 0, 0, 0, // state, subresourceCount, subresourceStart, padding
                    (uint32_t)I
                });
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateTextureFeedbackPassPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("Forward_VS", "vs_6_0", "assets\\shaders\\forwardRender.hlsl");
        auto PShader = RS->LoadShader("TextureFeedback_PS",   "ps_6_0", "assets\\shaders\\TextureFeedback.hlsl");

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // enable conservative rast

        if(RS->features.conservativeRast == RenderSystem::AvailableFeatures::ConservativeRast_AVAILABLE)
            Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthEnable      = true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = { 0 };
        PSO_Desc.pRootSignature         = RS->Library.RSDefault;
        PSO_Desc.VS                     = VShader;
        PSO_Desc.PS                     = PShader;
        PSO_Desc.BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        PSO_Desc.SampleMask             = UINT_MAX;
        PSO_Desc.RasterizerState        = Rast_Desc;
        PSO_Desc.DepthStencilState      = Depth_Desc;
        PSO_Desc.InputLayout            = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
        PSO_Desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PSO_Desc.NumRenderTargets       = 0;
        PSO_Desc.DSVFormat              = DXGI_FORMAT_D32_FLOAT;
        PSO_Desc.SampleDesc             = { 1, 0 };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateTextureFeedbackCompressorPSO(RenderSystem* RS)
    {
        auto computeShader = RS->LoadShader(
            "CompressBlocks", "cs_6_0",
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
            "PreFixSumBlockSizes", "cs_6_0",
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
            "MergeBlocks", "cs_6_0",
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
            "SetBlockCounters", "cs_6_0",
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
			allocator		        { IN_allocator		},
            textureBlockAllocator   { desc.textureCacheSize / desc.blockSize,   IN_allocator },
			renderSystem	        { IN_renderSystem	},
			settings		        { desc				},
            feedbackReturnBuffer    { IN_renderSystem.CreateReadBackBuffer(2 * MEGABYTE) },
            heap                    { IN_renderSystem.CreateHeap(desc.textureCacheSize, 0) },
            mappedAssets            { IN_allocator },
            timeStats               { IN_renderSystem.CreateTimeStampQuery(512) }
    {
        renderSystem.RegisterPSOLoader(
            TEXTUREFEEDBACKPASS,
            { &renderSystem.Library.RSDefault, CreateTextureFeedbackPassPSO });

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
            UpdateDispatcher&                   dispatcher,
            FrameGraph&                         frameGraph,
            CameraHandle                        camera,
            uint2                               renderTargetWH,
            UpdateTaskTyped<GetPVSTaskData>&    sceneGather,
            ReserveConstantBufferFunction&      reserveCB,
            ReserveVertexBufferFunction&        reserveVB)
    {
        if (updateInProgress)
            return;

        updateInProgress = true;

        frameGraph.AddNode<TextureFeedbackPass_Data>(
            TextureFeedbackPass_Data{
                camera,
                sceneGather,
                reserveCB,
            },
            [&](FrameGraphNodeBuilder& builder, TextureFeedbackPass_Data& data)
            {
                builder.AddDataDependency(sceneGather);

                data.feedbackBuffers[0]         = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4 * MEGABYTE), DRS_ShaderResource);
                data.feedbackBuffers[1]         = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(4 * MEGABYTE), DRS_ShaderResource);
                data.feedbackCounters           = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 256), DRS_Write);
                data.feedbackBlockSizes         = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 256), DRS_ShaderResource);
                data.feedbackBlockOffsets       = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(sizeof(uint32_t) * 256), DRS_ShaderResource);
                data.feedbackDepth              = builder.AcquireVirtualResource(GPUResourceDesc::DepthTarget({ 128, 128 }, DeviceFormat::D32_FLOAT), DRS_DEPTHBUFFERWRITE);

                data.readbackBuffer             = feedbackReturnBuffer;

                builder.SetDebugName(data.feedbackBuffers[0], "feedbackBuffer");
                builder.SetDebugName(data.feedbackBuffers[1], "feedbackBufferCompressed");
                builder.SetDebugName(data.feedbackCounters, "feedbackCounters");
                builder.SetDebugName(data.feedbackBlockSizes, "feedbackBlockSizes");
                builder.SetDebugName(data.feedbackBlockOffsets, "feedbackBlockOffsets");
                builder.SetDebugName(data.feedbackDepth, "feedbackDepth");
            },
            [=](TextureFeedbackPass_Data& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                auto& drawables = data.pvs.GetData().solid;
                auto& materials = MaterialComponent::GetComponent();

                if (!drawables.size()) {
                    updateInProgress = false;
                    return;
                }

                ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);

                ctx.SetPrimitiveTopology(EIT_TRIANGLELIST);
                ctx.ClearDepthBuffer(resources.GetRenderTarget(data.feedbackDepth), 1.0f);

                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.feedbackDepth) });
                ctx.SetRenderTargets({}, true, resources.GetRenderTarget(data.feedbackDepth));

                struct alignas(512) constantBufferLayout
                {
                    float       bias;
                    float       padding[3];
                    uint32_t    zeroBlock[256];
                } constants =
                {
                    std::log2f(128.0f / renderTargetWH[0]),
                    { 0.0f }
                };


                memset(constants.zeroBlock, 0, sizeof(constants.zeroBlock));

                size_t subDrawCount = 0;
                for (auto& drawable : drawables)
                {
                    auto* triMesh = GetMeshResource(drawable.D->MeshHandle);

                    subDrawCount += triMesh->subMeshes.size();
                }

                const size_t bufferSize =
                    AlignedSize<Drawable::VConstantsLayout>() * subDrawCount +
                    AlignedSize<constantBufferLayout>() +
                    AlignedSize<Camera::ConstantBuffer>();

                CBPushBuffer passConstantBuffer  { data.reserveCB(bufferSize) };

                const auto passConstants    = ConstantBufferDataSet{ constants, passConstantBuffer };
                const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), passConstantBuffer };

                ctx.CopyBufferRegion(
                    { resources.GetObjectResource(passConstants.Handle()) },
                    { passConstants.Offset() + ((size_t)&constants.zeroBlock - (size_t)&constants) },
                    { resources.GetObjectResource(resources.CopyToUAV(data.feedbackCounters, ctx)) },
                    { 0 },
                    { sizeof(constants.zeroBlock) },
                    { resources.GetObjectState(data.feedbackCounters) },
                    { DRS_Write }
                );

                DescriptorHeap uavHeap;
                uavHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);
                uavHeap.SetUAVStructured(ctx, 0, resources.WriteUAV(data.feedbackBuffers[0], ctx), resources.WriteUAV(data.feedbackCounters, ctx), sizeof(uint2), 0);

                ctx.SetGraphicsDescriptorTable(4, uavHeap);
                ctx.SetGraphicsConstantBufferView(0, cameraConstants);
                ctx.SetGraphicsConstantBufferView(2, passConstants);

                TriMesh* prevMesh = nullptr;
                MaterialHandle prevMaterial = InvalidHandle_t;

                ctx.TimeStamp(timeStats, 0);
                ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACKPASS));


                size_t element = 0;
                for (size_t J = element; J < drawables.size(); J++)
                {
                    auto& visable = drawables[J];
                    auto* triMesh = GetMeshResource(visable.D->MeshHandle);

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(
                            triMesh,
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

                    const auto materialHandle = visable.D->material;


                    if (!materials[materialHandle].SubMaterials.empty())
                    {
                        const auto subMeshCount = triMesh->subMeshes.size();
                        auto material = MaterialComponent::GetComponent()[visable.D->material];

                        for (size_t I = 0; I < subMeshCount; I++)
                        {
                            auto& subMesh       = triMesh->subMeshes[I];
                            const auto passMaterial  = materials[material.SubMaterials[I]];

                            DescriptorHeap srvHeap;

                            const auto& textures = passMaterial.Textures;

                            srvHeap.Init(
                                ctx,
                                resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
                                &allocator);

                            auto constantData = visable.D->GetConstants();
                            constantData.textureCount = textures.size();

                            for (size_t I = 0; I < textures.size(); I++) {
                                srvHeap.SetSRV(ctx, I, textures[I]);
                                constantData.textureHandles[I] = uint4{ 256, 256, textures[I].to_uint() };
                            }

                            srvHeap.NullFill(ctx);

                            const auto constants = ConstantBufferDataSet{ constantData, passConstantBuffer };

                            ctx.SetGraphicsConstantBufferView(1, constants);
                            ctx.SetGraphicsDescriptorTable(3, srvHeap);
                            ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
                        }
                    }
                    else
                    {
                        if (materialHandle != prevMaterial)
                        {
                            const auto& textures = materials[materialHandle].Textures;

                            DescriptorHeap srvHeap;
                            srvHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), textures.size(), &allocator);
                            ctx.SetGraphicsDescriptorTable(3, srvHeap);

                            for (size_t I = 0; I < textures.size(); I++)
                                srvHeap.SetSRV(ctx, I, textures[I]);
                        }

                        const auto& textures = materials[materialHandle].Textures;

                        prevMaterial = materialHandle;

                        const auto constants = ConstantBufferDataSet{ visable.D->GetConstants(), passConstantBuffer };

                        ctx.SetGraphicsConstantBufferView(1, constants);
                        ctx.DrawIndexed(triMesh->IndexCount);
                    }

                    ctx.AddUAVBarrier(resources.GetResource(data.feedbackBuffers[0]));
                    ctx.AddUAVBarrier(resources.GetResource(data.feedbackCounters));
                }

                ctx.TimeStamp(timeStats, 1);

                auto CompressionPass = [&](auto Source, auto Destination)
                {
                    ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);

                    DescriptorHeap uavCompressHeap;
                    uavCompressHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 3, &allocator);
                    uavCompressHeap.SetUAVBuffer(ctx, 0, resources.WriteUAV(data.feedbackCounters, ctx));
                    uavCompressHeap.SetUAVStructured(ctx, 1, resources.GetResource(Source), sizeof(uint32_t[2]));
                    uavCompressHeap.SetUAVBuffer(ctx, 2, resources.ReadWriteUAV(data.feedbackBlockSizes, ctx));

                    const uint32_t blockCount = (MEGABYTE * 2) / (1024 * sizeof(uint32_t[2]));

                    ctx.AddUAVBarrier(resources.GetResource(Source));
                    ctx.AddUAVBarrier(resources.GetResource(data.feedbackCounters));

                    ctx.SetComputeDescriptorTable(4, uavCompressHeap);
                    ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKCOMPRESSOR), { blockCount, 1, 1 });

                    ctx.AddUAVBarrier(resources.GetResource(Destination));
                    ctx.AddUAVBarrier(resources.GetResource(data.feedbackBlockSizes));

                    // Prefix Sum
                    DescriptorHeap srvPreFixSumHeap;
                    srvPreFixSumHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 1, &allocator);
                    srvPreFixSumHeap.SetStructuredResource(ctx, 0, resources.ReadUAV(data.feedbackBlockSizes, resources.GetObjectState(data.feedbackBlockSizes), ctx), sizeof(uint32_t), 0);
                    srvPreFixSumHeap.NullFill(ctx, 1);

                    DescriptorHeap uavPreFixSumHeap;
                    uavPreFixSumHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);
                    uavPreFixSumHeap.SetUAVStructured(ctx, 0, resources.ReadWriteUAV(data.feedbackBlockOffsets, ctx), sizeof(uint32_t), 0);
                    uavPreFixSumHeap.NullFill(ctx, 1);

                    ctx.SetComputeDescriptorTable(3, srvPreFixSumHeap);
                    ctx.SetComputeDescriptorTable(4, uavPreFixSumHeap);
                    ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKPREFIXSUMBLOCKSIZES), { 1 , 1, 1 });

                    //  Merge partial blocks
                    DescriptorHeap srvMergeHeap;
                    srvMergeHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 3, &allocator);
                    srvMergeHeap.SetStructuredResource(ctx, 0, resources.ReadUAV(Source, DRS_ShaderResource, ctx), 8);
                    srvMergeHeap.SetStructuredResource(ctx, 1, resources.ReadUAV(data.feedbackBlockSizes, DRS_ShaderResource, ctx), sizeof(uint32_t), 0);
                    srvMergeHeap.SetStructuredResource(ctx, 2, resources.ReadUAV(data.feedbackBlockOffsets, DRS_ShaderResource, ctx), sizeof(uint32_t), 0);

                    DescriptorHeap uavMergeHeap;
                    uavMergeHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 2, &allocator);
                    uavMergeHeap.SetUAVBuffer(ctx, 0, resources.ReadWriteUAV(Destination, ctx));
                    uavMergeHeap.SetUAVBuffer(ctx, 1, resources.ReadWriteUAV(data.feedbackCounters, ctx));

                    ctx.SetComputeDescriptorTable(3, srvMergeHeap);
                    ctx.SetComputeDescriptorTable(4, uavMergeHeap);

                    ctx.AddUAVBarrier(resources.GetResource(Destination));
                    ctx.AddUAVBarrier(resources.GetResource(data.feedbackBlockOffsets));

                    ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKMERGEBLOCKS), { blockCount , 1, 1 });

                    ctx.AddUAVBarrier(resources.GetResource(Destination));
                    
                    ctx.Dispatch(resources.GetPipelineState(TEXTUREFEEDBACKSETBLOCKSIZES), { 1, 1, 1 });

                    ctx.AddUAVBarrier(resources.GetResource(Destination));
                    ctx.AddUAVBarrier(resources.GetResource(data.feedbackBlockSizes));
                };

                const size_t passCount = 3;
                for (size_t itr = 0; itr < passCount; itr++) {
                    CompressionPass(data.feedbackBuffers[itr % 2], data.feedbackBuffers[(itr + 1) % 2]);
                }
                // Write out
                ctx.CopyBufferRegion(
                    {   resources.GetObjectResource(resources.ReadUAV(data.feedbackCounters, DRS_Read, ctx)) ,
                        resources.GetObjectResource(resources.ReadUAV(data.feedbackBuffers[passCount % 2], DRS_Read, ctx)) },
                    { 0, 0 },
                    {   resources.GetObjectResource(data.readbackBuffer),
                        resources.GetObjectResource(data.readbackBuffer) },
                    { 0, 64 },
                    { 64, 2048 * sizeof(uint2) },
                    { DRS_Write, DRS_Write },
                    { DRS_Write, DRS_Write });
                ctx.ResolveQuery(timeStats, 0, 4, resources.GetObjectResource(data.readbackBuffer), 8);

                ctx.QueueReadBack(data.readbackBuffer);
            });
    }


    /************************************************************************************************/


    TextureStreamingEngine::TextureStreamUpdate::TextureStreamUpdate(
        ReadBackResourceHandle      IN_resource,
        TextureStreamingEngine&     IN_textureStreamEngine,
        iAllocator*                 IN_allocator) :
            iWork               { IN_allocator },
            textureStreamEngine { IN_textureStreamEngine },
            allocator           { IN_allocator  },
            resource            { IN_resource   } {}


    /************************************************************************************************/


    void TextureStreamingEngine::TextureStreamUpdate::Run(iAllocator& threadLocalAllocator)
    {
        uint32_t    requestCount = 0;
        gpuTileID*  requests = nullptr;

        auto [buffer, bufferSize] = textureStreamEngine.renderSystem.OpenReadBackBuffer(resource);
        EXITSCOPE(textureStreamEngine.renderSystem.CloseReadBackBuffer(resource));

        std::chrono::high_resolution_clock Clock;
        auto Before = Clock.now();

        EXITSCOPE(
                auto After      = Clock.now();
                auto Duration   = std::chrono::duration_cast<std::chrono::microseconds>(After - Before);

                textureStreamEngine.updateTime = float(Duration.count()) / 1000.0f; );
        if (!buffer) {
            return;
        }

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
            const auto requestSize = requestCount * sizeof(uint2) - 2;
            requests = reinterpret_cast<gpuTileID*>(threadLocalAllocator.malloc(requestSize));
            memcpy(requests, (std::byte*)buffer + 64, requestSize);
        }

        if (!requests || !requestCount)
            return;


        const auto lg_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool
        {
            /*
            if (lhs.IsPacked() == rhs.IsPacked())
                return (lhs.GetSortingID() < rhs.GetSortingID());
            else if (lhs.tileID.GetMipLevel() == lhs.tileID.GetMipLevel())
                return (lhs.GetSortingID() < rhs.GetSortingID());
            else
                return (lhs.tileID.GetMipLevel() > lhs.tileID.GetMipLevel());
            */
            return lhs.GetSortingID() < rhs.GetSortingID();
        };

        const auto eq_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool
        {
            return lhs.GetSortingID() == rhs.GetSortingID();
        };

        FK_LOG_9("Request Size: %u / %u", requestCount, (bufferSize) / sizeof(uint2));

        std::sort(
            requests,
            requests + requestCount,
            lg_comparitor);

        const auto end            = std::unique(requests, requests + requestCount, eq_comparitor);
        const auto uniqueCount    = (size_t)(end - requests);

        const auto stateUpdateRes     = textureStreamEngine.UpdateTileStates(requests, requests + uniqueCount, &threadLocalAllocator);
        const auto blockAllocations   = textureStreamEngine.AllocateTiles(stateUpdateRes.begin(), stateUpdateRes.end());

        textureStreamEngine.PostUpdatedTiles(blockAllocations, *allocator);
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

        Vector<ResourceHandle>  updatedTextures = { &threadLocalAllocator  };
        ResourceHandle          prevResource    = InvalidHandle_t;
        TextureStreamContext    streamContext   = { &threadLocalAllocator };
        uint2                   blockSize       = { 256, 256 };
        TileMapList             mappings        = { &threadLocalAllocator };

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
                    block.tileID,
                    InvalidHandle_t,
                    TileMapState::Null,
                    block.offset,
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

            if (resourceState != DeviceResourceState::DRS_Write)
                ctx.Barrier(
                    deviceResource,
                    resourceState,
                    DeviceResourceState::DRS_Write);

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

            TileMapList mappings{ allocator };
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
                DeviceResourceState::DRS_Write,
                DeviceResourceState::DRS_GENERIC);

            renderSystem.SetObjectState(resource, DeviceResourceState::DRS_GENERIC);
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
                __debugbreak();
                continue;
            }

            const auto deviceResource   = renderSystem.GetDeviceResource(resource);
            const auto resourceState    = renderSystem.GetObjectState(packedBlock.resource);

            const auto packedBlockInfo  = ctx.GetPackedTileInfo(deviceResource);

            const auto startingLevel    = packedBlockInfo.startingLevel;
            const auto endingLevel      = packedBlockInfo.endingLevel;

            if (resourceState != DeviceResourceState::DRS_Write)
                ctx.Barrier(
                    deviceResource,
                    resourceState,
                    DeviceResourceState::DRS_Write);


            TileMapList mappings{ allocator };

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
                DeviceResourceState::DRS_Write,
                DeviceResourceState::DRS_GENERIC);

            renderSystem.SetObjectState(resource, DeviceResourceState::DRS_GENERIC);
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

        renderSystem.UpdateTileMappings(updatedTextures.begin(), updatedTextures.end(), allocator);
        renderSystem.SubmitUploadQueues(SYNC_Graphics, &ctxHandle);
    }


    /************************************************************************************************/


    Vector<gpuTileID> TextureBlockAllocator::UpdateTileStates(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator)
    {
        auto pred = [](const auto& lhs, const auto& rhs)
        {
            return (lhs.resource << 32 |  lhs.tileID) <  (size_t(rhs.resource) << 32 | rhs.tileID);
        };

        std::sort(blockTable.begin(), blockTable.end(), pred);

        for (auto& blockState : blockTable)
            if(blockState.state != EBlockState::Free) blockState.state = EBlockState::Stale;

        auto I                  = begin;
        size_t stateIterator    = 0;

        Vector<gpuTileID> allocationsNeeded{ allocator };

        std::for_each(begin, end,
            [&](const gpuTileID tile)
            {
                if (!tile.tileID.valid())
                    return;

                auto res =
                    std::lower_bound(
                        blockTable.begin(), blockTable.end(), tile,
                        [&](auto lhs, auto rhs)
                        {
                            return lhs < rhs;
                        });

                if (res != std::end(blockTable) && res->tileID == tile.tileID && res->resource == tile.TextureID)
                    res->state = EBlockState::InUse;
                else
                    allocationsNeeded.push_back(tile);
            });

        return allocationsNeeded;
    }


    /************************************************************************************************/


    BlockAllocation TextureBlockAllocator::AllocateBlocks(const gpuTileID* begin, const gpuTileID* end, iAllocator* allocator)
    {
        AllocatedBlockList allocatedBlocks      { allocator };
        AllocatedBlockList reallocatedBlocks    { allocator };
        AllocatedBlockList packedBlocks         { allocator };

        uint32_t            itr         = 0;
        const auto          blockCount  = blockTable.size();
        const gpuTileID*    block_itr   = begin;
        int allocatedBlockCount         = 0;

        while (block_itr < end && allocatedBlockCount < 128)
        {
            while (itr < blockCount)
            {
                const auto idx          = (last + itr) % blockCount;

                if (blockTable[idx].state != EBlockState::InUse &&
                    blockTable[idx].tileID.GetMipLevel() > block_itr->tileID.GetMipLevel())
                {
                    if (blockTable[idx].resource != InvalidHandle_t &&
                        (blockTable[idx].state == EBlockState::Stale ||
                         blockTable[idx].tileID.GetMipLevel() > block_itr->tileID.GetMipLevel()))
                    {
                        reallocatedBlocks.push_back({
                            blockTable[idx].tileID,
                            blockTable[idx].resource,
                            blockTable[idx].blockID * 64 * KILOBYTE,
                            blockTable[idx].blockID
                            });
                    }

                    const auto resourceHandle = ResourceHandle{ block_itr->TextureID };
                    const auto tileID         = block_itr->tileID;

                    blockTable[idx].resource = resourceHandle;
                    blockTable[idx].tileID   = tileID;
                    blockTable[idx].state    = EBlockState::InUse;
                    blockTable[idx].packed   = block_itr->IsPacked();

                    AllocatedBlock block = {
                            tileID,
                            resourceHandle,
                            blockTable[idx].blockID * 64 * KILOBYTE,
                            blockTable[idx].blockID };

                    if (block_itr->IsPacked())
                        packedBlocks.push_back(block);
                    else
                        allocatedBlocks.push_back(block);

                    allocatedBlockCount++;

                    break;
                }

                ++itr;
            }

            if (itr > blockCount)
            {
                last = 0;
                return {
                    std::move(reallocatedBlocks),
                    std::move(allocatedBlocks),
                    std::move(packedBlocks) }; /// Hrmm, we ran out of free blocks
            }

            block_itr++;
        }

        last = (last + itr + 1) % blockTable.size();

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
