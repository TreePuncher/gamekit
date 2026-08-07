#pragma once
#include "ShadowMapping.hpp"
#include "AnimationComponents.hpp"
#include "AnimationRendering.hpp"
#include "CameraComponent.hpp"
#include "TriMeshResource.hpp"

#include <fmt/format.h>
#include <limits>
#include <ranges>

namespace FlexKit
{   /************************************************************************************************/
	using std::views::iota;
	using std::views::zip;


	LoadPipelineStateRes ShadowMapper::CreateRowSums(IRenderSystem& irs, iAllocator&)
	{
#if 0
		auto& RS = static_cast<RenderSystem&>(irs);
		auto CShader = RS.LoadShader("RowSumsMain", "cs_6_0", "assets\\shaders\\SACM\\Create2DSAT.hlsl");


		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_Desc = {};
		PSO_Desc.pRootSignature	= *rootSignature;
		PSO_Desc.CS				= Shader2ByteCode(CShader);

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS.pDevice->CreateComputePipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, rootSignature };
#endif
		return {};
	}


	/************************************************************************************************/


	LoadPipelineStateRes ShadowMapper::CreateColumnSums(IRenderSystem& irs, iAllocator&)
	{
#if 0
		auto& RS = static_cast<RenderSystem&>(irs);
		auto CShader = RS.LoadShader("ColumnSumsMain", "cs_6_0", "assets\\shaders\\SACM\\Create2DSAT.hlsl");


		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_Desc = {};
		PSO_Desc.pRootSignature	= *rootSignature;
		PSO_Desc.CS				= Shader2ByteCode(CShader);

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS.pDevice->CreateComputePipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, rootSignature };
#endif
		return {};
	}


	/************************************************************************************************/


	LoadPipelineStateRes ShadowMapper::CreateShadowMapPass(IRenderSystem& irs, iAllocator&)
	{
#if 0
		auto& RS = static_cast<RenderSystem&>(irs);
		auto VShader = RS.LoadShader("VS_Main", "vs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");
		auto PShader = RS.LoadShader("PS_Main", "ps_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");

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
			PSO_Desc.pRootSignature                         = *rootSignature;
			PSO_Desc.VS                                     = Shader2ByteCode(VShader);
			PSO_Desc.PS                                     = Shader2ByteCode(PShader);
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
		auto HR = RS.pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "ShadowMapPass");

		return { PSO, rootSignature };
#endif
		return {};
	}


	/************************************************************************************************/


	LoadPipelineStateRes ShadowMapper::CreateShadowMapAnimatedPass(IRenderSystem& irs, iAllocator&)
	{
#if 0
		auto& RS = static_cast<RenderSystem&>(irs);
		auto VShader = RS.LoadShader("VS_Skinned_Main", "vs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");
		auto PShader = RS.LoadShader("PS_Main", "ps_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");

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
		Rast_Desc.CullMode = D3D12_CULL_MODE_FRONT;


		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                         = *rootSignature;
			PSO_Desc.VS                                     = Shader2ByteCode(VShader);
			PSO_Desc.PS                                     = Shader2ByteCode(PShader);
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
		auto HR = RS.pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "ShadowMapAnimatedPass");

		return { PSO, rootSignature };
