#include "TextureStreamingUtilities.h"
#include "Graphics.h"

namespace FlexKit
{   /************************************************************************************************/


    CRNDecompressor::CRNDecompressor(const uint32_t mipLevel, AssetHandle asset, iAllocator* IN_allocator) : allocator{ IN_allocator }
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


    /************************************************************************************************/


    CRNDecompressor::~CRNDecompressor()
    {
        allocator->free(buffer);
    }


    /************************************************************************************************/


    UploadReservation CRNDecompressor::ReadTile(const TileID_t id, const uint2 TileSize, CopyContext& ctx)
    {
        auto reservation    = ctx.Reserve(64 * KILOBYTE);
        auto blocksX        = TileSize[0] / 4;
        auto blocksY        = TileSize[1] / 4;
        auto localRowPitch  = blockSize * blocksX;

        const auto tileX = id.GetTileX();
        const auto tileY = id.GetTileY();

        for (size_t row = 0; row < blocksY; ++row)
            memcpy(
                reservation.buffer + row * localRowPitch,
                buffer + row * rowPitch + tileX * localRowPitch + rowPitch * blocksY * tileY,
                localRowPitch);

        return reservation;
    }


    /************************************************************************************************/


    std::optional<CRNDecompressor*> CreateCRNDecompressor(const uint32_t MIPlevel, AssetHandle asset, iAllocator* allocator)
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


    TextureBlockAllocator::TextureBlockAllocator(const size_t blockCount, iAllocator* IN_allocator) :
        blockTable      { IN_allocator },
        allocator       { IN_allocator }
    {
        blockTable.reserve(blockCount);

        for (size_t I = 0; I < blockCount; ++I)
            blockTable.emplace_back(Block{ (uint32_t)-1, InvalidHandle_t, 0, (uint32_t)I });
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateTextureFeedbackPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("Forward_VS",         "Forward_VS",           "vs_5_0", "assets\\shaders\\forwardRender.hlsl");
        auto PShader = LoadShader("TextureFeedback_PS", "TextureFeedback_PS",   "ps_5_0", "assets\\shaders\\TextureFeedback.hlsl");

        FINALLY
            
        Release(&VShader);
        Release(&PShader);

        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // enable conservative rast
        Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Disable depth 
        Depth_Desc.DepthEnable      = true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RSDefault;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 0;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_UNKNOWN;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }


        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    template<typename TY>
    size_t GetConstantsAlignedSize()
    {
        const auto unalignedSize    = sizeof(TY);
        const auto offset           = unalignedSize & (256 + 1);
        const auto adjustedOffset   = offset != 0 ? 256 - offset : 0;

        return unalignedSize + adjustedOffset;
    }

