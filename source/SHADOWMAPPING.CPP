#pragma once
#include "ShadowMapping.h"
#include "AnimationComponents.h"
#include "AnimationRendering.h"
#include <fmt/format.h>
#include <limits>

namespace FlexKit
{   /************************************************************************************************/


	ID3D12PipelineState* ShadowMapper::CreateRowSums(RenderSystem* RS)
	{
		auto CShader = RS->LoadShader("RowSumsMain", "cs_6_0", "assets\\shaders\\SACM\\Create2DSAT.hlsl");


		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_Desc = {};
		PSO_Desc.pRootSignature	= rootSignature;
		PSO_Desc.CS				= CShader;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ShadowMapper::CreateColumnSums(RenderSystem* RS)
	{
		auto CShader = RS->LoadShader("ColumnSumsMain", "cs_6_0", "assets\\shaders\\SACM\\Create2DSAT.hlsl");


		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_Desc = {};
		PSO_Desc.pRootSignature = rootSignature;
		PSO_Desc.CS = CShader;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


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
			PSO_Desc.NumRenderTargets                       = 1;
			PSO_Desc.SampleDesc.Count                       = 1;
			PSO_Desc.SampleDesc.Quality                     = 0;
			PSO_Desc.RTVFormats[0]							= DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
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
			PSO_Desc.NumRenderTargets                       = 1;
			PSO_Desc.SampleDesc.Count                       = 1;
			PSO_Desc.SampleDesc.Quality                     = 0;
			PSO_Desc.RTVFormats[0]							= DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
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


	ShadowCubeMatrices CalculateShadowMapMatrices(const float3 pos, const float r)
	{
		static const XMMATRIX ViewOrientations[] = {
			XMMATRIX{	0,  0, -1, 0,	// +X
						0, +1,  0, 0,
					   -1,  0,  0, 0,
					    0,  0,  0, 1 },
			XMMATRIX{	0,  0, +1, 0,	// -X
						0, +1,  0, 0,
					   +1,  0,  0, 0,
					    0,  0,  0, 1 },
			XMMATRIX{  +1,  0,  0, 0,	// +Y
						0,  0, -1, 0,
					    0, -1,  0, 0,
						0,  0,  0, 1 },
			XMMATRIX{  +1,  0,  0, 0,	// -Y
						0,  0, +1, 0,
						0, +1,  0, 0,
						0,  0,  0, 1 },
			XMMATRIX{  +1,  0,  0, 0,	// +Z // forward
						0, +1,  0, 0,
						0,  0, -1, 0,
						0,  0,  0, 1 },
			XMMATRIX{  -1,  0,  0, 0,	// -Z
						0, +1,  0, 0,
						0,  0, +1, 0,
						0,  0,  0, 1 }
		};

		ShadowCubeMatrices out;

		for (size_t I = 0; I < 6; I++)
		{
			const XMMATRIX ViewI            = ViewOrientations[I] * DirectX::XMMatrixTranslationFromVector(pos);
			const XMMATRIX View             = DirectX::XMMatrixInverse(nullptr, ViewI);
			const XMMATRIX perspective      = DirectX::XMMatrixPerspectiveFovRH(DegreetoRad(90.0f), 1, 0.01f, r);
			const XMMATRIX PV               = DirectX::XMMatrixTranspose(perspective) * XMMatrixTranspose(View);

			out.PV[I]       = XMMatrixToFloat4x4(PV).Transpose();
			out.ViewI[I]    = XMMatrixToFloat4x4(ViewI);
			out.View[I]     = XMMatrixToFloat4x4(View);
		}

		return out;
	}


	/************************************************************************************************/


	ShadowMapMatrices CalculateSpotLightMatrices(const float3 pos, const Quaternion q, const float r, const float a)
	{
		const XMMATRIX viewOrientation	= DirectX::XMMatrixRotationQuaternion(q);
		const XMMATRIX viewI			= DirectX::XMMatrixTranspose(viewOrientation) * DirectX::XMMatrixTranslationFromVector(pos);
		const XMMATRIX view				= DirectX::XMMatrixInverse(nullptr, viewI);
		const XMMATRIX perspective		= DirectX::XMMatrixPerspectiveFovRH(a, 1, 0.01f, r);
		const XMMATRIX pv				= DirectX::XMMatrixTranspose(perspective) * DirectX::XMMatrixTranspose(view);

		return { XMMatrixToFloat4x4(pv).Transpose(), XMMatrixToFloat4x4(view) };
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

		return InvalidHandle;
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
		DesciptorHeapLayout heapLayout;
		heapLayout.SetParameterAsShaderUAV(0, 0, 1);	// output

		rootSignature.AllowIA = true;
		rootSignature.AllowSO = false;
		rootSignature.SetParameterAsUINT(   0, 40,  3, 0,   PIPELINE_DEST_ALL);
		rootSignature.SetParameterAsCBV(    1, 0,   0,      PIPELINE_DEST_ALL);
		rootSignature.SetParameterAsUINT(   2, 16,  1, 0,   PIPELINE_DEST_ALL);
		rootSignature.SetParameterAsCBV(    3, 2,   0,      PIPELINE_DEST_ALL);
		rootSignature.SetParameterAsDescriptorTable(4, heapLayout);

		rootSignature.Build(renderSystem, &allocator);

		renderSystem.RegisterPSOLoader(SHADOWMAPPASS,			{ &rootSignature, [&](RenderSystem* rs) { return CreateShadowMapPass(rs); }});
		renderSystem.RegisterPSOLoader(SHADOWMAPANIMATEDPASS,	{ &rootSignature, [&](RenderSystem* rs) { return CreateShadowMapAnimatedPass(rs); } });
		renderSystem.RegisterPSOLoader(BUILDROWSUMS,			{ &rootSignature, [&](RenderSystem* rs) { return CreateRowSums(rs); } });
		renderSystem.RegisterPSOLoader(BUILDCOLUMNSUMS,			{ &rootSignature, [&](RenderSystem* rs) { return CreateColumnSums(rs); } });
	}


	/************************************************************************************************/


	ShadowMapPassData& ShadowMapper::ShadowMapPass(
								FrameGraph&								frameGraph,
								const GatherVisibleLightsTask&			visibleLights,
								LightUpdate&							lightUpdate,
								UpdateTask&								cameraUpdate,
								GatherPassesTask&						passes,
								ReserveConstantBufferFunction&			reserveCB,
								ReserveVertexBufferFunction&			reserveVB,
								static_vector<AdditionalShadowMapPass>&	additional,
								const double							t,
								MemoryPoolAllocator&					shadowMapPool,
								iAllocator&								tempAllocator,
								AnimationPoseUpload*					poses)
	{
		PassDrivenResourceAllocation allocation
		{
			.getPass			=
				[&]() -> std::span<const LightHandle>
				{
					return visibleLights.GetData().lights;
				},
			.initializeResources =
				[&](std::span<const LightHandle> lights, std::span<FrameResourceHandle> frameHandles, auto& resourceCtx, iAllocator&)
				{
					ProfileFunction();
						
					constexpr float f_max = std::numeric_limits<float>::max();
					static const D3D12_CLEAR_VALUE clearValue{
						.Format = DXGI_FORMAT_R32_FLOAT,
						.Color	{ f_max, f_max, f_max, f_max }
					};

					LightComponent& lightComponent = LightComponent::GetComponent();

					for(auto [handleIdx, lightHandle] : zip(iota(0), lights))
					{
						auto& light = lightComponent[lightHandle];

						switch(light.type)
						{
							case LightType::PointLight:
							{
								auto desc		= GPUResourceDesc::CubeMapUAV({ ShadowMapSize, ShadowMapSize }, DeviceFormat::R32_FLOAT, 1, true);
								desc.clearValue = clearValue;

								
								auto [shadowMap, _] = resourceCtx.AcquireResource(frameHandles[handleIdx], desc);
#if USING(DEBUGGRAPHICS)
								auto debugName = fmt::format("ShadowMap:{}:{}", renderSystem.graphicsSubmissionCounter, light.shadowMap);
								renderSystem.SetDebugName(shadowMap, debugName.c_str());
#endif
								break;
							}
							case LightType::SpotLight:
							case LightType::SpotLightBasicShadows:
							{
								auto desc		= GPUResourceDesc::UAVTexture({ ShadowMapSize, ShadowMapSize }, DeviceFormat::R32_FLOAT, true);
								desc.clearValue = clearValue;

								auto [shadowMap, _] = resourceCtx.AcquireResource(frameHandles[handleIdx], desc);
#if USING(DEBUGGRAPHICS)
								auto debugName = fmt::format("ShadowMap:{}:{}", renderSystem.graphicsSubmissionCounter, light.shadowMap);
								renderSystem.SetDebugName(shadowMap, debugName.c_str());
#endif
							}	break;
						}
					}
				},
			.layout		= DeviceLayout::DeviceLayout_DepthStencilWrite,
			.access		= DeviceAccessState::DASDEPTHBUFFERWRITE,
			.max		= ShadowMapComponent::GetComponent().size(),
			.pool		= &shadowMapPool,
			.dependency	= visibleLights
		};

		const ResourceAllocation& acquireMaps = frameGraph.AllocateResourceSet(allocation);

		auto& shadowMapPass		= tempAllocator.allocate<ShadowMapPassData>(acquireMaps);
		auto& lights			= LightComponent::GetComponent();

		auto getPasses = 
			[&](iAllocator& allocator) -> Vector<PassData>
			{
				auto& lightList			= visibleLights.GetData().lights;

				Vector<PassData> passes{ allocator };
				passes.reserve(lightList.size());

				for (auto [idx, lightHandle] : zip(iota(0), lightList))
				{
					auto& light = lights[lightHandle];

					switch (light.type)
					{
					case LightType::PointLightNoShadows:
					case LightType::SpotLightNoShadows:
						continue;
					case LightType::PointLight:
					case LightType::SpotLight:
					case LightType::SpotLightBasicShadows:
						passes.emplace_back(
							(uint32_t)idx,
							lightHandle,
							light.shadowState);
						break;
					default:
						DebugBreak();
					}
				}

				return std::move(passes);
			};

		auto getPVS = [&](PassData& pass) -> std::span<VisibilityHandle> { return { pass.shadowMapData->visableObjects }; };

		DataDrivenMultiPassDescription<Common_Data, PassData, VisibilityHandle> passDesc =
		{
			.sharedData = {
				.reserveCB = reserveCB,
				.reserveVB = reserveVB },
			.getPVS		= getPVS,
			.getPasses	= getPasses,
		};

		auto passSetupFn = [&](FrameGraphNodeBuilder& builder, Common_Data& data)
		{
			shadowMapPass.multiPass	= builder.GetNodeHandle();
			data.depthBuffer		= builder.AcquireVirtualResource(GPUResourceDesc::DepthTarget({ ShadowMapSize, ShadowMapSize }, DeviceFormat::D32_FLOAT, 6), DASDEPTHBUFFERWRITE);

			for (const auto map : acquireMaps)
				builder.WriteTransition(map, DASDEPTHBUFFERWRITE);

			builder.AddDataDependency(lightUpdate);
			builder.AddNodeDependency(acquireMaps);
		};

		auto passDrawFN = [&, this](auto begin, auto end, std::span<VisibilityHandle> pvs, PassData& pass, Common_Data& common, FrameResources& resources, Context& ctx, iAllocator& allocator)
		{
			ProfileFunction();
			ctx.BeginEvent_DEBUG("DepthPass");

			const ResourceHandle passTarget		= resources.GetResource(acquireMaps[pass.passIdx]);
			const ResourceHandle depthBuffer	= resources.GetResource(common.depthBuffer);

			auto& light = LightComponent::GetComponent()[pass.pointLightHandle];

			if (light.type == LightType::PointLightNoShadows ||
				light.type == LightType::SpotLightNoShadows)
				return;

			if (passTarget == InvalidHandle)
				DebugBreak();

			resources.renderSystem.SetDebugName(passTarget,	"passTarget");

			if (begin == pvs.begin())
			{
				const auto initialLayout = ctx.renderSystem->GetObjectLayout(passTarget);

				constexpr float f_max = std::numeric_limits<float>::max();

				ctx.AddTextureBarrier(passTarget,
					DASCommon,			DASRenderTarget,
					initialLayout,		DeviceLayout_RenderTarget,
					Sync_PixelShader,	Sync_All);

				ctx.ClearDepthBuffer(depthBuffer, 1.0f);
				ctx.ClearRenderTarget(passTarget, f_max);
				
				ctx.AddTextureBarrier(depthBuffer,
					DASDEPTHBUFFERWRITE,			DASDEPTHBUFFERWRITE,
					DeviceLayout_DepthStencilWrite, DeviceLayout_DepthStencilWrite,
					Sync_All,						Sync_DepthStencil);

				ctx.AddTextureBarrier(passTarget,
					DASRenderTarget,				DASRenderTarget,
					DeviceLayout_RenderTarget,		DeviceLayout_RenderTarget,
					Sync_All,						Sync_RenderTarget);
			}


			ctx.SetRootSignature(rootSignature);

			switch (light.type)
			{
				case LightType::PointLight:
				{
					const DepthStencilView_Options DSV_desc{ 0, 0, depthBuffer, 6 };

					ctx.SetScissorAndViewports({ passTarget });
					ctx.SetRenderTargets2({ passTarget }, 0, DSV_desc);

					RenderPointLightShadowMap(begin, end, pass, common, passTarget, resources, ctx, allocator);
				}	break;
				case LightType::SpotLight:
				case LightType::SpotLightBasicShadows:
				{
					const DepthStencilView_Options DSV_desc{ 0, 0, depthBuffer, 1 };

					ctx.SetScissorAndViewports({ passTarget });
					ctx.SetRenderTargets2({ passTarget }, 0, DSV_desc);

					RenderSpotLightShadowMap(begin, end, pass, common, passTarget, resources, ctx, allocator);
				}	break;
			}

			ctx.EndEvent_DEBUG();

			if (end == pvs.end())
			{
				for (auto& additionalDraws : common.additional)
					additionalDraws(common.reserveCB, common.reserveVB, passTarget, light.type, resources, ctx, allocator);

				switch (light.type)
				{
				case LightType::PointLight:
					ctx.AddTextureBarrier(passTarget,
						DASRenderTarget,			DASPixelShaderResource,
						DeviceLayout_RenderTarget,	DeviceLayout_ShaderResource,
						Sync_RenderTarget,			Sync_PixelShader);
					break;
				case LightType::SpotLight:
				case LightType::SpotLightBasicShadows:
					BuildSummedAreaTable(passTarget, resources, ctx, allocator);
					break;
				}

				//ctx.renderSystem->SetObjectLayout(pass.renderTarget, DeviceLayout_ShaderResource);
			}
		};

		auto& multiPass = frameGraph.AddDataDrivenMultiPass(passDesc, passSetupFn, passDrawFN);

		return shadowMapPass;
	}


	/************************************************************************************************/


	void ShadowMapper::RenderPointLightShadowMap(ShadowMapper::Iterator_TY begin, ShadowMapper::Iterator_TY end, ShadowMapper::PassData& pass, ShadowMapper::Common_Data& common, ResourceHandle passTarget, FrameResources& resources, Context& ctx, iAllocator& allocator)
	{
		PVS						brushes			{ allocator, (size_t)std::distance(begin, end) };
		Vector<GameObject*>		animatedBrushes	{ allocator };

		auto&					lights				= LightComponent::GetComponent();
		auto&					visiblityComponent	= SceneVisibilityComponent::GetComponent();
		auto&					light				= lights[pass.pointLightHandle];
		auto&					materials			= MaterialComponent::GetComponent();
		const float3			lightPosition		= GetPositionW(light.node);

		// Gather objects
		for (auto itr = begin; itr < end; itr++)
		{
			VisibilityFields& visible = visiblityComponent[*itr];
			auto& entity = *visible.entity;

			Apply(entity,
				[&](BrushView& view)
				{
					auto passes = materials.GetPasses(view.GetMaterial());

					if (auto res = std::find_if(passes.begin(), passes.end(),
						[](auto& pass)
						{
							return pass == SHADOWMAPPASS;
						}); res != passes.end())
					{
						PushPV(entity, view.GetBrush(), brushes, lightPosition);
					}
					else if (auto res = std::find_if(passes.begin(), passes.end(),
							[](auto& pass)
							{
								return pass == SHADOWMAPANIMATEDPASS;
							}); res != passes.end())
						{
							animatedBrushes.push_back(&entity);
						}
				});
		}


		static const Quaternion Orientations[6] = {
			Quaternion{   0,  90, 0 }, // Right
			Quaternion{   0, -90, 0 }, // Left
			Quaternion{ -90,   0, 0 }, // Top
			Quaternion{  90,   0, 0 }, // Bottom
			Quaternion{   0, 180, 0 }, // Backward
			Quaternion{   0,   0, 0 }, // Forward
		};


		const float3 right	= Orientations[0] * float3(0, 0, -1);
		const float3 left	= Orientations[1] * float3(0, 0, -1);

		const float3 top	= Orientations[2] * float3(0, 0, -1);
		const float3 bottom	= Orientations[3] * float3(0, 0, -1);

		const Frustum fustrum[] =
		{
			GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, light.R, lightPosition, Orientations[0]),
			GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, light.R, lightPosition, Orientations[1]),
			GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, light.R, lightPosition, Orientations[2]),
			GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, light.R, lightPosition, Orientations[3]),
			GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, light.R, lightPosition, Orientations[4]),
			GetFrustum(1.0f, (float)pi / 2.0f, 0.1f, light.R, lightPosition, Orientations[5]),
		};

		auto shadowMapPSO	= resources.GetPipelineState(SHADOWMAPPASS);

		ctx.SetPipelineState(shadowMapPSO);
		ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

		TriMeshHandle	currentMesh		= InvalidHandle;
		size_t			currentLodIdx	= 0xffffffffffffffff;
		size_t			indexCount		= 0;
		BoundingSphere	BS;

		const float3 position	= FlexKit::GetPositionW(light.node);
		const auto matrices		= CalculateShadowMapMatrices(position, light.R);

		for(uint32_t brushIdx = 0; brushIdx < (uint32_t)brushes.size(); brushIdx++)
		{
			auto& PV			= brushes[brushIdx];
			const auto lodLevel	= PV.LODlevel;
			auto& meshes		= PV.brush->meshes;

			const size_t meshCount = meshes.size();
			for(size_t I = 0; I < meshCount; I++)
			{
				auto& mesh			= meshes[I];
				auto meshLodLevel	= lodLevel[I];

				if( currentMesh != mesh || currentLodIdx != meshLodLevel)
				{
					currentMesh = mesh;

					auto* const triMesh = GetMeshResource(currentMesh);

					currentLodIdx	= Max(meshLodLevel, triMesh->GetHighestLoadedLodIdx());
					auto& lod		= triMesh->lods[currentLodIdx];
					indexCount		= lod.GetIndexCount();

					ctx.AddIndexBuffer(triMesh, (uint32_t)currentLodIdx);
					ctx.AddVertexBuffers(triMesh,
						(uint32_t)currentLodIdx,
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
								float	maxZ;
							}tempConstants = {
									.PV		= matrices.PV[itr],
									.View	= matrices.View[itr],
									.Idx	= itr,
									.maxZ	= light.R };

							ctx.SetGraphicsConstantValue(0, 34, &tempConstants);
							ctx.DrawIndexedInstanced(indexCount);
						}
					}
				}
			}
		}

		struct PoseConstants
		{
			float4x4 M[256];
		};

		CBPushBuffer animatedConstantBuffer = common.reserveCB((AlignedSize<Brush::VConstantsLayout>() + AlignedSize<PoseConstants>()) * animatedBrushes.size());
		if(animatedBrushes.size())
		{
			auto PSOAnimated = resources.GetPipelineState(SHADOWMAPANIMATEDPASS);
			ctx.SetPipelineState(PSOAnimated);
			ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

			for (const auto& brush : animatedBrushes)
			{
				Apply(*brush,
					[&](const SceneNodeView&	nodeView,
						const BrushView&		brushView,
						const SkeletonView&		poseView)
					{
						const auto& draw		= brushView.GetBrush();
						const auto& pose		= poseView.GetPoseState();
						const auto& skeleton	= *pose.Sk;

						PoseConstants poseTemp;

						const size_t end = pose.JointCount;
						for (size_t I = 0; I < end; ++I)
							poseTemp.M[I] = skeleton.IPose[I] * pose.CurrentPose[I];

						for(auto mesh : draw.meshes)
						{
							const auto		triMesh		= GetMeshResource(mesh);
							const auto		lodLevelIdx	= triMesh->GetHighestLoadedLodIdx();
							const auto&		lodLevel	= triMesh->GetHighestLoadedLod();
							const size_t	indexCount	= triMesh->GetHighestLoadedLod().GetIndexCount();

							ctx.AddIndexBuffer(triMesh, lodLevelIdx);
							ctx.AddVertexBuffers(triMesh,
								lodLevelIdx,
								{   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
									VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
									VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
								});

							const auto poseConstants = ConstantBufferDataSet{ poseTemp, animatedConstantBuffer };

							const float4x4 WT			= nodeView.GetWT();
							const float4 brushPOS_WT	= WT * float4(triMesh->BS.xyz(), 1);

							ctx.SetGraphicsConstantValue(2, 16, WT.Transpose());
							ctx.SetGraphicsConstantBufferView(3, poseConstants);

							for (uint32_t itr = 0; itr < 6; itr++)
							{
								struct 
								{
									float4x4	PV;
									float4x4	View;
									uint32_t	Idx;
									float		maxZ;
								}tempConstants = {
										.PV		= matrices.PV[itr],
										.View	= matrices.View[itr],
										.Idx	= itr,
										.maxZ	= light.R
								};

								if (Intersects(fustrum[itr], float4{ brushPOS_WT.xyz(), triMesh->BS.w }))
								{
									ctx.SetGraphicsConstantValue(0, 40, &tempConstants);
									ctx.DrawIndexedInstanced(indexCount);
								}
							}
						}
					});
			}
		}
	}


	/************************************************************************************************/


	void ShadowMapper::RenderSpotLightShadowMap(Iterator_TY begin, Iterator_TY end, PassData& pass, ShadowMapper::Common_Data& common, ResourceHandle passTarget, FrameResources& resources, Context& ctx, iAllocator& allocator)
	{
		PVS						brushes			{ allocator, (size_t)std::distance(begin, end) };
		Vector<GameObject*>		animatedBrushes	{ allocator };

		auto&					lights				= LightComponent::GetComponent();
		auto&					visiblityComponent	= SceneVisibilityComponent::GetComponent();
		auto&					light				= lights[pass.pointLightHandle];
		auto&					materials			= MaterialComponent::GetComponent();
		const float3			lightPosition		= GetPositionW(light.node);
		const Quaternion		lightOrientation	= GetOrientation(light.node);

		// Gather objects
		for (auto itr = begin; itr < end; itr++)
		{
			VisibilityFields& visible = visiblityComponent[*itr];
			auto& entity = *visible.entity;

			Apply(entity,
				[&](BrushView& view)
				{
					auto passes = materials.GetPasses(view.GetMaterial());

					if (auto res = std::find_if(passes.begin(), passes.end(),
						[](auto& pass)
						{
							return pass == SHADOWMAPPASS;
						}); res != passes.end())
					{
						PushPV(entity, view.GetBrush(), brushes, lightPosition);
					}
					else if (auto res = std::find_if(passes.begin(), passes.end(),
							[](auto& pass)
							{
								return pass == SHADOWMAPANIMATEDPASS;
							}); res != passes.end())
						{
							animatedBrushes.push_back(&entity);
						}
				});
		}


		auto shadowMapPSO	= resources.GetPipelineState(SHADOWMAPPASS);
		auto Build2DSATPSO	= resources.GetPipelineState(SHADOWMAPPASS);

		ctx.SetPipelineState(shadowMapPSO);
		ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

		TriMeshHandle	currentMesh		= InvalidHandle;
		size_t			currentLodIdx	= 0xffffffffffffffff;
		size_t			indexCount		= 0;

		const auto [pv, v] = CalculateSpotLightMatrices(lightPosition, lightOrientation, light.R, light.outerAngle);

		for(uint32_t brushIdx = 0; brushIdx < (uint32_t)brushes.size(); brushIdx++)
		{
			auto& PV			= brushes[brushIdx];
			const auto lodLevel	= PV.LODlevel;
			auto& meshes		= PV.brush->meshes;

			const float4x4	WT		= GetWT(PV.brush->Node);
			const float3	POS		= GetPositionW(PV.brush->Node);
			const float		scale	= Max(WT[0][0], Max(WT[1][1], WT[2][2]));

			struct
			{
				float4x4 PV;
				float4x4 View;
				uint32_t Idx;
				float	maxZ;
			}tempConstants = {
					.PV		= pv,
					.View	= v,
					.Idx	= 0,
					.maxZ	= light.R };

			ctx.SetGraphicsConstantValue(2, 16, WT.Transpose());
			ctx.SetGraphicsConstantValue(0, 34, &tempConstants);

			const size_t meshCount = meshes.size();
			for(size_t I = 0; I < meshCount; I++)
			{
				auto& mesh			= meshes[I];
				auto meshLodLevel	= lodLevel[I];

				if( currentMesh != mesh || currentLodIdx != meshLodLevel)
				{
					currentMesh = mesh;

					auto* const triMesh = GetMeshResource(currentMesh);

					currentLodIdx	= Max(meshLodLevel, triMesh->GetHighestLoadedLodIdx());
					auto& lod		= triMesh->lods[currentLodIdx];
					indexCount		= lod.GetIndexCount();

					ctx.AddIndexBuffer(triMesh, (uint32_t)currentLodIdx);
					ctx.AddVertexBuffers(triMesh,
						(uint32_t)currentLodIdx,
						{ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });
				}

				ctx.DrawIndexedInstanced(indexCount);
			}
		}

		struct PoseConstants
		{
			float4x4 M[256];
		};

		CBPushBuffer animatedConstantBuffer = common.reserveCB((AlignedSize<Brush::VConstantsLayout>() + AlignedSize<PoseConstants>()) * animatedBrushes.size());
		if(animatedBrushes.size())
		{
			auto PSOAnimated = resources.GetPipelineState(SHADOWMAPANIMATEDPASS);
			ctx.SetPipelineState(PSOAnimated);
			ctx.SetScissorAndViewports({ passTarget });
			ctx.SetRenderTargets({}, true, passTarget);
			ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

			for (const auto& brush : animatedBrushes)
			{
				Apply(*brush,
					[&](const SceneNodeView&	nodeView,
						const BrushView&		brushView,
						const SkeletonView&		poseView)
					{
						const auto& draw		= brushView.GetBrush();
						const auto& pose		= poseView.GetPoseState();
						const auto& skeleton	= *pose.Sk;

						PoseConstants poseTemp;

						const size_t end = pose.JointCount;
						for (size_t I = 0; I < end; ++I)
							poseTemp.M[I] = skeleton.IPose[I] * pose.CurrentPose[I];

						const float4x4 WT			= nodeView.GetWT();

						struct 
						{
							float4x4	PV;
							float4x4	View;
							uint32_t	Idx;
							float		maxZ;
						}tempConstants = {
								.PV		= pv,
								.View	= v,
								.Idx	= 0,
								.maxZ	= light.R
						};

						ctx.SetGraphicsConstantValue(2, 16, WT.Transpose());
						ctx.SetGraphicsConstantValue(0, 40, &tempConstants);
						ctx.SetGraphicsConstantBufferView(3, ConstantBufferDataSet{ poseTemp, animatedConstantBuffer });

						for(auto mesh : draw.meshes)
						{
							const auto		triMesh		= GetMeshResource(mesh);
							const auto		lodLevelIdx	= triMesh->GetHighestLoadedLodIdx();
							const auto&		lodLevel	= triMesh->GetHighestLoadedLod();
							const size_t	indexCount	= triMesh->GetHighestLoadedLod().GetIndexCount();

							ctx.AddIndexBuffer(triMesh, lodLevelIdx);
							ctx.AddVertexBuffers(triMesh,
								lodLevelIdx,
								{   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
									VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
									VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
								});

							ctx.DrawIndexedInstanced(indexCount);
						}
					});
			}
		}
	}


	/************************************************************************************************/


	void ShadowMapper::BuildSummedAreaTable(ResourceHandle target, FrameResources& resources, Context& ctx, iAllocator& allocator)
	{
#if 0 
		auto rowPSO		= resources.GetPipelineState(BUILDROWSUMS);
		auto columnPSO	= resources.GetPipelineState(BUILDCOLUMNSUMS);

		ctx.SetComputeRootSignature(rootSignature);


		ctx.AddTextureBarrier(target,
			DASRenderTarget,			DASUAV,
			DeviceLayout_RenderTarget,	DeviceLayout_UnorderedAccess,
			Sync_RenderTarget,			Sync_Compute);

		DescriptorHeap heap{ ctx, rootSignature.GetDescHeap(0), allocator };
		heap.SetUAVTexture(ctx, 0, target);

		ctx.SetComputeDescriptorTable(4, heap);
		ctx.Dispatch(rowPSO, { 1, 1024, 1});

		ctx.AddUAVBarrier(target, -1, DeviceLayout_UnorderedAccess);

		ctx.Dispatch(columnPSO, { 1024, 1, 1 });

		ctx.AddTextureBarrier(target,
			DASUAV,							DASPixelShaderResource,
			DeviceLayout_UnorderedAccess,	DeviceLayout_ShaderResource,
			Sync_Compute,					Sync_PixelShader);
#else
	ctx.AddTextureBarrier(target,
		DASRenderTarget,			DASPixelShaderResource,
		DeviceLayout_RenderTarget,	DeviceLayout_ShaderResource,
		Sync_RenderTarget,			Sync_PixelShader);

#endif
	}


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