#endif
		return {};
	}


	/************************************************************************************************/


	ShadowCubeMatrices CalculateShadowMapMatrices(const float3 pos, const float r)
	{
		static const float4x4 ViewOrientations[] = {
			float4x4{	0,  0, -1, 0,	// +X
						0, +1,  0, 0,
					   -1,  0,  0, 0,
					    0,  0,  0, 1 },
			float4x4{	0,  0, +1, 0,	// -X
						0, +1,  0, 0,
					   +1,  0,  0, 0,
					    0,  0,  0, 1 },
			float4x4{  +1,  0,  0, 0,	// +Y
						0,  0, -1, 0,
					    0, -1,  0, 0,
						0,  0,  0, 1 },
			float4x4{  +1,  0,  0, 0,	// -Y
						0,  0, +1, 0,
						0, +1,  0, 0,
						0,  0,  0, 1 },
			float4x4{  +1,  0,  0, 0,	// +Z // forward
						0, +1,  0, 0,
						0,  0, -1, 0,
						0,  0,  0, 1 },
			float4x4{  -1,  0,  0, 0,	// -Z
						0, +1,  0, 0,
						0,  0, +1, 0,
						0,  0,  0, 1 }
		};

		ShadowCubeMatrices out;

		for (size_t I = 0; I < 6; I++)
		{
			const float4x4 ViewI            = ViewOrientations[I] * TranslationMatrix(pos);
			const float4x4 View             = ViewOrientations[I].Transpose() * TranslationMatrix(-pos);
			const float4x4 perspective      = PerspectiveRH(DegreetoRad(90.0f), 0.01f, r, 1.0f);
			const float4x4 PV               = perspective * View;

			out.PV[I]       = PV;
			out.ViewI[I]    = ViewI;
			out.View[I]     = View;
		}

		return out;
	}


	/************************************************************************************************/


	ShadowMapMatrices CalculateSpotLightMatrices(const float3 pos, const Quaternion q, const float r, const float a)
	{
		const float4x4 viewOrientation	= Quaternion2Matrix(q);
		const float4x4 viewTranslation	= TranslationMatrix(pos);
		const float4x4 viewI			= viewOrientation * viewTranslation;
		const float4x4 view				= viewOrientation.Transpose() * TranslationMatrix(-pos);
		const float4x4 perspective		= PerspectiveRH(a, 0.01f, r, 1.0f);
		const float4x4 pv				= perspective * view;

		return { pv, view };
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

	ShadowMapper::ShadowMapper(IRenderSystem& renderSystem, iAllocator& allocator) :
		resourcePool    { &allocator }
	{
		DescriptorSetLayout heapLayout;
		heapLayout.AddUAVs(1);	// output

		PipelineInterfaceBuilder builder{ allocator };
		builder.AllowIA = true;
		builder.SetParameterAsUINT(0, 40);
		builder.SetParameterAsCBV(1);
		builder.SetParameterAsUINT(2, 16);
		builder.SetParameterAsCBV(3);
		builder.SetParameterAsDescriptorSet(4, heapLayout);

		rootSignature = builder.Build(allocator);
		FK_ASSERT(rootSignature != nullptr, "Failed to create Root Signature!");

		renderSystem.RegisterPSOLoader(SHADOWMAPPASS,			{ this, &ShadowMapper::CreateShadowMapPass });
		renderSystem.RegisterPSOLoader(SHADOWMAPANIMATEDPASS,	{ this, &ShadowMapper::CreateShadowMapAnimatedPass });
		renderSystem.RegisterPSOLoader(BUILDROWSUMS,			{ this, &ShadowMapper::CreateRowSums });
		renderSystem.RegisterPSOLoader(BUILDCOLUMNSUMS,			{ this, &ShadowMapper::CreateColumnSums });
	}


	/************************************************************************************************/


	ShadowMapPassData& ShadowMapper::ShadowMapPass(
								FrameGraph&								frameGraph,
								const GatherVisibleLightsTask&			visibleLights,
								LightUpdate&							lightUpdate,
								UpdateTask&								cameraUpdate,
								GatherPassesTask&						passes,
								std::span<AdditionalShadowMapPass>		additional,
								const double							t,
		                        PoolAllocatorInterface&					shadowMapPool,
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
					static const ClearValue clearValue{
						.format = DeviceFormat::R32_FLOAT,
						.color	{ f_max, f_max, f_max, f_max }
					};

					LightComponent& lightComponent = LightComponent::GetComponent();

					for(auto [handleIdx, lightHandle] : enumerate(lights))
					{
						if (handleIdx >= frameHandles.size())
							return;

						auto& light = lightComponent[lightHandle];

						switch(light.type)
						{
							case LightType::PointLight:
							{
								auto desc		= GPUResourceDesc::CubeMapUAV({ ShadowMapSize, ShadowMapSize }, DeviceFormat::R32_FLOAT, 1, true);
								desc.clearValue = clearValue;

								auto&& [shadowMap, _] = resourceCtx.AcquireTemporary(frameHandles[handleIdx], desc);
								break;
							}
							case LightType::SpotLight:
							case LightType::SpotLightBasicShadows:
							{
								auto desc		= GPUResourceDesc::UAVTexture({ ShadowMapSize, ShadowMapSize }, DeviceFormat::R32_FLOAT, true);
								desc.clearValue = clearValue;

								auto&& [shadowMap, _] = resourceCtx.AcquireTemporary(frameHandles[handleIdx], desc);
							}	break;
						}
					}
				},
			.layout		= DeviceLayout::DepthStencilWrite,
			.access		= DeviceAccessState::DASDEPTHBUFFERWRITE,
			.max		= Max((uint32_t)ShadowMapComponent::GetComponent().size(), 16),
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

				for (auto [idx, lightHandle] : enumerate(lightList))
				{
					auto& light = lights[lightHandle];

					switch (light.type)
					{
					case LightType::PointLightNoShadows:
					case LightType::SpotLightNoShadows:
					case LightType::Direction:
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
			.sharedData = {},
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

		auto passDrawFN = [&, this](auto begin, auto end, std::span<VisibilityHandle> pvs, PassData& pass, Common_Data& common, FrameResources& resources, IDirectContext& ctx, iAllocator& allocator)
		{
			ProfileFunction();
			ctx.BeginEvent_DEBUG("DepthPass");

			const ResourceHandle passTarget		= resources.GetResource(acquireMaps[pass.passIdx]);
			const ResourceHandle depthBuffer	= resources.GetResource(common.depthBuffer);

			auto& light = LightComponent::GetComponent()[pass.pointLightHandle];

			if (light.type == LightType::PointLightNoShadows ||
				light.type == LightType::SpotLightNoShadows)
				return;

			FK_ASSERT(passTarget == InvalidHandle);

			if (begin == pvs.begin())
			{
				resources.renderSystem->SetDebugName(passTarget, "passTarget");
				resources.renderSystem->SetDebugName(depthBuffer, "depthBuffer");

				const auto initialLayout = ctx.GetRenderSystem().GetObjectLayout(passTarget);

				constexpr float f_max = std::numeric_limits<float>::max();

				ctx.AddTextureBarrier(passTarget,
					DASCommon,			DASRenderTarget,
					initialLayout,		DeviceLayout::RenderTarget,
					Sync_PixelShader,	Sync_All);

				ctx.ClearDepthBuffer(depthBuffer, 1.0f);
				ctx.ClearRenderTarget(passTarget, f_max);
				
				ctx.AddTextureBarrier(depthBuffer,
					DASDEPTHBUFFERWRITE,			DASDEPTHBUFFERWRITE,
					DeviceLayout::DepthStencilWrite, DeviceLayout::DepthStencilWrite,
					Sync_All,						Sync_DepthStencil);

				ctx.AddTextureBarrier(passTarget,
					DASRenderTarget,				DASRenderTarget,
					DeviceLayout::RenderTarget,		DeviceLayout::RenderTarget,
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
					additionalDraws(passTarget, light.type, resources, ctx, allocator);

				switch (light.type)
				{
				case LightType::PointLight:
					ctx.AddTextureBarrier(passTarget,
						DASRenderTarget,			DASPixelShaderResource,
						DeviceLayout::RenderTarget,	DeviceLayout::ShaderResource,
						Sync_RenderTarget,			Sync_PixelShader);
					break;
				case LightType::SpotLight:
				case LightType::SpotLightBasicShadows:
					BuildSummedAreaTable(passTarget, resources, ctx, allocator);
					break;
				}

				//ctx.renderSystem->SetObjectLayout(pass.renderTarget, DeviceLayout::SRV);
			}
		};

		auto& multiPass = frameGraph.AddDataDrivenMultiPass(passDesc, passSetupFn, passDrawFN);

		return shadowMapPass;
	}


	/************************************************************************************************/


	void ShadowMapper::RenderPointLightShadowMap(ShadowMapper::Iterator_TY begin, ShadowMapper::Iterator_TY end, ShadowMapper::PassData& pass, ShadowMapper::Common_Data& common, ResourceHandle passTarget, FrameResources& resources, IDirectContext& ctx, iAllocator& allocator)
	{
		BrushDrawList			drawList		{ allocator, (size_t)std::distance(begin, end) };
		Vector<GameObject*>		animatedDrawList{ allocator };

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
						PushDraw(entity, view.GetBrush(), drawList, lightPosition);
					}
					else if (auto res = std::find_if(passes.begin(), passes.end(),
							[](auto& pass)
							{
								return pass == SHADOWMAPANIMATEDPASS;
							}); res != passes.end())
						{
							animatedDrawList.push_back(&entity);
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

		auto shadowMapPSO	= resources.GetPipelineState(SHADOWMAPPASS, allocator);

		ctx.SetPipelineState(shadowMapPSO);
		ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

		TriMeshHandle	currentMesh		= InvalidHandle;
		size_t			currentLodIdx	= 0xffffffffffffffff;
		size_t			indexCount		= 0;
		BoundingSphere	BS;

		const float3 position	= FlexKit::GetPositionW(light.node);
		const auto matrices		= CalculateShadowMapMatrices(position, light.R);

		for(uint32_t brushIdx = 0; brushIdx < (uint32_t)drawList.size(); brushIdx++)
		{
			auto& draw			= drawList[brushIdx];
			const auto lodLevel	= draw.LODlevel;
			auto& meshes		= draw.brush->meshes;

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
						{ VERTEXBUFFER_TYPE::POSITION });

					BS = triMesh->bs;
				}

				const float4x4 WT   = GetWT(draw.brush->Node);
				const float3 POS    = GetPositionW(draw.brush->Node);
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
					ctx.SetGraphicsConstantValue(2, 16, float4x4_GPU{ WT });

					for (uint32_t itr = 0; itr < 6; itr++)
					{
						if (intersections[itr])
						{
							struct
							{
								float4x4_GPU	PV;
								float4x4_GPU	View;
								uint32_t		Idx;
								float			maxZ;
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
			float4x4_GPU M[256];
		};

		CBPushBuffer animatedConstantBuffer = resources.ReserveCB((AlignedSize<Brush::VConstantsLayout>() + AlignedSize<PoseConstants>()) * animatedDrawList.size());
		if(animatedDrawList.size())
		{
			auto PSOAnimated = resources.GetPipelineState(SHADOWMAPANIMATEDPASS, allocator);
			ctx.SetPipelineState(PSOAnimated);
			ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

			for (const auto& draw : animatedDrawList)
			{
				Apply(*draw,
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
								{   VERTEXBUFFER_TYPE::POSITION,
									VERTEXBUFFER_TYPE::ANIMATION1,
									VERTEXBUFFER_TYPE::ANIMATION2,
								});

							const auto poseConstants = ConstantBufferDataSet{ poseTemp, animatedConstantBuffer };

							const float4x4 WT			= nodeView.GetWT();
							const float4 brushPOS_WT	= WT * float4(triMesh->bs.xyz(), 1);

							ctx.SetGraphicsConstantValue(2, 16, float4x4{ WT });
							ctx.SetGraphicsConstantBufferView(3, poseConstants);

							for (uint32_t itr = 0; itr < 6; itr++)
							{
								struct 
								{
									float4x4_GPU	PV;
									float4x4_GPU	View;
									uint32_t		Idx;
									float			maxZ;
								}tempConstants = {
										.PV		= matrices.PV[itr],
										.View	= matrices.View[itr],
										.Idx	= itr,
										.maxZ	= light.R
								};

								if (Intersects(fustrum[itr], float4{ brushPOS_WT.xyz(), triMesh->bs.w }))
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


	void ShadowMapper::RenderSpotLightShadowMap(Iterator_TY begin, Iterator_TY end, PassData& pass, ShadowMapper::Common_Data& common, ResourceHandle passTarget, FrameResources& resources, IDirectContext& ctx, iAllocator& allocator)
	{
		BrushDrawList			drawList		{ allocator, (size_t)std::distance(begin, end) };
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
						PushDraw(entity, view.GetBrush(), drawList, lightPosition);
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


		auto shadowMapPSO	= resources.GetPipelineState(SHADOWMAPPASS, allocator);
		auto Build2DSATPSO	= resources.GetPipelineState(SHADOWMAPPASS, allocator);

		ctx.SetPipelineState(shadowMapPSO);
		ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

		TriMeshHandle	currentMesh		= InvalidHandle;
		size_t			currentLodIdx	= 0xffffffffffffffff;
		size_t			indexCount		= 0;

		const auto [pv, v] = CalculateSpotLightMatrices(lightPosition, lightOrientation, light.R, light.outerAngle);

		for(uint32_t brushIdx = 0; brushIdx < (uint32_t)drawList.size(); brushIdx++)
		{
			auto& visible		= drawList[brushIdx];
			const auto lodLevel	= visible.LODlevel;
			auto& meshes		= visible.brush->meshes;

			const float4x4	WT		= GetWT(visible.brush->Node);
			const float3	POS		= GetPositionW(visible.brush->Node);
			const float		scale	= Max(WT[0][0], Max(WT[1][1], WT[2][2]));

			struct
			{
				float4x4_GPU	PV;
				float4x4_GPU	View;
				uint32_t		Idx;
				float			maxZ;
			}drawConstants = {
					.PV		= pv,
					.View	= v,
					.Idx	= 0,
					.maxZ	= light.R };

			ctx.SetGraphicsConstantValue(2, 16, float4x4_GPU{ WT });
			ctx.SetGraphicsConstantValue(0, 34, &drawConstants);

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
						{ VERTEXBUFFER_TYPE::POSITION });
				}

				ctx.DrawIndexedInstanced(indexCount);
			}
		}

		struct PoseConstants
		{
			float4x4_GPU M[256];
		};

		CBPushBuffer animatedConstantBuffer = resources.ReserveCB((AlignedSize<Brush::VConstantsLayout>() + AlignedSize<PoseConstants>()) * animatedBrushes.size());
		if(animatedBrushes.size())
		{
			auto PSOAnimated = resources.GetPipelineState(SHADOWMAPANIMATEDPASS, allocator);
			ctx.SetPipelineState(PSOAnimated);
			ctx.SetScissorAndViewports({ passTarget });
			ctx.SetRenderTargets({}, true, passTarget);
			ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

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
							float4x4_GPU	PV;
							float4x4_GPU	View;
							uint32_t		Idx;
							float			maxZ;
						}drawConstants = {
								.PV		= pv,
								.View	= v,
								.Idx	= 0,
								.maxZ	= light.R
						};

						ctx.SetGraphicsConstantValue(2, 16, float4x4_GPU{ WT });
						ctx.SetGraphicsConstantValue(0, 40, &drawConstants);
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
								{   VERTEXBUFFER_TYPE::POSITION,
									VERTEXBUFFER_TYPE::ANIMATION1,
									VERTEXBUFFER_TYPE::ANIMATION2,
								});

							ctx.DrawIndexedInstanced(indexCount);
						}
					});
			}
		}
	}


	/************************************************************************************************/


	void ShadowMapper::BuildSummedAreaTable(ResourceHandle target, FrameResources& resources, IDirectContext& ctx, iAllocator& allocator)
	{
#if 0 
		auto rowPSO		= resources.GetPipelineState(BUILDROWSUMS);
		auto columnPSO	= resources.GetPipelineState(BUILDCOLUMNSUMS);

		ctx.SetComputeRootSignature(rootSignature);


		ctx.AddTextureBarrier(target,
			DASRenderTarget,			DASUAV,
			DeviceLayout::RenderTarget,	DeviceLayout::UnorderedAccess,
			Sync_RenderTarget,			Sync_Compute);

		DescriptorSet heap{ ctx, rootSignature.GetDescriptorSetLayout(0), allocator };
		heap.SetUAVTexture(ctx, 0, target);

		ctx.SetComputeDescriptorSet(4, heap);
		ctx.Dispatch(rowPSO, { 1, 1024, 1});

		ctx.AddUAVBarrier(target, -1, DeviceLayout::UnorderedAccess);

		ctx.Dispatch(columnPSO, { 1024, 1, 1 });

		ctx.AddTextureBarrier(target,
			DASUAV,							DASPixelShaderResource,
			DeviceLayout::UnorderedAccess,	DeviceLayout::ShaderResource,
			Sync_Compute,					Sync_PixelShader);
#else
	ctx.AddTextureBarrier(target,
		DASRenderTarget,			DASPixelShaderResource,
		DeviceLayout::RenderTarget,	DeviceLayout::ShaderResource,
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