    void TextureStreamingEngine::TextureFeedbackPass(
            UpdateDispatcher&                   dispatcher,
            FrameGraph&                         frameGraph,
            CameraHandle                        camera,
            UpdateTaskTyped<GetPVSTaskData>&    sceneGather,
            ResourceHandle                      testTexture,
            ReserveConstantBufferFunction&      constantBufferAllocator)
    {
        if (updateInProgress || counter++ < 2)
            return;

        counter = 0;

        updateInProgress = true;

        frameGraph.Resources.AddUAVResource(feedbackBuffer, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackBuffer));
        frameGraph.Resources.AddUAVResource(feedbackCounters, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackCounters));
        frameGraph.Resources.AddDepthBuffer(feedbackDepth, 0, frameGraph.Resources.renderSystem.GetObjectState(feedbackDepth));

        frameGraph.AddNode<TextureFeedbackPass_Data>(
            TextureFeedbackPass_Data{
                camera,
                sceneGather,
                constantBufferAllocator
            },
            [&](FrameGraphNodeBuilder& nodeBuilder, TextureFeedbackPass_Data& data)
            {
                nodeBuilder.AddDataDependency(sceneGather);

                data.feedbackBuffer     = nodeBuilder.ReadWriteUAV(feedbackBuffer);
                data.feedbackCounters   = nodeBuilder.ReadWriteUAV(feedbackCounters);
                data.feedbackDepth      = nodeBuilder.WriteDepthBuffer(feedbackDepth);
            },
            [=](TextureFeedbackPass_Data& data, FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                const size_t entityBufferSize = GetConstantsAlignedSize<Drawable::VConstantsLayout>();

                CBPushBuffer entityConstantBuffer   { data.constantBufferAllocator(entityBufferSize * data.pvs.GetData().solid.size() * 2) };
                CBPushBuffer passContantBuffer      { data.constantBufferAllocator(4096) };

                ctx.SetRootSignature(resources.renderSystem.Library.RSDefault);
                ctx.SetPipelineState(resources.GetPipelineState(TEXTUREFEEDBACK));


                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.feedbackTarget) });
                ctx.SetRenderTargets({}, true, resources.GetRenderTarget(data.feedbackDepth));

                struct constantBufferLayout
                {
                    uint32_t    textureHandles[16];
                    uint2       blockSizes[16];

                    uint32_t    zeroBlock[8];
                } constants =
                {
                    { (uint32_t)testTexture },
                    { { 256, 256 }}
                };

                auto passConstants = ConstantBufferDataSet{ constants, passContantBuffer };

                ctx.CopyBufferRegion(
                    { resources.GetObjectResource(passConstants.Handle()) },
                    { passConstants.Offset() + ((size_t)&constants.zeroBlock - (size_t)&constants) },
                    { resources.GetObjectResource(resources.CopyToUAV(data.feedbackCounters, ctx)) },
                    { 0 },
                    { 8 },
                    { resources.GetObjectState(data.feedbackCounters) },
                    { DRS_Write }
                );

                DescriptorHeap uavHeap;
                uavHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(1), 4, &allocator);
                uavHeap.SetUAV(ctx, 0, resources.WriteUAV(data.feedbackCounters, &ctx));
                uavHeap.SetUAV(ctx, 1, resources.GetUAVBufferResource(data.feedbackBuffer));

                DescriptorHeap srvHeap;
                srvHeap.Init2(ctx, resources.renderSystem.Library.RSDefault.GetDescHeap(0), 4, &allocator);
                srvHeap.SetSRV(ctx, 0, testTexture);

                ctx.SetGraphicsDescriptorTable(3, srvHeap);
                ctx.SetGraphicsDescriptorTable(4, uavHeap);

                ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ GetCameraConstants(camera), passContantBuffer });
                ctx.SetGraphicsConstantBufferView(2, passConstants);

                TriMesh* prevMesh = nullptr;

                for (size_t I = 0; I < data.pvs.GetData().solid.size(); ++I)
                {
                    auto& visable = data.pvs.GetData().solid[I];
                    auto* triMesh = GetMeshResource(visable.D->MeshHandle);

                    visable.D->GetConstants();

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

                    ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ visable.D->GetConstants(), entityConstantBuffer });
                    ctx.DrawIndexed(triMesh->IndexCount);
                }


                ctx.CopyBufferRegion(
                    {   resources.GetObjectResource(resources.ReadUAVBuffer(data.feedbackCounters, DRS_Read, &ctx)) ,
                        resources.GetObjectResource(resources.ReadUAVBuffer(data.feedbackBuffer, DRS_Read, &ctx)) },
                    { 0, 0 },
                    {   resources.GetObjectResource(feedbackReturnBuffer[readBackBlock]),
                        resources.GetObjectResource(feedbackReturnBuffer[readBackBlock]) },
                    { 0, 64 },
                    { 8, MEGABYTE / 2 },
                    { DRS_Write, DRS_Write },
                    { DRS_Write, DRS_Write }
                );


                resources.renderSystem.LockReadBack(
                    feedbackReturnBuffer[readBackBlock],
                    resources.renderSystem.GetCurrentFrame() + 2);
            });
    }


    /************************************************************************************************/


    TextureStreamingEngine::TextureBlockUpdate::TextureBlockUpdate(TextureStreamingEngine& IN_textureStreamEngine, char* buffer, iAllocator* IN_allocator) :
            iWork               { IN_allocator              },
            allocator           { IN_allocator              },
            textureStreamEngine { IN_textureStreamEngine    }
        {
            memcpy(&requestCount, buffer, sizeof(uint64_t));

            if (requestCount)
            {
                requests = reinterpret_cast<gpuTileID*>(IN_allocator->malloc(requestCount));

                memcpy(requests, buffer + 64, requestCount * 8);
            }
        }


    /************************************************************************************************/


    void TextureStreamingEngine::TextureBlockUpdate::Run()
    {
        if (!requests || !requestCount)
            return;

        auto lg_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool
        {
            return lhs.GetSortingID() < rhs.GetSortingID();
        };

        auto eq_comparitor = [](const gpuTileID lhs, const gpuTileID rhs) -> bool
        {
            return lhs.GetSortingID() == rhs.GetSortingID();
        };

        std::sort(
            requests,
            requests + requestCount,
            lg_comparitor);

        const auto end            = std::unique(requests, requests + requestCount, eq_comparitor);
        const auto uniqueCount    = (size_t)(end - requests);

        auto stateUpdateRes     = textureStreamEngine.UpdateTileStates(requests, requests + uniqueCount, allocator);
        auto blockAllocations   = textureStreamEngine.AllocateTiles(stateUpdateRes.begin(), stateUpdateRes.end());

        allocator->free(requests);
        textureStreamEngine.PostUpdatedTiles(blockAllocations);
    }


    /************************************************************************************************/


    void TextureStreamingEngine::BindAsset(const AssetHandle textureAsset, const ResourceHandle  resource)
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

        if (res == std::end(mappedAssets) || res->resource != resource)
            mappedAssets.insert(res, MappedAsset{ resource, textureAsset });
        else
            res->textureAsset = textureAsset;
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

        return (res != std::end(mappedAssets) && res->GetResourceID() == resource) ?
            std::optional<AssetHandle>{ res->textureAsset } :
            std::optional<AssetHandle>{};
    }


    /************************************************************************************************/


    void TextureStreamingEngine::PostUpdatedTiles(const BlockAllocation& allocations)
    {
        auto ctxHandle      = renderSystem.OpenUploadQueue();
        auto& ctx           = renderSystem._GetCopyContext(ctxHandle);
        auto uploadQueue    = renderSystem._GetCopyQueue();
        auto deviceHeap     = renderSystem.GetDeviceResource(heap);

        Vector<ResourceHandle>  updatedTextures = { allocator };
        ResourceHandle          prevResource    = InvalidHandle_t;
        TextureStreamContext    streamContext   = { allocator };
        uint2                   blockSize       = { 256, 256 };
        TileMapList             mappings{ allocator };

        for (const AllocatedBlock& block : allocations.allocations)
        {
            if (prevResource != block.resource)
            {
                if (prevResource != InvalidHandle_t)
                {
                    auto currentState = renderSystem.GetObjectState(block.resource);
                    if(currentState != DeviceResourceState::DRS_GENERIC)
                        ctx.Barrier(
                            renderSystem.GetDeviceResource(block.resource),
                            currentState,
                            DeviceResourceState::DRS_GENERIC);

                    renderSystem.UpdateTextureTileMappings(block.resource, mappings);
                    mappings.clear();
                }

                if (auto res = GetResourceAsset(block.resource); res)
                {
                    if (!streamContext.Open(block.tileID.GetMip(), res.value()))
                        continue;

                    prevResource    = block.resource;
                    blockSize       = streamContext.GetBlockSize();

                    updatedTextures.push_back(block.resource);
                }
                else 
                    continue;
            }

            const auto deviceResource = renderSystem.GetDeviceResource(block.resource);
            const auto resourceState  = renderSystem.GetObjectState(block.resource);

            if (resourceState != DeviceResourceState::DRS_Write)
                ctx.Barrier(
                    deviceResource,
                    resourceState,
                    DeviceResourceState::DRS_Write);

            renderSystem.SetObjectState(block.resource, DRS_Write);

            mappings.push_back(
                TileMapping{
                    .tileID     = block.tileID,
                    .heap       = heap,
                    .state      = TileMapState::Updated,
                    .heapOffset = block.tileIdx
                });

            const auto tile = streamContext.ReadTile(block.tileID, blockSize, ctx);

            ctx.CopyTile(
                deviceResource,
                block.tileID,
                block.tileIdx,
                tile);

            renderSystem._UpdateTileMapping(
                block.tileID,
                blockSize,
                deviceResource,
                deviceHeap,
                block.tileIdx);
        }

        if (allocations.allocations.size() && mappings.size() != 0) {
            const ResourceHandle resource = allocations.allocations.back().resource;
            renderSystem.UpdateTextureTileMappings(
                resource,
                mappings);
        }

        mappings.clear();

        renderSystem.UpdateTileMappings(updatedTextures.begin(), updatedTextures.end(), allocator);
        renderSystem.SubmitUploadQueues(&ctxHandle);
        //updateInProgress    = false;
        readBackBlock       = ++readBackBlock % 3;
        counter             = 0;
    }


    /************************************************************************************************/


    Vector<gpuTileID> TextureBlockAllocator::UpdateTileStates(gpuTileID* begin, gpuTileID* end, iAllocator* allocator)
    {
        auto pred = [](const auto& lhs, const auto& rhs)
        {
            return lhs.tileID < rhs.tileID;
        };

        std::sort(blockTable.begin(), blockTable.end(), pred);

        for (auto& blockState : blockTable)
            if(blockState.state != EBlockState::Free) blockState.state = EBlockState::Stale;

        auto I                  = begin;
        size_t stateIterator    = 0;

        Vector<gpuTileID>   allocationsNeeded{ allocator };

        // Update states of loaded tiles
        while(I < end)
        {
            if (blockTable[stateIterator].resource == InvalidHandle_t)
            {
                for (; I < end; I++)
                    allocationsNeeded.push_back(*I);

                return allocationsNeeded;
            }

            while (I->TextureID != blockTable[stateIterator].resource) stateIterator++;
            while (I->tileID > blockTable[stateIterator].tileID)
            {
                auto I_TileID       = I->tileID;
                auto blockTileID    = blockTable[stateIterator].tileID;

                stateIterator++;
            }

            if (I->tileID == blockTable[stateIterator].tileID)
                blockTable[stateIterator].state = EBlockState::InUse;
            else
                allocationsNeeded.push_back(*I);
            I++;
        }

        return allocationsNeeded;
    }


    /************************************************************************************************/


    BlockAllocation TextureBlockAllocator::AllocateBlocks(gpuTileID* begin, gpuTileID* end, iAllocator* allocator)
    {
        AllocatedBlockList allocatedBlocks  { allocator };
        AllocatedBlockList reallocatedBlocks{ allocator };

        allocatedBlocks.reserve(blockTable.size());
        reallocatedBlocks.reserve(blockTable.size());

        uint32_t    itr        = 0;
        const auto  blockCount = blockTable.size();
        gpuTileID*  block_itr  = begin;

        while (block_itr < end)
        {
            while (itr < blockCount)
            {
                const auto idx = itr++;
                if (blockTable[idx + last].state != EBlockState::InUse)
                {
                    const auto resourceHandle = ResourceHandle{ block_itr->TextureID };
                    const auto tileID         = block_itr->tileID;

                    blockTable[idx].resource    = resourceHandle;
                    blockTable[idx].tileID      = tileID;

                    blockTable[idx + last].state = EBlockState::InUse;

                    allocatedBlocks.push_back({
                        .tileID     = tileID,
                        .resource   = resourceHandle ,
                        .offset     = blockTable[idx].blockID * 64 * KILOBYTE,
                        .tileIdx    = blockTable[idx].blockID });

                    break;
                }
            }

            if (itr > blockCount)
            {
                last = 0;
                return {}; /// Hrmm, we ran out of free blocks
            }

            block_itr++;
        }

        last = itr % blockTable.size();

        return {
            std::move(reallocatedBlocks),
            std::move(allocatedBlocks) };
    }


    /************************************************************************************************/


    BlockAllocation TextureStreamingEngine::AllocateTiles(gpuTileID* begin, gpuTileID* end)
    {
        return textureBlockAllocator.AllocateBlocks(begin, end, allocator);
    }


    /************************************************************************************************/


    gpuTileList TextureStreamingEngine::UpdateTileStates(gpuTileID* begin, gpuTileID* end, iAllocator* allocator)
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

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
