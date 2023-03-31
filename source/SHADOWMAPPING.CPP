#pragma once
#include "ShadowMapping.h"
#include "AnimationComponents.h"
#include "AnimationRendering.h"
#include <fmt/format.h>
#include <limits>

namespace FlexKit
{   /************************************************************************************************/


	ID3D12PipelineState* ShadowMapper::Fill2DPlanes(RenderSystem* RS)
	{
		auto CShader = RS->LoadShader("ColumnSumsMain", "cs_6_0", "assets\\shaders\\SACM\\Create2DSAT.hlsl");


		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_Desc = {};
		PSO_Desc.pRootSignature	= rootSignature;
		PSO_Desc.CS				= CShader;

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
			//PSO_Desc.RTVFormats[0]							= DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
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
			//PSO_Desc.RTVFormats[0]							= DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
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

		ShadowPassMatrices out;

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
		heapLayout.SetParameterAsSRV(0, 0, 1);			// input
		heapLayout.SetParameterAsShaderUAV(1, 0, 1);	// output

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

		renderSystem.RegisterPSOLoader(BUILD2DSAT,				{ &rootSignature, [&](RenderSystem* rs) { return Fill2DPlanes(rs); } });
	}


	AcquireShadowMapTask& ShadowMapper::AcquireShadowMaps(UpdateDispatcher& dispatcher, RenderSystem& renderSystem, MemoryPoolAllocator& UAVPool, PointLightUpdate& pointLightUpdate)
	{
		return dispatcher.Add<AcquireShadowMapResources>(
			[&](UpdateDispatcher::UpdateBuilder& builder, AcquireShadowMapResources& data)
			{
				ProfileFunction();

				builder.AddInput(pointLightUpdate);
			},
			[&dirtyList = pointLightUpdate.GetData().dirtyList, &renderSystem = renderSystem, &shadowMapAllocator = UAVPool, this]
			(AcquireShadowMapResources& data, iAllocator& threadLocalAllocator)
			{
				ProfileFunction();

				size_t idx = 0;

				auto&			lights		= LightComponent::GetComponent();
				auto			itr			= dirtyList.begin();
				const auto		end			= dirtyList.end();
				const uint64_t	frameID		= renderSystem.graphicsSubmissionCounter;

				for (;itr != end; itr++)
				{
					auto lightHandle	= *itr;
					auto& light			= lights[lightHandle];

					if (light.shadowMap != InvalidHandle)
					{
						AddResource(light.shadowMap, frameID);
						light.shadowMap = InvalidHandle;
					}

					if (auto res = GetResource(frameID); res != InvalidHandle)
					{
						light.shadowMap = res;
					}
					else
						break;
				}

				for (auto i = itr; i != end; i++)
				{
					auto lightHandle	= *i;
					auto& light			= lights[lightHandle];

					if (light.shadowMap != InvalidHandle)
					{
						AddResource(light.shadowMap, frameID);
						light.shadowMap == InvalidHandle;
					}
				}


				Parallel_For(
					renderSystem.threads, threadLocalAllocator, itr, end, 1,
					[&](auto lightHandle, iAllocator& threadLocalAllocator)
					{
						ProfileFunction();

						D3D12_CLEAR_VALUE clearValue{
							.Format = DXGI_FORMAT_D32_FLOAT,
							.Color	{ 1, 1, 1, 1 }
						};

						auto desc		= GPUResourceDesc::DepthTarget({ ShadowMapSize, ShadowMapSize }, DeviceFormat::D32_FLOAT, 6);
						desc.clearValue = clearValue;

						auto& light			= lights[lightHandle];
						auto [shadowMap, _]	= shadowMapAllocator.Acquire(desc, false);
						light.shadowMap		= shadowMap;

						if (shadowMap == InvalidHandle)
							DebugBreak();

#if USING(DEBUGGRAPHICS)
						auto debugName = fmt::format("ShadowMap:{}:{}", renderSystem.graphicsSubmissionCounter, light.shadowMap);
						renderSystem.SetDebugName(shadowMap, debugName.c_str());
#endif
					});
			});
	}


	/************************************************************************************************/


