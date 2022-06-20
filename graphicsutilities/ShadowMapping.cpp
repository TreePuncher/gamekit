#pragma once
#include "ShadowMapping.h"
#include "AnimationComponents.h"
#include "AnimationRendering.h"
#include <fmt/format.h>

namespace FlexKit
{   /************************************************************************************************/


	ID3D12PipelineState* ShadowMapper::CreateShadowMapPass(RenderSystem* RS)
	{
        auto VShader = RS->LoadShader("VS_Main", "vs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");
        auto PShader = RS->LoadShader("PS_Main", "ps_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");

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
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode = D3D12_FILL_MODE_SOLID;
		Rast_Desc.CullMode = D3D12_CULL_MODE_FRONT;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                         = rootSignature;
			PSO_Desc.VS                                     = VShader;
			PSO_Desc.PS                                     = PShader;
			PSO_Desc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask                             = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets                       = 0;
			PSO_Desc.SampleDesc.Count                       = 1;
			PSO_Desc.SampleDesc.Quality                     = 0;
			PSO_Desc.DSVFormat                              = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                            = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState                      = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
			PSO_Desc.RasterizerState                        = Rast_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


    /************************************************************************************************/


	ID3D12PipelineState* ShadowMapper::CreateShadowMapAnimatedPass(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VS_Skinned_Main", "vs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");
        auto PShader = RS->LoadShader("PS_Main", "ps_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");

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
                { "POSITION",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode = D3D12_FILL_MODE_SOLID;
		Rast_Desc.CullMode = D3D12_CULL_MODE_NONE;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                         = rootSignature;
			PSO_Desc.VS                                     = VShader;
			PSO_Desc.PS                                     = PShader;
			PSO_Desc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask                             = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets                       = 0;
			PSO_Desc.SampleDesc.Count                       = 1;
			PSO_Desc.SampleDesc.Quality                     = 0;
			PSO_Desc.DSVFormat                              = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                            = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState                      = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
			PSO_Desc.RasterizerState                        = Rast_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ShadowPassMatrices CalculateShadowMapMatrices(const float3 pos, const float r, const float T)
	{
		static const XMMATRIX ViewOrientations[] = {
            DirectX::XMMatrixRotationY((float)-pi / 2.0f),  // Right Face
            DirectX::XMMatrixRotationY((float) pi / 2.0f),  // Left Face,
            DirectX::XMMatrixRotationX((float) pi / 2.0f),  // Bottom
            DirectX::XMMatrixRotationX((float)-pi / 2.0f),  // Top
            DirectX::XMMatrixRotationY((float) pi),         // Backward
            DirectX::XMMatrixIdentity(),                    // Foward
		};

		ShadowPassMatrices out;

		for (size_t I = 0; I < 6; I++)
		{
			const XMMATRIX ViewI            = ViewOrientations[I] * DirectX::XMMatrixTranslationFromVector(pos);
			const XMMATRIX View             = DirectX::XMMatrixInverse(nullptr, ViewI);
			const XMMATRIX perspective      = DirectX::XMMatrixPerspectiveFovRH(float(pi/2), 1, 0.01f, r);
			const XMMATRIX PV               = DirectX::XMMatrixTranspose(perspective) * XMMatrixTranspose(View);

			out.PV[I]       = XMMatrixToFloat4x4(PV).Transpose();
            out.ViewI[I]    = XMMatrixToFloat4x4(ViewI);
            out.View[I]     = XMMatrixToFloat4x4(View);
		}

		return out;
	}


	/************************************************************************************************/


    float cot(float t)
    {
        return 1.0f / std::tanf(t);
    }

    float ScreenSpaceSize(const Camera c, const BoundingSphere bs)
    {
        const auto position = GetPositionW(c.Node);
        const auto d        = (position - bs.xyz()).magnitude();
        const auto r        = bs.r;
        const auto fovy     = c.FOV / c.AspectRatio;
        const auto pr       = cot(fovy / 2) * r / sqrt(d*d - r*r);

        return (float)pi * pr * pr;
    }


    /************************************************************************************************/


    ResourceHandle ShadowMapper::GetResource(const size_t frameID)
    {
        if (auto res = std::find_if(resourcePool.begin(), resourcePool.end(), [&](auto& res) { return frameID > (res.availibility); }); res != resourcePool.end())
        {
            auto resource = res->resource;
            resourcePool.remove_unstable(res);
            return { resource };
        }

        return InvalidHandle_t;
    }


    /************************************************************************************************/


    void ShadowMapper::AddResource(ResourceHandle shadowMap, const size_t frameID)
    {
        resourcePool.emplace_back(shadowMap, frameID);
    }


    /************************************************************************************************/

    ShadowMapper::ShadowMapper(RenderSystem& renderSystem, iAllocator& allocator) :
        rootSignature   { &allocator },
        resourcePool    { &allocator }
    {
        rootSignature.AllowIA = true;
        rootSignature.AllowSO = false;
        rootSignature.SetParameterAsUINT(   0, 40,  3, 0,   PIPELINE_DEST_ALL);
        rootSignature.SetParameterAsCBV(    1, 0,   0,      PIPELINE_DEST_ALL);
        rootSignature.SetParameterAsUINT(   2, 16,  1, 0,   PIPELINE_DEST_ALL);
        rootSignature.SetParameterAsCBV(    3, 2,   0,      PIPELINE_DEST_ALL);
        rootSignature.Build(renderSystem, &allocator);

        renderSystem.RegisterPSOLoader(SHADOWMAPPASS,          { &renderSystem.Library.RS6CBVs4SRVs, [&](RenderSystem* rs) { return CreateShadowMapPass(rs); }});
        renderSystem.RegisterPSOLoader(SHADOWMAPANIMATEDPASS,  { &renderSystem.Library.RS6CBVs4SRVs, [&](RenderSystem* rs) { return CreateShadowMapAnimatedPass(rs); } });
    }


    AcquireShadowMapTask& ShadowMapper::AcquireShadowMaps(UpdateDispatcher& dispatcher, RenderSystem& renderSystem, MemoryPoolAllocator& RTPool, PointLightUpdate& pointLightUpdate)
    {
        return dispatcher.Add<AcquireShadowMapResources>(
            [&](UpdateDispatcher::UpdateBuilder& builder, AcquireShadowMapResources& data)
            {
                ProfileFunction();

                builder.AddInput(pointLightUpdate);
            },
            [&dirtyList = pointLightUpdate.GetData().dirtyList, &renderSystem = renderSystem, &shadowMapAllocator = RTPool, this]
            (AcquireShadowMapResources& data, iAllocator& threadLocalAllocator)
            {
                ProfileFunction();

                size_t idx = 0;

                auto& lights        = PointLightComponent::GetComponent();
                auto itr            = dirtyList.begin();
                const auto end      = dirtyList.end();
                const auto frameID  = renderSystem.GetCurrentFrame();

                for (;itr != end; itr++)
                {
                    auto lightHandle    = *itr;
                    auto& light         = lights[lightHandle];

                    if (light.shadowMap != InvalidHandle_t)
                    {
                        AddResource(light.shadowMap, frameID);
                        light.shadowMap = InvalidHandle_t;
                    }

                    if (auto res = GetResource(frameID); res != InvalidHandle_t)
                    {
                        light.shadowMap = res;
                    }
                    else
                        break;
                }

                for (auto i = itr; i != end; i++)
                {
                    auto lightHandle    = *i;
                    auto& light         = lights[lightHandle];

                    if (light.shadowMap != InvalidHandle_t)
                    {
                        AddResource(light.shadowMap, frameID);
                        light.shadowMap == InvalidHandle_t;
                    }
                }


                Parallel_For(
                    renderSystem.threads, threadLocalAllocator, itr, end, 1,
                    [&](auto lightHandle, iAllocator& threadLocalAllocator)
                    {
                        ProfileFunction();

                        auto& light         = lights[lightHandle];
                        auto [shadowMap, _] = shadowMapAllocator.Acquire(GPUResourceDesc::DepthTarget({ 256, 256 }, DeviceFormat::D32_FLOAT, 6), false);
                        light.shadowMap     = shadowMap;

#if USING(DEBUGGRAPHICS)
                        auto debugName = fmt::format("ShadowMap:{}:{}", renderSystem.GetCurrentFrame(), light.shadowMap);
                        renderSystem.SetDebugName(shadowMap, debugName.c_str());
#endif
                    });
            });
    }


    /************************************************************************************************/


	ShadowMapPassData& ShadowMapper::ShadowMapPass(
		FrameGraph&                             frameGraph,
        const PointLightShadowGatherTask&       shadowMaps,
        UpdateTask&                             cameraUpdate,
        GatherPassesTask&                       gather,
        UpdateTask&                             shadowMapAcquire,
        ReserveConstantBufferFunction&          reserveCB,
        ReserveVertexBufferFunction&            reserveVB,
        static_vector<AdditionalShadowMapPass>& additional,
		const double                            t,
		iAllocator*                             allocator,
        AnimationPoseUpload*                    poses)
	{
		auto& shadowMapPass = allocator->allocate<ShadowMapPassData>(ShadowMapPassData{ allocator->allocate<Vector<TemporaryFrameResourceHandle>>(allocator) });

        constexpr size_t workerThreadCount = 3;
        for(size_t I = 0; I < workerThreadCount; I++)
		    frameGraph.AddNode<LocalShadowMapPassData>(
				    LocalShadowMapPassData{
                        I,
                        workerThreadCount,
                        shadowMaps.GetData().pointLightShadows,
					    shadowMapPass,
					    reserveCB,
                        reserveVB,
                        additional
				    },
				    [&](FrameGraphNodeBuilder& builder, LocalShadowMapPassData& data)
				    {
                        builder.AddDataDependency(cameraUpdate);
                        builder.AddDataDependency(gather);
                        builder.AddDataDependency(shadowMapAcquire);
				    },
				    [this, &gather, t = t](LocalShadowMapPassData& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
				    {
                        ProfileFunction();

                        auto shadowMapPSO = resources.GetPipelineState(SHADOWMAPPASS);
                        ctx.SetRootSignature(rootSignature);

                        auto& visableLights = data.pointLightShadows;
                        auto& pointLights  = PointLightComponent::GetComponent();
                    
                        Vector<ResourceHandle>          shadowMaps      { &allocator, visableLights.size() };
                        Vector<ConstantBufferDataSet>   passConstant2   { &allocator, visableLights.size() };

                        ctx.BeginEvent_DEBUG("Shadow Mapping");

                        const uint32_t workCount  = (uint32_t)(visableLights.size() / data.threadTotal) + (visableLights.size() % (uint32_t)data.threadTotal != 0 ? 1 : 0);
                        const uint32_t begin      = (uint32_t)(workCount * (uint32_t)data.threadIdx);
                        const uint32_t end        = Min(begin + workCount, (uint32_t)visableLights.size());

                        for (uint32_t I = begin; I < end; I++)
                        {
                            ProfileFunction();

                            ctx.BeginEvent_DEBUG("Shadow Map Pass");

                            auto& lightHandle   = visableLights[I];
                            auto& pointLight    = pointLights[lightHandle];

                            if (pointLight.state == LightStateFlags::Dirty && pointLight.shadowMap != InvalidHandle_t)
                            {
                                pointLight.state = LightStateFlags::Clean;

                                const auto& visibilityComponent = SceneVisibilityComponent::GetComponent();
                                const auto& visables            = pointLight.shadowState->visableObjects;
					            const auto depthTarget          = pointLight.shadowMap;
                                const float3 pointLightPosition = GetPositionW(pointLight.Position);

					            if (!visables.size())
						            return;

                                static const Quaternion Orientations[6] = {
                                    Quaternion{   0,  90, 0 }, // Right
                                    Quaternion{   0, -90, 0 }, // Left
                                    Quaternion{ -90,   0, 0 }, // Top
                                    Quaternion{  90,   0, 0 }, // Bottom
                                    Quaternion{   0, 180, 0 }, // Backward
                                    Quaternion{   0,   0, 0 }, // Forward
                                };

                                const Frustum fustrum[] =
                                {
                                    GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, pointLight.R, pointLightPosition, Orientations[0]),
                                    GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, pointLight.R, pointLightPosition, Orientations[1]),
                                    GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, pointLight.R, pointLightPosition, Orientations[2]),
                                    GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, pointLight.R, pointLightPosition, Orientations[3]),
                                    GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, pointLight.R, pointLightPosition, Orientations[4]),
                                    GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, pointLight.R, pointLightPosition, Orientations[5]),
                                };

                                PVS                 brushes         { &allocator, visables.size() };
                                Vector<GameObject*> animatedBrushes { &allocator, visables.size() };

                                auto& materials = MaterialComponent::GetComponent();

                                for (auto& visable : visables)
                                {
                                    ProfileFunctionLabeled(VISIBILITY);

                                    auto entity = visibilityComponent[visable].entity;

                                    Apply(*entity,
                                        [&](BrushView& view)
                                        {
                                            auto passes = materials.GetPasses(view.GetMaterial());

                                            if (auto res = std::find_if(passes.begin(), passes.end(),
                                                [](auto& pass)
                                                {
                                                    return pass == SHADOWMAPPASS;
                                                }); res != passes.end())
                                            {
                                                PushPV(*entity, view.GetBrush(), brushes, pointLightPosition);
                                            }

                                            if (auto res = std::find_if(passes.begin(), passes.end(),
                                                [](auto& pass)
                                                {
                                                    return pass == SHADOWMAPANIMATEDPASS;
                                                }); res != passes.end())
                                            {
                                                animatedBrushes.push_back(entity);
                                            }
                                        });
                                }

                                const float3 Position   = FlexKit::GetPositionW(pointLight.Position);
                                const auto matrices     = CalculateShadowMapMatrices(Position, pointLight.R, (float)t);

                                struct PoseConstants
                                {
                                    float4x4 M[256];
                                };


                                if (auto state = resources.renderSystem().GetObjectState(depthTarget); state != DRS_DEPTHBUFFERWRITE)
                                    ctx.AddResourceBarrier(depthTarget, state, DRS_DEPTHBUFFERWRITE);


                                CBPushBuffer animatedConstantBuffer = data.reserveCB((AlignedSize<Brush::VConstantsLayout>() + AlignedSize<PoseConstants>()) * animatedBrushes.size());

                                const DepthStencilView_Options DSV_desc = { 0, 0, depthTarget };

                                ctx.DiscardResource(depthTarget);
                                ctx.ClearDepthBuffer(depthTarget, 1.0f);

                                ctx.SetPipelineState(shadowMapPSO);
                                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
                                ctx.SetScissorAndViewports({ depthTarget });
                                ctx.SetRenderTargets2({}, 0, DSV_desc);

                                TriMeshHandle   currentMesh     = InvalidHandle_t;
                                size_t          currentLodIdx   = 0xffffffffffffffff;
                                size_t          indexCount      = 0;
                                BoundingSphere  BS;


					            for(uint32_t brushIdx = 0; brushIdx < brushes.size(); brushIdx++)
					            {
                                    auto& PV            = brushes[brushIdx];
                                    const auto lodLevel = PV.LODlevel;

                                    if( currentMesh     != PV.brush->MeshHandle ||
                                        currentLodIdx   != lodLevel)
                                    {
                                        currentMesh     = PV.brush->MeshHandle;

						                auto* const triMesh = GetMeshResource(currentMesh);

                                        currentLodIdx       = Max(lodLevel, triMesh->GetHighestLoadedLodIdx());
                                        auto& lod           = triMesh->lods[currentLodIdx];
                                        indexCount          = lod.GetIndexCount();

                                        ctx.AddIndexBuffer(triMesh, currentLodIdx);
                                        ctx.AddVertexBuffers(triMesh,
                                            currentLodIdx,
                                            { VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });

                                        BS = triMesh->BS;
                                    }


                                    const float4x4 WT   = GetWT(PV.brush->Node);
                                    const float3 POS    = GetPositionW(PV.brush->Node);
                                    const float4 POS_WT = POS + (WT * float4(BS.xyz(), 0)).xyz();
                                    const float scale   = Max(WT[0][0], Max(WT[1][1], WT[2][2]));

                                    auto brushBoundingSphere = BoundingSphere{ POS_WT.xyz(), BS.w * scale };
                                    bool intersections[] =
                                    {
                                        Intersects(fustrum[0], brushBoundingSphere), // Left
                                        Intersects(fustrum[1], brushBoundingSphere), // Right
                                        Intersects(fustrum[2], brushBoundingSphere), // Top
                                        Intersects(fustrum[3], brushBoundingSphere), // Bottom
                                        Intersects(fustrum[4], brushBoundingSphere), // Forward
                                        Intersects(fustrum[5], brushBoundingSphere), // Backward
                                    };

                                    if (intersections[0] || intersections[1] || intersections[2] ||
                                        intersections[3] || intersections[4] || intersections[5])
                                    {

                                        ctx.SetGraphicsConstantValue(2, 16, WT.Transpose());

                                        for (uint32_t itr = 0; itr < 6; itr++)
                                        {
                                            if (intersections[itr])
                                            {
                                                struct
                                                {
                                                    float4x4 PV;
                                                    float4x4 View;
                                                    uint32_t Idx;
                                                    float    maxZ;
                                                }tempConstants = {
                                                        .PV     = matrices.PV[itr],
                                                        .View   = matrices.View[itr],
                                                        .Idx    = itr,
                                                        .maxZ   = pointLight.R };

                                                ctx.SetGraphicsConstantValue(0, 34, &tempConstants);
                                                ctx.DrawIndexedInstanced(indexCount);
                                            }
                                        }
                                    }
					            }

                                if(animatedBrushes.size())
                                {
                                    auto PSOAnimated = resources.GetPipelineState(SHADOWMAPANIMATEDPASS);
                                    ctx.SetPipelineState(PSOAnimated);
                                    ctx.SetScissorAndViewports({ depthTarget });
                                    ctx.SetRenderTargets2({}, 0, DSV_desc);
                                    ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

                                    for (auto& brush : animatedBrushes)
                                    {
                                        Apply(*brush,
                                            [&](SceneNodeView<>&    nodeView,
                                                BrushView&          brushView,
                                                SkeletonView&       poseView)
                                            {
                                                auto& draw      = brushView.GetBrush();
                                                auto& pose      = poseView.GetPoseState();
                                                auto& skeleton  = *pose.Sk;

                                                PoseConstants poseTemp;

                                                const size_t end = pose.JointCount;
                                                for (size_t I = 0; I < end; ++I)
                                                    poseTemp.M[I] = skeleton.IPose[I] * pose.CurrentPose[I];

                                                const auto      triMesh         = GetMeshResource(draw.MeshHandle);
                                                const auto      lodLevelIdx     = triMesh->GetHighestLoadedLodIdx();
                                                const auto&     lodLevel        = triMesh->GetHighestLoadedLod();
                                                const size_t    indexCount      = triMesh->GetHighestLoadedLod().GetIndexCount();

                                                ctx.AddIndexBuffer(triMesh, lodLevelIdx);
						                        ctx.AddVertexBuffers(triMesh,
                                                    lodLevelIdx,
							                        {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                                        VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
                                                        VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
                                                    });

                                                const auto poseConstants = ConstantBufferDataSet{ poseTemp, animatedConstantBuffer };

                                                const float4x4 WT           = nodeView.GetWT();
                                                const float4 brushPOS_WT    = WT * float4(triMesh->BS.xyz(), 1);

                                                ctx.SetGraphicsConstantValue(2, 16, WT.Transpose());
                                                ctx.SetGraphicsConstantBufferView(3, poseConstants);

                                                for (uint32_t itr = 0; itr < 6; itr++)
                                                {
                                                    struct 
                                                    {
                                                        float4x4    PV;
                                                        float4x4    View;
                                                        uint32_t    Idx;
                                                        float       maxZ;
                                                    }tempConstants = {
                                                            .PV     = matrices.PV[itr],
                                                            .View   = matrices.View[itr],
                                                            .Idx    = itr,
                                                            .maxZ   = pointLight.R
                                                    };

                                                    if (FlexKit::Intersects(fustrum[itr], float4{ brushPOS_WT.xyz(), triMesh->BS.w }))
                                                    {
                                                        ctx.SetGraphicsConstantValue(0, 40, &tempConstants);
                                                        ctx.DrawIndexedInstanced(indexCount);
                                                    }
                                                }
                                            });
                                    }
                                }

                                shadowMaps.push_back(depthTarget);
                            }

                            ctx.EndEvent_DEBUG();
                        }

                        if(shadowMaps.size())
                        {
                            ProfileFunctionLabeled(Barriers);

                            for (auto& additionalPass : data.additionalShadowPass)
                            {
                                ctx.BeginEvent_DEBUG("Additional Shadow Map Pass");

                                additionalPass(
                                    data.reserveCB,
                                    data.reserveVB,
                                    passConstant2.data(),
                                    shadowMaps.data(),
                                    (uint32_t)shadowMaps.size(),
                                    resources,
                                    ctx,
                                    allocator);

                                ctx.EndEvent_DEBUG();
                            }

                            for (const auto target : shadowMaps) {
                                ctx.AddResourceBarrier(target, DRS_DEPTHBUFFERWRITE, DRS_PixelShaderResource);
                                ctx.renderSystem->SetObjectState(target, DRS_PixelShaderResource);
                            }
                        }

                        ctx.EndEvent_DEBUG();
				    });

		return shadowMapPass;
	}


}   /************************************************************************************************/
