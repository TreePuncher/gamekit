#include "PhysicsDebugVis.h"
#include "FrameGraph.h"
#include "physicsutilities.h"
#include "defaultpipelinestates.h"


namespace FlexKit
{	/************************************************************************************************/


	ID3D12PipelineState* CreateWireframeDebugVis(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("V12Main",			"vs_6_0",	"assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawLinearDepth",	"ps_6_0",	"assets\\shaders\\pshader.hlsl");

		const D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 0,					D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, sizeof(float4),		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, sizeof(float4)*2,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode			= D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
		Rast_Desc.CullMode			= D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
		//Depth_Desc.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
		Depth_Desc.DepthEnable		= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS						= DrawRectVShader;
			PSO_Desc.PS						= DrawRectPShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "DrawLinearDepthWireFrameDebug");

		return PSO;
	}

	
	/************************************************************************************************/


	ID3D12PipelineState* CreateSolidDebugVis(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("V12Main",			"vs_6_0",	"assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawLinearDepth",	"ps_6_0",	"assets\\shaders\\pshader.hlsl");

		const D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 0,					D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, sizeof(float4),		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, sizeof(float4)*2,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode			= D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		Rast_Desc.CullMode			= D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc		= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
		Depth_Desc.DepthEnable		= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS						= DrawRectVShader;
			PSO_Desc.PS						= DrawRectPShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "DrawLinearDepth");

		return PSO;
	}

	static const PSOHandle Wireframe	= PSOHandle(GetTypeGUID(CreateWireframeDebugVis));
	static const PSOHandle Solid		= PSOHandle(GetTypeGUID(CreateSolidDebugVis));


	/************************************************************************************************/


	void RegisterPhysicsDebugVis(RenderSystem& renderSystem)
	{
		renderSystem.RegisterPSOLoader(Wireframe,	{ &renderSystem.Library.RS6CBVs4SRVs, CreateWireframeDebugVis });
		renderSystem.RegisterPSOLoader(Solid,		{ &renderSystem.Library.RS6CBVs4SRVs, CreateSolidDebugVis });
	}


	/************************************************************************************************/


	PhysicsDebugOverlayPass& RenderPhysicsOverlay(
		FrameGraph&								frameGraph,
		ResourceHandle							renderTarget,
		ResourceHandle							depthTarget,
		LayerHandle								layer,
		CameraHandle							camera,
		const ReserveVertexBufferFunction&		reserveVB,
		const ReserveConstantBufferFunction&	reserveCB)
	{
		return frameGraph.AddNode<PhysicsDebugOverlayPass>(
			PhysicsDebugOverlayPass{
				reserveVB,
				reserveCB
			},
			[&](FrameGraphNodeBuilder& builder, PhysicsDebugOverlayPass& data)
			{
				data.renderTarget	= builder.RenderTarget(renderTarget);
				data.depthTarget	= builder.DepthTarget(depthTarget);
			},
			[layer, camera](PhysicsDebugOverlayPass& pass, const FlexKit::ResourceHandler& frameResources, FlexKit::Context& ctx, [[maybe_unused]]auto& allocator)
			{
				auto& layer_ref	= FlexKit::PhysXComponent::GetComponent().GetLayer_ref(layer);
				auto& geometry	= layer_ref.debugGeometry;

				if (geometry.size())
				{
					ctx.BeginEvent_DEBUG("Physics Debug Overlay");

					auto VBuffer = pass.reserveVB(geometry.size() * sizeof(geometry[0]));
					auto CBuffer = pass.reserveCB(sizeof(GetCameraConstants(camera)) + sizeof(Brush::VConstantsLayout));

					struct Constants
					{
						float4 albedo;
						float4 specular;
						float4x4 WT;
					} passConsants {
						.WT = float4x4::Identity()
					};

					VertexBufferDataSet		vertices		{ geometry, VBuffer };
					ConstantBufferDataSet	cameraConstants	{ GetCameraConstants(camera), CBuffer };
					ConstantBufferDataSet	entityConstants	{ passConsants, CBuffer };

					const auto rt	= frameResources.GetResource(pass.renderTarget);
					static auto PSO = frameResources.GetPipelineState(Wireframe);
					ctx.SetRootSignature(frameResources.renderSystem().Library.RS6CBVs4SRVs);
					ctx.SetScissorAndViewports({ rt });
					ctx.SetRenderTargets({ rt }, true, frameResources.GetResource(pass.depthTarget));
					ctx.SetVertexBuffers({ vertices });
					ctx.SetGraphicsConstantBufferView(1, cameraConstants);
					ctx.SetGraphicsConstantBufferView(2, entityConstants);

					// Draw triangles
					ctx.SetPipelineState(frameResources.GetPipelineState(Solid));
					ctx.SetPrimitiveTopology(EIT_TRIANGLE);
					ctx.Draw(layer_ref.debugTriCount);

					// Draw lines
					ctx.SetPipelineState(PSO);
					ctx.SetPrimitiveTopology(EIT_LINE);
					ctx.Draw(layer_ref.debugLineCount, layer_ref.debugTriCount);

					ctx.EndEvent_DEBUG();
				}
			});
	}


}	/************************************************************************************************/