	ShadowMapPassData& ShadowMapper::ShadowMapPass(
		FrameGraph&								frameGraph,
		const LightShadowGatherTask&			shadowMapsGather,
		UpdateTask&								cameraUpdate,
		GatherPassesTask&						gather,
		AcquireShadowMapTask&					shadowMapAcquire,
		ReserveConstantBufferFunction&			reserveCB,
		ReserveVertexBufferFunction&			reserveVB,
		static_vector<AdditionalShadowMapPass>&	additional,
		const double							t,
		iAllocator&								tempAllocator,
		AnimationPoseUpload*					poses)
	{
		auto& shadowMapPass = tempAllocator.allocate<ShadowMapPassData>();

		struct Common_Data
		{
			ReserveConstantBufferFunction	reserveCB;
			ReserveVertexBufferFunction		reserveVB;
		};

		auto& lights = LightComponent::GetComponent();

		auto& shadowMap_data	= shadowMapsGather.GetData();
		auto& shadowMaps		= shadowMap_data.lights;

		struct PassData
		{
			ResourceHandle		renderTarget;
			LightHandle	pointLightHandle;
			CubeMapState*		shadowMapData;
		};

		auto getPasses = 
			[&](iAllocator& allocator) -> Vector<PassData>
			{
				auto& shadowMap_data	= shadowMapsGather.GetData();
				auto& lightList			= shadowMap_data.lights;

				Vector<PassData> passes{ allocator };
				passes.reserve(shadowMaps.size());

				for (auto lightHandle : lightList)
				{
					auto& light = lights[lightHandle];

					if (light.state != LightStateFlags::Dirty || light.shadowMap == InvalidHandle)
						continue;

					passes.emplace_back(light.shadowMap, lightHandle, light.shadowState);
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
			builder.AddDataDependency(shadowMapAcquire);
			shadowMapPass.multiPass		= builder.GetNodeHandle();
		};

		auto passDrawFN = [&, this](auto begin, auto end, std::span<VisibilityHandle> pvs, PassData& pass, Common_Data& common, FrameResources& resources, Context& ctx, iAllocator& allocator)
		{
			ProfileFunction();
			ctx.BeginEvent_DEBUG("DepthPass");

			const ResourceHandle passTarget	= pass.renderTarget;
			if (passTarget == InvalidHandle)
				DebugBreak();

			resources.renderSystem.SetDebugName(passTarget,	"passTarget");

			if (begin == pvs.begin())
			{
				const auto initialLayout = ctx.renderSystem->GetObjectLayout(passTarget);

				ctx.AddTextureBarrier(passTarget,
					DASCommon,			DASDEPTHBUFFERWRITE,
					initialLayout,		DeviceLayout_DepthStencilWrite,
					Sync_PixelShader,	Sync_All);

				ctx.ClearDepthBuffer(passTarget, 1.0f);
				
				ctx.AddTextureBarrier(passTarget,
					DASDEPTHBUFFERWRITE,			DASDEPTHBUFFERWRITE,
					DeviceLayout_DepthStencilWrite, DeviceLayout_DepthStencilWrite,
					Sync_All,						Sync_DepthStencil);
			}

			auto&			visiblityComponent	= SceneVisibilityComponent::GetComponent();
			auto&			light				= lights[pass.pointLightHandle];
			const float3	lightPosition		= GetPositionW(light.Position);
			auto&			materials			= MaterialComponent::GetComponent();

			PVS						brushes	{ allocator, (size_t)std::distance(begin, end) };
			Vector<GameObject*>		animatedBrushes{ allocator };

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
			auto Build2DSATPSO	= resources.GetPipelineState(SHADOWMAPPASS);


			const DepthStencilView_Options DSV_desc{ 0, 0, pass.renderTarget };

			ctx.SetRootSignature(rootSignature);
			ctx.SetPipelineState(shadowMapPSO);
			ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
			ctx.SetScissorAndViewports({ passTarget });
			ctx.SetRenderTargets2({}, 0, DSV_desc);

			TriMeshHandle	currentMesh		= InvalidHandle;
			size_t			currentLodIdx	= 0xffffffffffffffff;
			size_t			indexCount		= 0;
			BoundingSphere	BS;

			const float3 position	= FlexKit::GetPositionW(light.Position);
			const auto matrices		= CalculateShadowMapMatrices(position, light.R, (float)t);

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

			ctx.EndEvent_DEBUG();

			if (end == pvs.end())
			{
				ctx.AddTextureBarrier(passTarget,
					DASDEPTHBUFFERWRITE,			DASPixelShaderResource,
					DeviceLayout_DepthStencilWrite,	DeviceLayout_ShaderResource,
					Sync_DepthStencil,				Sync_PixelShader);

				ctx.renderSystem->SetObjectLayout(pass.renderTarget, DeviceLayout_ShaderResource);
			}
		};

		auto& multiPass = frameGraph.AddDataDrivenMultiPass(passDesc, passSetupFn, passDrawFN);

		return shadowMapPass;
	}


}   /************************************************************************************************/
