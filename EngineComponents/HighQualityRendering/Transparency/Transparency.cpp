#include <CameraComponent.hpp>
#include <FrameGraph.hpp>
#include <Materials.hpp>
#include <RenderSystemInterface.hpp>
#include <Transparency.hpp>
#include <WorldRender.hpp>


namespace FlexKit
{   /************************************************************************************************/


	struct OITPass
	{
		UpdateTaskTyped<GetDrawListTaskData>& drawList;

		CameraHandle			camera;
		FrameResourceHandle		accumalatorObject;
		FrameResourceHandle		counterObject;
		FrameResourceHandle		depthTarget;
	};


	/************************************************************************************************/


	struct OITBlend
	{
		FrameResourceHandle		renderTargetObject;
		FrameResourceHandle		accumalatorObject;
		FrameResourceHandle		counterObject;
		FrameResourceHandle		depthTarget;
	};


	/************************************************************************************************/


	struct OIT_MLAB
	{
		FrameResourceHandle		renderTargetObject;
		FrameResourceHandle		accumalatorObject;
		FrameResourceHandle		counterObject;
		FrameResourceHandle		depthTarget;
	};


	/************************************************************************************************/


	LoadPipelineStateRes	CreateOITBlendPSO			(IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes	CreateOITDrawPSO			(IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes	CreateOITDrawAnimatedPSO	(IRenderSystem& RS, iAllocator& allocator);

	LoadPipelineStateRes	CreateMarkClustersPSO		(IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes	CreateMLABDrawPSO			(IRenderSystem& RS, iAllocator& allocator);


	/************************************************************************************************/


	LoadPipelineStateRes CreateOITDrawPSO(IRenderSystem& rs, iAllocator& allocator)
	{
	    PipelineBuilder{ rs, allocator }
		    .AddInputTopology(ETopology::EIT_TRIANGLE)
		    .AddInputLayout({
		        .inputs = {
		            { "POSITION",	0, DeviceFormat::R32G32B32_FLOAT,	0, 0,	EInputClassification::PerVertex, 0 },
			        { "NORMAL",		0, DeviceFormat::R32G32B32_FLOAT,	1, 0,	EInputClassification::PerVertex, 0 },
			        { "TANGENT",	0, DeviceFormat::R32G32B32_FLOAT,	2, 0,	EInputClassification::PerVertex, 0 },
			        { "TEXCOORD",	0, DeviceFormat::R32G32_FLOAT,		3, 0,	EInputClassification::PerVertex, 0 },
				},
		        .count	= 4 }
			)
			.AddVertexShader("VMain", "assets\\shaders\\OITPass.hlsl")
			.AddPixelShader("PassMain", "assets\\shaders\\OITPass.hlsl")
            .AddRasterizerState({})
            .AddDepthStencilState({
				    .depthEnable	= true,
                    .depthWriteMask = EDepthWriteMask::Zero,
                    .depthFunc		= EComparison::LESS,
                })
	        .AddRootSignature(rs.Library(ROOTLIBRARYSIG::RSDefault))
			.AddInputTopology(ETopology::EIT_TRIANGLE)
            .AddRenderTargetState({.targetCount =  1, .targetFormats = { DeviceFormat::R16G16B16A16_FLOAT, DeviceFormat::R16G16B16A16_FLOAT }})
            .AddDepthStencilFormat(DeviceFormat::D32_FLOAT)
            .AddBlendState(
				BlendState{
					.independentBlendEnable = true,
					.renderTarget = {
						RenderTargetStateDesc{
						    .blendEnable	= true,
							.srcBlend		= EBlend::INV_SRC_ALPHA,
							.dstBlend		= EBlend::SRC_ALPHA,
							.blendOp		= EBlendOP::ADD,
							.srcBlendAlpha	= EBlend::ONE,
							.dstBlendAlpha	= EBlend::ONE,
							.blendOpAlpha	= EBlendOP::ADD,
						},
						RenderTargetStateDesc{
						    .blendEnable	= true,
							.srcBlend		= EBlend::ZERO,
							.dstBlend		= EBlend::INV_SRC_ALPHA,
							.blendOp		= EBlendOP::ADD,
							.srcBlendAlpha	= EBlend::ZERO,
							.dstBlendAlpha	= EBlend::INV_SRC_ALPHA,
							.blendOpAlpha	= EBlendOP::ADD,
						},
					},
				})
	        .Build(rs);
		/*
		auto VShader = RS.LoadShader("VMain",		"vs_6_0", "assets\\shaders\\OITPass.hlsl");
		auto PShader = RS.LoadShader("PassMain",	"ps_6_0", "assets\\shaders\\OITPass.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	1, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	2, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,		3, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode						= D3D12_CULL_MODE_BACK;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable					= true;
		Depth_Desc.DepthWriteMask				= D3D12_DEPTH_WRITE_MASK_ZERO;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS.Library(ROOTLIBRARYSIG::RSDefault)->GetAPIObject();
			PSO_Desc.VS						= Shader2ByteCode(VShader);
			PSO_Desc.PS						= Shader2ByteCode(PShader);
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 2;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.RTVFormats[1]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // count
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;

			PSO_Desc.BlendState.IndependentBlendEnable			= true;

			PSO_Desc.BlendState.RenderTarget[0].BlendEnable		= true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp			= D3D12_BLEND_OP_ADD;
			PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha	= D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlend		= D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha	= D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend		= D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha	= D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[1].BlendEnable		= true;
			PSO_Desc.BlendState.RenderTarget[1].BlendOp			= D3D12_BLEND_OP_ADD;
			PSO_Desc.BlendState.RenderTarget[1].BlendOpAlpha	= D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[1].SrcBlend		= D3D12_BLEND_ZERO;
			PSO_Desc.BlendState.RenderTarget[1].SrcBlendAlpha	= D3D12_BLEND_ZERO;

			PSO_Desc.BlendState.RenderTarget[1].DestBlend		= D3D12_BLEND_INV_SRC_ALPHA;
			PSO_Desc.BlendState.RenderTarget[1].DestBlendAlpha	= D3D12_BLEND_INV_SRC_ALPHA;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS.pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, RS.Library(ROOTLIBRARYSIG::RSDefault) };
        */
	    return {};
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateOITDrawAnimatedPSO(IRenderSystem& RS, iAllocator& allocator)
	{
		return {};
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateOITBlendPSO(IRenderSystem& RS, iAllocator& allocator)
	{
		PipelineBuilder{ RS, allocator }
			.AddVertexShader("VMain", "assets\\shaders\\OITBlend.hlsl")
			.AddPixelShader("BlendMain", "assets\\shaders\\OITBlend.hlsl")
            .AddRasterizerState({})
            .AddDepthStencilState({
				    .depthEnable	= true,
                    .depthFunc		=  FlexKit::EComparison::LESS,
                })
	        .AddRootSignature(RS.Library(ROOTLIBRARYSIG::RSDefault))
			.AddInputTopology(ETopology::EIT_TRIANGLE)
            .AddRenderTargetState({.targetCount =  1, .targetFormats = {DeviceFormat::R16G16B16A16_FLOAT}})
            .AddDepthStencilFormat(DeviceFormat::D32_FLOAT)
            .AddBlendState(
				BlendState{
					.independentBlendEnable = true,
					.renderTarget = {
						RenderTargetStateDesc{
						    .blendEnable	= true,
							.srcBlend		= EBlend::INV_SRC_ALPHA,
							.dstBlend		= EBlend::SRC_ALPHA,
							.blendOp		= EBlendOP::ADD,
							.srcBlendAlpha	= EBlend::ONE,
							.dstBlendAlpha	= EBlend::ONE,
						    .blendOpAlpha	= EBlendOP::ADD,
						},
					},
				})
	        .Build(RS);
	    /*
		auto VShader = RS.LoadShader("VMain", "vs_6_0",		"assets\\shaders\\OITBlend.hlsl");
		auto PShader = RS.LoadShader("BlendMain", "ps_6_0",	"assets\\shaders\\OITBlend.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable					= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= static_cast<RenderSystem&>(RS).Library(ROOTLIBRARYSIG::RSDefault)->GetAPIObject();
			PSO_Desc.VS						= Shader2ByteCode(VShader);
			PSO_Desc.PS						= Shader2ByteCode(PShader);
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { nullptr, 0 };
			PSO_Desc.DepthStencilState		= Depth_Desc;

			PSO_Desc.BlendState.IndependentBlendEnable			= true;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable		= true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp			= D3D12_BLEND_OP_ADD;
			PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha	= D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlend		= D3D12_BLEND_INV_SRC_ALPHA;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha	= D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend		= D3D12_BLEND_SRC_ALPHA;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha	= D3D12_BLEND_ONE;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = static_cast<RenderSystem&>(RS).pDevice14->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, static_cast<RenderSystem&>(RS).Library(ROOTLIBRARYSIG::RSDefault) };
        */
		return {};
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateMLABDrawPSO(IRenderSystem& RS, iAllocator& allocator)
	{
#if 0
		auto VShader = RS.LoadShader("VMain",		"vs_6_0", "assets\\shaders\\OITPass.hlsl");
		auto PShader = RS.LoadShader("PassMain",	"ps_6_0", "assets\\shaders\\OITPass.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	1, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	2, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,		3, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode						= D3D12_CULL_MODE_BACK;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable					= true;
		Depth_Desc.DepthWriteMask				= D3D12_DEPTH_WRITE_MASK_ZERO;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= static_cast<RenderSystem&>(RS).Library(ROOTLIBRARYSIG::RSDefault)->GetAPIObject();
			PSO_Desc.VS						= Shader2ByteCode(VShader);
			PSO_Desc.PS						= Shader2ByteCode(PShader);
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 2;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.RTVFormats[1]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // count
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;

			PSO_Desc.BlendState.IndependentBlendEnable			= true;

			PSO_Desc.BlendState.RenderTarget[0].BlendEnable		= true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp			= D3D12_BLEND_OP_ADD;
			PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha	= D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlend		= D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha	= D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend		= D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha	= D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[1].BlendEnable		= true;
			PSO_Desc.BlendState.RenderTarget[1].BlendOp			= D3D12_BLEND_OP_ADD;
			PSO_Desc.BlendState.RenderTarget[1].BlendOpAlpha	= D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[1].SrcBlend		= D3D12_BLEND_ZERO;
			PSO_Desc.BlendState.RenderTarget[1].SrcBlendAlpha	= D3D12_BLEND_ZERO;

			PSO_Desc.BlendState.RenderTarget[1].DestBlend		= D3D12_BLEND_INV_SRC_ALPHA;
			PSO_Desc.BlendState.RenderTarget[1].DestBlendAlpha	= D3D12_BLEND_INV_SRC_ALPHA;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = static_cast<RenderSystem&>(RS).pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, RS.Library(ROOTLIBRARYSIG::RSDefault) };
#endif
		return {};
	}


	/************************************************************************************************/


	Transparency::Transparency(IRenderSystem& renderSystem, iAllocator& allocator) 
		//MLABDrawSignature{ allocator }
	{
		renderSystem.RegisterPSOLoader(OITBLEND,		CreateOITBlendPSO);
		renderSystem.RegisterPSOLoader(OITDRAW,			CreateOITDrawPSO);
		renderSystem.RegisterPSOLoader(OITDRAWANIMATED,	CreateOITDrawAnimatedPSO);
	}


	/************************************************************************************************/


	OITPass& Transparency::OIT_WB_Pass(
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		GatherPassesTask&				passes,
		CameraHandle					camera,
		ResourceHandle					depthTarget,
		iAllocator*						allocator)
	{
		return frameGraph.AddNode<OITPass>(
			OITPass{
				passes,
				camera
			},
			[&](FrameGraphNodeBuilder& builder, OITPass& data)
			{
				builder.AddDataDependency(passes);

				const auto WH = builder.GetRenderSystem().GetTextureWH(depthTarget);

				const auto counterDesc =
					[&] {
					auto renderTarget = GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT);
					renderTarget.clearValue = ClearValue{
						.format = DeviceFormat::R16G16B16A16_FLOAT,
						.color  = { 1.0f, 1.0f, 1.0f, 1.0f },
					};

					return renderTarget; }();

				data.depthTarget		= builder.DepthRead(depthTarget);
				data.accumalatorObject	= builder.AcquireVirtualResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT), DASRenderTarget, VirtualResourceScope::Frame);
				data.counterObject		= builder.AcquireVirtualResource(counterDesc, DASRenderTarget, VirtualResourceScope::Frame);

				builder.SetDebugName(data.accumalatorObject,	"Accumalator");
				builder.SetDebugName(data.counterObject,		"counterObject");
			},
			[=](OITPass& data, ResourceHandler& resources, IDirectContext& ctx, iAllocator& tempAllocator)
			{
				ProfileFunction();

				const IRootSignature*	rootSig		= resources.renderSystem().Library(ROOTLIBRARYSIG::RSDefault);
				auto&					materials	= MaterialComponent::GetComponent();


				TriMeshHandle	previous		= InvalidHandle;
				TriMesh*		triMesh			= nullptr;
				MaterialHandle	prevMaterial	= InvalidHandle;
				
				ctx.ClearRenderTarget(resources.GetResource(data.accumalatorObject));
				ctx.ClearRenderTarget(resources.GetResource(data.counterObject), float4{ 1, 1, 1, 1 });

				auto& passes					= data.drawList.GetData().passes;
				std::span<const BrushEntry> drawList;

				if (auto res = std::find_if(passes.begin(), passes.end(),
					[](auto& pass) -> bool
					{
						return pass.pass == PassHandle{GetCRCGUID(OIT_MCGUIRE)};
					}); res != passes.end())
				{
					drawList = res->drawList;
				}
				else
					return;

				if (!drawList.size())
					return;

				ctx.BeginEvent_DEBUG("OIT - PASS");


				ctx.SetRootSignature(rootSig);
				ctx.SetPipelineState(resources.GetPipelineState(OITDRAW, tempAllocator));

				CBPushBuffer constantBuffer{
					resources.ReserveCB(
						AlignedSize<Brush::VConstantsLayout>() * drawList.size() +
						AlignedSize<Camera::ConstantBuffer>() )};

				const auto cameraConstantValues = GetCameraConstants(data.camera);
				const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };


				ctx.SetGraphicsConstantBufferView(0, cameraConstants);
				ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

				ctx.SetScissorAndViewports({
						resources.GetResource(data.accumalatorObject),
						resources.GetResource(data.counterObject),
					});

				ctx.SetRenderTargets(
					{
						resources.GetResource(data.accumalatorObject),
						resources.GetResource(data.counterObject),
					},
					true,
					resources.GetResource(data.depthTarget)
				);

				for (auto& draw : drawList)
				{
					const auto meshes		= draw.brush->meshes;
					const auto material		= draw.brush->material;

					for(uint32_t I = 0; I < meshes.size(); I++)
					{
						auto lodLevel = draw.LODlevel[I];
						auto mesh = meshes[I];

						if (mesh != previous)
						{
							previous	= mesh;
							triMesh		= GetMeshResource(mesh);

							auto& lod = triMesh->lods[lodLevel];
						
							ctx.AddIndexBuffer(
								triMesh,
								lodLevel);

							ctx.AddVertexBuffers(
								triMesh,
								lodLevel,
								{
									VERTEXBUFFER_TYPE::POSITION,
									VERTEXBUFFER_TYPE::NORMAL,
									VERTEXBUFFER_TYPE::TANGENT,
									VERTEXBUFFER_TYPE::UV,
								}
							);
						}

						if (material != prevMaterial)
						{
							const auto& textures = materials[material].textures;

							DescriptorHeap heap;

							heap.Init2(ctx, rootSig->GetDescHeap(0), textures.size(), &tempAllocator);

							for (size_t I = 0; I < textures.size(); ++I)
								heap.SetSRV(ctx, I, textures[I]);

							ctx.SetGraphicsDescriptorTable(4, heap);
						}

						const auto textureCount     = materials[material].textures.size();
						const auto constantValues   = draw.brush->GetConstants();

						const ConstantBufferDataSet localConstants{ constantValues, constantBuffer };

						ctx.SetGraphicsConstantBufferView(1, localConstants);

						for (auto& subMesh : triMesh->lods[lodLevel].subMeshes)
							ctx.DrawIndexed(
								subMesh.IndexCount,
								subMesh.BaseIndex);
					}

				}

				ctx.EndEvent_DEBUG();
			});
	}


	/************************************************************************************************/


	OITBlend& Transparency::OIT_WB_Blend(
		UpdateDispatcher&		dispatcher,
		FrameGraph&				frameGraph,
		OITPass&				OITPass,
		FrameResourceHandle		renderTarget,
		iAllocator*				allocator)
	{
		return frameGraph.AddNode<OITBlend>(
			OITBlend{},
			[&](FrameGraphNodeBuilder& builder, OITBlend& data)
			{
				data.renderTargetObject	= builder.WriteTransition(renderTarget, DASRenderTarget);
				data.accumalatorObject	= builder.ReadTransition(OITPass.accumalatorObject, DASPixelShaderResource);
				data.counterObject		= builder.ReadTransition(OITPass.counterObject, DASPixelShaderResource);
			},
			[=](OITBlend& data, ResourceHandler& resources, IDirectContext& ctx, iAllocator& tempAllocator)
			{
				ProfileFunction();

				ctx.BeginEvent_DEBUG("OIT - Blend");

				ctx.SetPipelineState(resources.GetPipelineState(OITBLEND, tempAllocator));
				ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

				const IRootSignature* rootSig = resources.renderSystem().Library(ROOTLIBRARYSIG::RSDefault);
				auto& descHeapLayout = rootSig->GetDescHeap(0);
				DescriptorHeap descHeap;

				descHeap.Init2(ctx, descHeapLayout, 2, &tempAllocator);
				descHeap.SetSRV(ctx, 0, resources.PixelShaderResource(data.accumalatorObject, ctx));
				descHeap.SetSRV(ctx, 1, resources.PixelShaderResource(data.counterObject, ctx));

				ctx.SetRootSignature(rootSig);
				ctx.SetGraphicsDescriptorTable(4, descHeap);

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTargetObject), });

				ctx.SetRenderTargets(
					{	resources.GetResource(data.renderTargetObject), },
					false);

				ctx.Draw(6);

				ctx.EndEvent_DEBUG();
			});
	}


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2016-2025 Robert May

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
