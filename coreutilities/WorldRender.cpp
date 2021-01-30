#include "WorldRender.h"
#include "TextureStreamingUtilities.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>
#include <cmath>


namespace FlexKit
{	/************************************************************************************************/


	ID3D12PipelineState* CreateLightPassPSO(RenderSystem* RS)
	{
		auto lightPassShader = RS->LoadShader("tiledLightCulling", "cs_6_0", "assets\\shaders\\lightPass.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
		PSO_desc.CS				= lightPassShader;
		PSO_desc.pRootSignature = RS->Library.ComputeSignature;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardDrawPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("Forward_VS", "vs_6_0",	"assets\\shaders\\forwardRender.hlsl");
		auto DrawRectPShader = RS->LoadShader("Forward_PS", "ps_6_0",	"assets\\shaders\\forwardRender.hlsl");

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


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.PS                    = DrawRectPShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;

			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
		}


		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		//FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateDepthPrePassPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("DepthPass_VS", "vs_6_0",	"assets\\shaders\\forwardRender.hlsl");


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


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 0;
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateGBufferPassPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("Forward_VS",       "vs_6_0",	"assets\\shaders\\forwardRender.hlsl");
		auto DrawRectPShader = RS->LoadShader("GBufferFill_PS",   "ps_6_0",	"assets\\shaders\\forwardRender.hlsl");


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
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.PS                    = DrawRectPShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 4;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Specular
			PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
			PSO_Desc.RTVFormats[3]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Tangent
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateGBufferSkinnedPassPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("ForwardSkinned_VS",    "vs_6_0",	"assets\\shaders\\forwardRender.hlsl");
		auto DrawRectPShader = RS->LoadShader("GBufferFill_PS",       "ps_6_0",	"assets\\shaders\\forwardRender.hlsl");


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
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,  5, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.PS                    = DrawRectPShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 4;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Specular
			PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
			PSO_Desc.RTVFormats[3]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Tangent
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateDeferredShadingPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ShadingPass_VS",     "vs_6_0",	"assets\\shaders\\deferredRender.hlsl");
		auto PShader = RS->LoadShader("DeferredShade_PS",   "ps_6_0",	"assets\\shaders\\deferredRender.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;

			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateBilaterialBlurHorizontalPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ShadingPass_VS",             "vs_6_0", "assets\\shaders\\deferredRender.hlsl");
		auto PShader = RS->LoadShader("BilateralBlurHorizontal_PS", "ps_6_0", "assets\\shaders\\BilateralBlur.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 2;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateBilaterialBlurVerticalPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ShadingPass_VS",             "vs_6_0", "assets\\shaders\\deferredRender.hlsl");
		auto PShader = RS->LoadShader("BilateralBlurVertical_PS",   "ps_6_0", "assets\\shaders\\BilateralBlur.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


    ID3D12PipelineState* CreateClearClusterCountersPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("ClearCounters", "cs_6_0", R"(assets\shaders\ClusteredRendering.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ComputeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateLight_DEBUGARGSVIS_PSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");
        auto PShader = RS->LoadShader("PMain", "ps_6_1", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");
        auto GShader = RS->LoadShader("GMain3", "gs_6_0", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { nullptr, 0 };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateLightBVH_PHASE1_PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateLightBVH_PHASE1", "cs_6_0", R"(assets\shaders\LightBVH.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ComputeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }

    /************************************************************************************************/


    ID3D12PipelineState* CreateLightBVH_PHASE2_PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateLightBVH_PHASE2", "cs_6_0", R"(assets\shaders\LightBVH.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ComputeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateLightBVH_DEBUGVIS_PSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");
        auto PShader = RS->LoadShader("PMain", "ps_6_1", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");
        auto GShader = RS->LoadShader("GMain", "gs_6_0", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { nullptr, 0 };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateCluster_DEBUGVIS_PSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");
        auto PShader = RS->LoadShader("PMain", "ps_6_1", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");
        auto GShader = RS->LoadShader("GMain2", "gs_6_0", "assets\\shaders\\lightBVH_DEBUGVIS.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { nullptr, 0 };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateCluster_DEBUGARGSVIS_PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateArguments", "cs_6_0", R"(assets\shaders\ClusterArgsDebugVis.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateLightListArgs_PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateLightListArguents", "cs_6_0", R"(assets\shaders\LightListArguementIndirect.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ComputeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateClusterLightListsPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateClustersLightLists", "cs_6_0", R"(assets\shaders\lightListConstruction.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ComputeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateClustersPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateClusters", "cs_6_0", R"(assets\shaders\ClusteredRendering.hlsl)");

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
            RS->Library.ComputeSignature,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


	ID3D12PipelineState* CreateComputeTiledDeferredPSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("csmain", "cs_6_0", R"(assets\shaders\computedeferredtiledshading.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateEnvironmentPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("passthrough_VS", "vs_6_0", "assets\\shaders\\DeferredRender.hlsl");
		auto PShader = RS->LoadShader("environment_PS", "ps_6_0", "assets\\shaders\\DeferredRender.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;

			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp     = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardDrawInstancedPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VMain",		"vs_6_0", "assets\\shaders\\DrawInstancedVShader.hlsl");
		auto PShader = RS->LoadShader("FlatWhite",	"ps_6_0", "assets\\shaders\\forwardRender.hlsl");

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
				{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TANGENT",    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

				{ "INSTANCEWT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 0,   D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWT",	1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 16,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWT",	2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 32,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWT",	3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 48,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
			PSO_Desc.BlendState.RenderTarget[0].DestBlend	= D3D12_BLEND::D3D12_BLEND_DEST_COLOR;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlend	= D3D12_BLEND::D3D12_BLEND_SRC_COLOR;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateOcclusionDrawPSO(RenderSystem* RS)
	{
        FK_ASSERT(0);

		return nullptr;

		auto DrawRectVShader = RS->LoadShader("Forward_VS", "vs_6_0",	"assets\\shaders\\forwardRender.hlsl");

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


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 0;
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTexture2CubeMapIrradiancePSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("texture2CubeMap_VS", "vs_6_0", "assets\\shaders\\texture2Cubemap.hlsl");
		auto GShader = RS->LoadShader("texture2CubeMap_GS", "gs_6_0", "assets\\shaders\\texture2Cubemap.hlsl");
		auto PShader = RS->LoadShader("texture2CubeMapDiffuse_PS", "ps_6_0", "assets\\shaders\\texture2Cubemap.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode                      = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			//PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/

	
	ID3D12PipelineState* CreateTexture2CubeMapGGXPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("texture2CubeMap_VS",     "vs_6_0", "assets\\shaders\\texture2Cubemap.hlsl");
		auto GShader = RS->LoadShader("texture2CubeMap_GS",     "gs_6_0", "assets\\shaders\\texture2Cubemap.hlsl");
		auto PShader = RS->LoadShader("texture2CubeMapGGX_PS",  "ps_6_0", "assets\\shaders\\texture2Cubemap.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.CullMode                      = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.PS                    = PShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			//PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


    DrawOutputs& WorldRender::DrawScene(UpdateDispatcher& dispatcher, FrameGraph& frameGraph, DrawSceneDescription& drawSceneDesc, WorldRender_Targets targets, iAllocator* persistent, ThreadSafeAllocator& temporary)
    {
        auto&       scene           = drawSceneDesc.scene;
        const auto  camera          = drawSceneDesc.camera;
        auto&       gbuffer         = drawSceneDesc.gbuffer;
        const auto  t               = drawSceneDesc.t;

        auto        depthTarget     = targets.DepthTarget;
        auto        renderTarget    = targets.RenderTarget;

        auto& PVS = GatherScene(dispatcher, &scene, camera, temporary);
        PVS.AddInput(drawSceneDesc.transformDependency);
        PVS.AddInput(drawSceneDesc.cameraDependency);

        auto& pointLightGather          = scene.GetPointLights(dispatcher, temporary);
        auto& sceneBVH                  = scene.GetSceneBVH(dispatcher, drawSceneDesc.transformDependency, temporary);
        auto& pointLightShadows         = scene.GetVisableLights(dispatcher, camera, sceneBVH, temporary);

        scene.UpdatePointLights(dispatcher, sceneBVH, pointLightShadows, persistent);

        pointLightGather.AddInput(drawSceneDesc.transformDependency);
        pointLightGather.AddInput(drawSceneDesc.cameraDependency);

        auto& skinnedObjects    = GatherSkinned(dispatcher, scene, camera, temporary);
        auto& updatedPoses      = UpdatePoses(dispatcher, skinnedObjects, temporary);

        // [skinned Objects] -> [update Poses] 
        skinnedObjects.AddInput(drawSceneDesc.cameraDependency);
        updatedPoses.AddInput(skinnedObjects);

        const SceneDescription sceneDesc = {
            drawSceneDesc.camera,
            pointLightGather,
            pointLightShadows,
            drawSceneDesc.transformDependency,
            drawSceneDesc.cameraDependency,
            PVS,
            skinnedObjects
        };

        auto& reserveCB = drawSceneDesc.reserveCB;
        auto& reserveVB = drawSceneDesc.reserveVB;

        // Add Resources
        AddGBufferResource(gbuffer, frameGraph);
        frameGraph.AddMemoryPool(&UAVPool);
        frameGraph.AddMemoryPool(&UAVTexturePool);
        frameGraph.AddMemoryPool(&RTPool);

        ClearGBuffer(gbuffer, frameGraph);

        RenderPBR_GBufferPass(
            dispatcher,
            frameGraph,
            sceneDesc,
            sceneDesc.camera,
            gbuffer,
            depthTarget,
            reserveCB,
            temporary);

        auto& lightPass = UpdateLightBuffers(
            dispatcher,
            frameGraph,
            camera,
            scene,
            sceneDesc,
            depthTarget,
            reserveCB,
            temporary);

        auto& shadowMapPass = ShadowMapPass(
            frameGraph,
            sceneDesc,
            renderSystem.GetTextureWH(depthTarget),
            reserveCB,
            t,
            temporary);

        RenderPBR_DeferredShade(
            dispatcher,
            frameGraph,
            sceneDesc,
            pointLightGather,
            gbuffer, depthTarget, renderTarget, shadowMapPass, lightPass,
            reserveCB, reserveVB,
            t,
            temporary);

        if (drawSceneDesc.debugDisplay)
            DEBUGVIS_DrawLightBVH(
                dispatcher,
                frameGraph,
                camera,
                targets.RenderTarget,
                lightPass,
                reserveCB,
                temporary);


        auto& outputs = temporary.allocate<DrawOutputs>(PVS);

        return outputs;
    }


    /************************************************************************************************/


    DepthPass& WorldRender::DepthPrePass(
        UpdateDispatcher&               dispatcher,
        FrameGraph&                     frameGraph,
        const CameraHandle              camera,
        GatherTask&                     pvs,
        const ResourceHandle            depthBufferTarget,
        ReserveConstantBufferFunction   reserveConsantBufferSpace,
        iAllocator*                     allocator)
    {
        const size_t MaxEntityDrawCount = 1000;

        auto& pass = frameGraph.AddNode<DepthPass>(
            pvs.GetData().solid,
            [&, camera](FrameGraphNodeBuilder& builder, DepthPass& data)
            {
                const size_t localBufferSize = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

                data.entityConstantsBuffer = std::move(reserveConsantBufferSpace(sizeof(ForwardDrawConstants) * MaxEntityDrawCount));
                data.passConstantsBuffer = std::move(reserveConsantBufferSpace(2048));
                data.depthBufferObject = builder.WriteDepthBuffer(depthBufferTarget);
                data.depthPassTarget = depthBufferTarget;

                builder.AddDataDependency(pvs);
            },
            [=](DepthPass& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                const auto cameraConstants = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };

                DescriptorHeap heap{
                    ctx,
                    resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
                    &allocator };

                heap.NullFill(ctx);

                ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
                ctx.SetPipelineState(resources.GetPipelineState(DEPTHPREPASS));

                ctx.SetScissorAndViewports({ data.depthPassTarget });
                ctx.SetRenderTargets(
                    {},
                    true,
                    resources.GetResource(data.depthBufferObject));

                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
                ctx.SetGraphicsDescriptorTable(0, heap);
                ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                ctx.SetGraphicsConstantBufferView(3, cameraConstants);
                ctx.NullGraphicsConstantBufferView(6);

                TriMesh* prevMesh = nullptr;
                for (const auto& drawable : data.drawables)
                {
                    auto* const triMesh = GetMeshResource(drawable.D->MeshHandle);
                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(triMesh,
                            { VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                            });
                    }

                    auto test = drawable.D->GetConstants();
                    auto constants = ConstantBufferDataSet{ test, data.entityConstantsBuffer };
                    ctx.SetGraphicsConstantBufferView(2, constants);
                    ctx.DrawIndexedInstanced(triMesh->IndexCount);
                }
            });

        return pass;
    }


    /************************************************************************************************/


	BackgroundEnvironmentPass& WorldRender::BackgroundPass(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
		const CameraHandle              camera,
		const ResourceHandle            renderTarget,
		const ResourceHandle            hdrMap,
		ReserveConstantBufferFunction   reserveCB,
		ReserveVertexBufferFunction     reserveVB,
		iAllocator*                     allocator)
	{
		FK_ASSERT(0);

		auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
			BackgroundEnvironmentPass{},
			[&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
			{
				const size_t localBufferSize        = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
				auto& renderSystem                  = frameGraph.GetRenderSystem();

				data.renderTargetObject             = builder.WriteRenderTarget(renderTarget);
				data.passConstants                  = reserveCB(6 * KILOBYTE);
				data.passVertices                   = reserveVB(sizeof(float4) * 6);
				//data.diffuseMap                     = hdrMap;
			},
			[=](BackgroundEnvironmentPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& tempAllocator)
			{
				DescriptorHeap descHeap;
				descHeap.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 20, &tempAllocator);
				//descHeap.SetSRV(ctx, 6, data.diffuseMap);
				descHeap.NullFill(ctx, 20);

				auto& renderSystem          = frameResources.renderSystem();
				const auto WH               = frameResources.renderSystem().GetTextureWH(renderTarget);
				const auto cameraConstants  = GetCameraConstants(camera);

				struct
				{
					float4 Position;
				}   vertices[] =
				{
					float4(-1,   1,  1, 1),
					float4(1,    1,  1, 1),
					float4(-1,  -1,  1, 1),

					float4(-1,  -1,  1, 1),
					float4(1,    1,  1, 1),
					float4(1,   -1,  1, 1),
				};

				struct
				{
					float2 WH;
				}passConstants = { float2(WH[0], WH[1]) };

				ctx.SetRootSignature(frameResources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
				ctx.SetGraphicsDescriptorTable(4, descHeap);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets({ frameResources.GetResource({ data.renderTargetObject }) }, false, {});
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, 6, data.passVertices } });
				ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });

				ctx.Draw(6);
			});

		return pass;
	}


	/************************************************************************************************/


	BackgroundEnvironmentPass& WorldRender::RenderPBR_IBL_Deferred(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
		const SceneDescription&         sceneDescription,
		const CameraHandle              camera,
		const ResourceHandle            renderTarget,
		const ResourceHandle            depthTarget,
		GBuffer&                        gbuffer,
		ReserveConstantBufferFunction   reserveCB,
		ReserveVertexBufferFunction     reserveVB,
		float                           t,
		iAllocator*                     allocator)
	{
		auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
			BackgroundEnvironmentPass{},
			[&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
			{
				builder.AddDataDependency(sceneDescription.cameras);

				const size_t localBufferSize    = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
				auto& renderSystem              = frameGraph.GetRenderSystem();
				data.renderTargetObject         = builder.WriteRenderTarget(renderTarget);

				data.AlbedoTargetObject         = builder.ReadShaderResource(gbuffer.albedo);
				data.NormalTargetObject         = builder.ReadShaderResource(gbuffer.normal);
				data.TangentTargetObject        = builder.ReadShaderResource(gbuffer.tangent);
				data.MRIATargetObject           = builder.ReadShaderResource(gbuffer.MRIA);
				data.depthBufferTargetObject    = builder.ReadShaderResource(depthTarget);

				data.passConstants  = reserveCB(6 * KILOBYTE);
				data.passVertices   = reserveVB(sizeof(float4) * 6);
			},
			[=](BackgroundEnvironmentPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& allocator)
			{
				auto& renderSystem          = frameResources.renderSystem();
				const auto WH               = renderSystem.GetTextureWH(renderTarget);
				const auto cameraConstants  = GetCameraConstants(camera);

				struct
				{
					float4 Position;
				}   vertices[] =
				{
					float4(-1,   1,  1, 1),
					float4(1,    1,  1, 1),
					float4(-1,  -1,  1, 1),

					float4(-1,  -1,  1, 1),
					float4(1,    1,  1, 1),
					float4(1,   -1,  1, 1),
				};


				struct
				{
					float2 WH;
					float  t;
				}passConstants = { float2(WH[0], WH[1]), t };

				DescriptorHeap descHeap;
				descHeap.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 20, &allocator);

				descHeap.SetSRV(ctx, 0, frameResources.GetResource(data.AlbedoTargetObject));
				descHeap.SetSRV(ctx, 1, frameResources.GetResource(data.MRIATargetObject));
				descHeap.SetSRV(ctx, 2, frameResources.GetResource(data.NormalTargetObject));
				descHeap.SetSRV(ctx, 3, frameResources.GetResource(data.TangentTargetObject));
				descHeap.SetSRV(ctx, 4, frameResources.GetResource(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
				descHeap.NullFill(ctx, 20);

				ctx.SetRootSignature(renderSystem.Library.RSDefault);
				ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
				ctx.SetGraphicsDescriptorTable(3, descHeap);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets({ frameResources.GetResource(data.renderTargetObject) }, false);
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, 6, data.passVertices } });
				ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
				ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet{ passConstants, data.passConstants });

				ctx.Draw(6);
			});

		return pass;
	}


	/************************************************************************************************/


	BilateralBlurPass& WorldRender::BilateralBlur(
		FrameGraph&                     frameGraph,
		const ResourceHandle            source,
		const ResourceHandle            temp1,
		const ResourceHandle            temp2,
		const ResourceHandle            temp3,
		const ResourceHandle            destination,
		GBuffer&                        gbuffer,
		const ResourceHandle            depthBuffer,
		ReserveConstantBufferFunction   reserveCB,
		ReserveVertexBufferFunction     reserveVB,
		iAllocator*                     tempMemory)
	{
		auto& pass = frameGraph.AddNode<BilateralBlurPass>(
			BilateralBlurPass
			{
				reserveCB,
				reserveVB
			},
			[&](FrameGraphNodeBuilder& builder, BilateralBlurPass& data)
			{
				data.DestinationObject  = builder.WriteRenderTarget(destination);
				data.TempObject1        = builder.WriteRenderTarget(temp1);
				data.TempObject2        = builder.WriteRenderTarget(temp2); // 2 channel
				data.TempObject3        = builder.WriteRenderTarget(temp3); // 2 channel

				data.DepthSource        = builder.ReadShaderResource(depthBuffer);
				data.NormalSource       = builder.ReadShaderResource(gbuffer.normal);
				data.Source             = builder.ReadShaderResource(source);
			},
			[=](BilateralBlurPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& allocator)
			{
				auto& renderSystem          = frameResources.renderSystem();
				const auto WH               = frameResources.renderSystem().GetTextureWH(destination);

				auto constantBuffer = data.reserveCB(2048);
				auto vertexBuffer   = data.reserveVB(2048);

				struct
				{
					float4 Position;
				}   vertices[] =
				{
					float4(-1,   1,  1, 1),
					float4(1,    1,  1, 1),
					float4(-1,  -1,  1, 1),

					float4(-1,  -1,  1, 1),
					float4(1,    1,  1, 1),
					float4(1,   -1,  1, 1),
				};


				struct
				{
					float2 WH;
				}passConstants = { float2(WH[0], WH[1]) };

				DescriptorHeap descHeap;
				descHeap.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 5, &allocator);

				descHeap.SetSRV(ctx, 0, frameResources.GetResource(data.Source));
				descHeap.SetSRV(ctx, 1, frameResources.GetResource(data.NormalSource));
				descHeap.SetSRV(ctx, 2, frameResources.GetResource(data.DepthSource), DeviceFormat::R32_FLOAT);
				descHeap.NullFill(ctx, 3);

				ctx.SetRootSignature(frameResources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(frameResources.GetPipelineState(BILATERALBLURPASSHORIZONTAL));
				ctx.SetGraphicsDescriptorTable(3, descHeap);

				ctx.SetScissorAndViewports({ destination });
				ctx.SetRenderTargets({ frameResources.GetResource(data.TempObject1), frameResources.GetResource(data.TempObject2) }, false);
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, 6, vertexBuffer } });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, constantBuffer });

				ctx.Draw(6);

				DescriptorHeap descHeap2;
				descHeap2.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 5, &allocator);

				descHeap2.SetSRV(ctx, 0, frameResources.ReadRenderTarget(data.TempObject1, ctx));
				descHeap2.SetSRV(ctx, 1, frameResources.GetResource(data.NormalSource));
				descHeap2.SetSRV(ctx, 2, frameResources.GetResource(data.DepthSource), DeviceFormat::R32_FLOAT);
				descHeap2.SetSRV(ctx, 3, frameResources.ReadRenderTarget(data.TempObject2, ctx));

				ctx.SetPipelineState(frameResources.GetPipelineState(BILATERALBLURPASSVERTICAL));
				ctx.SetGraphicsDescriptorTable(3, descHeap2);
				ctx.SetRenderTargets({ frameResources.GetResource(data.DestinationObject) }, false);
				ctx.Draw(6);
			});

		return pass;
	}


	/************************************************************************************************/


	ForwardPlusPass& WorldRender::RenderPBR_ForwardPlus(
		UpdateDispatcher&			    dispatcher,
		FrameGraph&					    frameGraph, 
		const DepthPass&			    depthPass,
		const CameraHandle			    camera,
		const WorldRender_Targets&	    Targets,
		const SceneDescription&	        desc,
		const float                     t,
		ReserveConstantBufferFunction   reserveCB,
		ResourceHandle                  environmentMap,
		iAllocator*					    allocator)
	{
		const size_t MaxEntityDrawCount = 10000;

		typedef Vector<ForwardPlusPass> ForwardDrawableList;
		
        FK_ASSERT(0, "NOT FULLY IMPLEMENTED!");

		auto& pass = frameGraph.AddNode<ForwardPlusPass>(
			ForwardPlusPass{
				desc.lights.GetData().pointLights,
				depthPass.drawables,
				depthPass.entityConstantsBuffer },
			[&](FrameGraphNodeBuilder& builder, ForwardPlusPass& data)
			{
				builder.AddDataDependency(desc.PVS);
				builder.AddDataDependency(desc.cameras);

				data.BackBuffer			    = builder.WriteRenderTarget	(Targets.RenderTarget);
				data.DepthBuffer            = builder.WriteDepthBuffer (Targets.DepthTarget);
				size_t localBufferSize      = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

				data.passConstantsBuffer    = std::move(reserveCB(localBufferSize * 2));

				//data.lightLists			    = builder.ReadWriteUAV(lightLists,			DRS_ShaderResource);
				//data.pointLightBuffer	    = builder.ReadWriteUAV(pointLightBuffer,	DRS_ShaderResource);
			},
			[=](ForwardPlusPass& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };
				const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ (float)data.pointLights.size(), t, { 0, 0 } }, data.passConstantsBuffer };

				DescriptorHeap descHeap;
				descHeap.Init(
					ctx,
					resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
					&allocator);

				descHeap.SetSRV(ctx, 1, resources.GetResource(data.lightLists));
				descHeap.SetSRV(ctx, 2, resources.GetResource(data.pointLightBuffer));
				descHeap.NullFill(ctx);

				ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState(resources.GetPipelineState(FORWARDDRAW));

				// Setup Initial Shading State
				ctx.SetScissorAndViewports({Targets.RenderTarget});
				ctx.SetRenderTargets(
					{ resources.GetResource(data.BackBuffer) },
					true,
					resources.GetResource(data.DepthBuffer));

				ctx.SetPrimitiveTopology			(EInputTopology::EIT_TRIANGLE);
				ctx.SetGraphicsDescriptorTable		(0, descHeap);
				ctx.SetGraphicsConstantBufferView	(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView	(3, passConstants);
				ctx.NullGraphicsConstantBufferView	(6);

				TriMesh* prevMesh = nullptr;

				for (size_t itr = 0; itr < data.drawables.size(); ++itr)
				{
					auto& drawable = data.drawables[itr];
					TriMesh* triMesh = GetMeshResource(drawable.D->MeshHandle);

					if (triMesh != prevMesh)
					{
						prevMesh = triMesh;

						ctx.AddIndexBuffer(triMesh);
						ctx.AddVertexBuffers(triMesh,
							{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							});
					}

					auto proxy = CreateCBIterator<decltype(drawable.D->GetConstants())>(data.entityConstants)[itr];

					ctx.SetGraphicsConstantBufferView(2, proxy);
					ctx.DrawIndexedInstanced(triMesh->IndexCount, 0, 0, 1, 0);
				}
			});

		return pass;
	}


	/************************************************************************************************/

    struct alignas(16) BVH_Node
    {
        float4 MinPoint;
        float4 MaxPoint;

        uint32_t Offset;
        uint32_t Count; // max child count is 16

        uint32_t Leaf;
    };

    constexpr size_t sizeofBVH_Node     = sizeof(BVH_Node);
    constexpr size_t BVH_ELEMENT_COUNT  = 32;

	LightBufferUpdate& WorldRender::UpdateLightBuffers(
		UpdateDispatcher&		        dispatcher,
		FrameGraph&				        graph,
		const CameraHandle	            camera,
		const GraphicScene&	            scene,
		const SceneDescription&         sceneDescription,
        ResourceHandle                  depthBuffer,
		ReserveConstantBufferFunction   reserveCB,
		iAllocator*				        tempMemory)

    {
        auto WH = graph.GetRenderSystem().GetTextureWH(depthBuffer);

		auto& lightBufferData = graph.AddNode<LightBufferUpdate>(
			LightBufferUpdate{
					&sceneDescription.lights.GetData().pointLights,
					camera,
					reserveCB,
                    createClusterLightListLayout
			},
			[&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
			{
				auto& renderSystem          = graph.GetRenderSystem();

                // Output Objects
                data.clusterBufferObject	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1 * MEGABYTE), DRS_UAV, false);
                data.indexBufferObject      = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture({ WH }, DeviceFormat::R32_UINT), DRS_UAV, false);
				data.lightListObject	    = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE), DRS_UAV, false);
                data.lightBufferObject      = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE), DRS_ShaderResource, false);

                // Temporaries
                data.lightBVH               = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE), DRS_UAV);
                data.lightLookupObject      = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE), DRS_UAV);
                data.lightCounterObject     = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);

                data.counterObject          = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);

                data.depthBufferObject      = builder.ReadRenderTarget(depthBuffer);
                data.argumentBufferObject   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV);

				data.camera				    = camera;


                builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV, true);

                builder.SetDebugName(data.clusterBufferObject, "ClusterBufferObject");
                builder.SetDebugName(data.indexBufferObject, "indexBufferObject");
                builder.SetDebugName(data.lightListObject, "lightListObject");
                builder.SetDebugName(data.lightBufferObject, "lightBufferObject");
                builder.SetDebugName(data.lightBVH, "lightBVH");
                builder.SetDebugName(data.lightLookupObject, "lightLookupObject");
                builder.SetDebugName(data.lightCounterObject, "lightCounterObject");
                builder.SetDebugName(data.counterObject, "counterObject");
                builder.SetDebugName(data.argumentBufferObject, "argumentBufferObject");

				builder.AddDataDependency(sceneDescription.lights);
				builder.AddDataDependency(sceneDescription.cameras);
			},
			[timeStats = timeStats](LightBufferUpdate& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
                auto        CreateClusters              = resources.GetPipelineState(CREATECLUSTERS);
                auto        ClearCounters               = resources.GetPipelineState(CLEARCOUNTERSPSO);
                auto        CreateBVH_Phase1            = resources.GetPipelineState(CREATELIGHTBVH_PHASE1);
                auto        CreateBVH_Phase2            = resources.GetPipelineState(CREATELIGHTBVH_PHASE2);
                auto        CreateLightLists            = resources.GetPipelineState(CREATECLUSTERLIGHTLISTS);
                auto        CreateLightListArguments    = resources.GetPipelineState(CREATELIGHTLISTARGS_PSO);

                const auto  cameraConstants     = GetCameraConstants(data.camera);
                const auto  lightCount          = data.pointLightHandles->size();

				struct ConstantsLayout
				{
					float4x4 iproj;
					float4x4 view;
					uint2    LightMapWidthHeight;
					uint32_t lightCount;

                    uint32_t nodeCount;
                    uint32_t nodeOffset;
                }constantsValues = {
                    XMMatrixToFloat4x4(DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(cameraConstants.Proj))),
					cameraConstants.View,
                    { 0, 0 },
					(uint32_t)lightCount
				};

				PointLightComponent& pointLights = PointLightComponent::GetComponent();

                uint32_t temp = ceilf(std::logf(float(lightCount)) / std::logf(BVH_ELEMENT_COUNT));

				CBPushBuffer    constantBuffer = data.reserveCB(
					AlignedSize( sizeof(FlexKit::GPUPointLight) * data.pointLightHandles->size() ) +
                    AlignedSize<ConstantsLayout>() * (1 + ceilf(std::logf(float(lightCount)) / std::logf(BVH_ELEMENT_COUNT))) +
                    AlignedSize<Camera::ConstantBuffer>()
				);

                CBPushBuffer    constantBuffer2 = data.reserveCB(AlignedSize<ConstantsLayout>() * (1 + ceilf(std::logf(float(Max(lightCount, 1))) / std::logf(BVH_ELEMENT_COUNT))));

                const ConstantBufferDataSet constants{ constantsValues, constantBuffer };
                const ConstantBufferDataSet cameraConstantsBuffer{ GetCameraConstants(data.camera), constantBuffer };

				Vector<FlexKit::GPUPointLight> pointLightValues{ &allocator };

				for (const auto light : *data.pointLightHandles)
				{
					const PointLight pointLight	= pointLights[light];
					const float3 WS_position	= GetPositionW(pointLight.Position);
                    const float3 VS_position    = (constantsValues.view * float4(WS_position, 1)).xyz();

					pointLightValues.push_back(
						{	{ pointLight.K, pointLight.I	},
							{ WS_position, 32        } });
				}

				const size_t uploadSize = pointLightValues.size() * sizeof(GPUPointLight);
				const ConstantBufferDataSet pointLights_GPU{ (char*)pointLightValues.data(), uploadSize, constantBuffer };

				ctx.CopyBufferRegion(
					{ resources.GetObjectResource(pointLights_GPU.Handle()) },
					{ pointLights_GPU.Offset() },
					{ resources.GetObjectResource(resources.WriteUAV(data.lightBufferObject, ctx)) },
					{ 0 },
					{ uploadSize },
					{ DRS_Write },
					{ DRS_Write }
				);

                ctx.SetComputeRootSignature(resources.renderSystem().Library.ComputeSignature);

                DescriptorHeap clearHeap;
                clearHeap.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                clearHeap.SetUAVBuffer(ctx, 2, resources.ReadWriteUAV(data.counterObject, ctx));
                clearHeap.NullFill(ctx);


                ctx.SetComputeDescriptorTable(0, clearHeap);
                ctx.Dispatch(ClearCounters, uint3{ 1, 1, 1 });

				DescriptorHeap clusterCreationResources;
                clusterCreationResources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);

                // UAVs start at 0
                clusterCreationResources.SetUAVStructured(ctx, 0, resources.ReadWriteUAV(data.clusterBufferObject, ctx), resources.ReadWriteUAV(data.counterObject, ctx), sizeof(GPUCluster), 0);
                clusterCreationResources.SetUAVTexture(ctx, 1, resources.ReadWriteUAV(data.indexBufferObject, ctx));

                // SRV's start at 4
                clusterCreationResources.SetSRV(ctx, 4, resources.ReadRenderTarget(data.depthBufferObject, ctx), DeviceFormat::R32_FLOAT);

                // CBV's start at 8
                clusterCreationResources.SetCBV(ctx, 8, cameraConstantsBuffer);
                clusterCreationResources.SetCBV(ctx, 9, constants);
                clusterCreationResources.NullFill(ctx);

                ctx.TimeStamp(timeStats, 2);

                const auto threadsWH = resources.GetTextureWH(data.depthBufferObject) / 32;
                ctx.SetComputeDescriptorTable(0, clusterCreationResources);
                ctx.Dispatch(CreateClusters, uint3{ threadsWH[0], threadsWH[1] + 1, 1 });

                ctx.TimeStamp(timeStats, 3);

                // Create light BVH
                // TODO: allow for more than 1024 point lights
                //          move sorting of lights to a separate pass
                //          then pull sorted light list into the two pass BVH construct
                DescriptorHeap BVH_Phase1_Resources;
                BVH_Phase1_Resources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                BVH_Phase1_Resources.SetStructuredResource(ctx, 4, resources.ReadUAV(data.lightBufferObject, DRS_ShaderResource, ctx), sizeof(GPUPointLight));
                BVH_Phase1_Resources.SetUAVStructured(ctx, 0, resources.ReadWriteUAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);
                BVH_Phase1_Resources.SetUAVStructured(ctx, 1, resources.ReadWriteUAV(data.lightLookupObject, ctx), sizeof(uint2), 0);
                BVH_Phase1_Resources.SetCBV(ctx, 8, constants);

                ctx.TimeStamp(timeStats, 6);
                ctx.SetComputeDescriptorTable(0, BVH_Phase1_Resources);
                ctx.Dispatch(CreateBVH_Phase1, { 1, 1, 1 }); // Two dispatches to sync across calls

                ctx.AddUAVBarrier(resources.GetResource(data.lightLookupObject));
                ctx.AddUAVBarrier(resources.GetResource(data.lightBVH));

                auto Phase2_Pass =
                    [&](const uint32_t nodeOffset, const uint32_t nodeCount)
                    {

                        struct ConstantsLayout
				        {
					        float4x4 iproj;
					        float4x4 view;
					        uint2    LightMapWidthHeight;
					        uint32_t lightCount;

                            uint32_t nodeCount;
                            uint32_t nodeOffset;
                        }constantValues = {
                            XMMatrixToFloat4x4(DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(cameraConstants.Proj))),
					        cameraConstants.View,
                            { 0, 0 },
					        (uint32_t)lightCount,
                            nodeCount,
                            nodeOffset
				        };

                        const ConstantBufferDataSet constantSet{ constantValues, constantBuffer2 };


                        DescriptorHeap BVH_Phase2_Resources;
                        BVH_Phase2_Resources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                        BVH_Phase2_Resources.SetUAVStructured(ctx, 0, resources.ReadWriteUAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);
                        BVH_Phase2_Resources.SetStructuredResource(ctx, 4, resources.ReadUAV(data.lightBufferObject, DRS_ShaderResource, ctx), sizeof(GPUPointLight));
                        BVH_Phase2_Resources.SetCBV(ctx, 8, constantSet);

                        ctx.SetComputeDescriptorTable(0, BVH_Phase2_Resources);
                        ctx.Dispatch(CreateBVH_Phase2, { 1, 1, 1 });

                        ctx.AddUAVBarrier(resources.GetResource(data.lightBVH));
                    };

                size_t offset       = 0;
                size_t nodeCount    = std::ceilf(float(lightCount) / BVH_ELEMENT_COUNT);
                const uint32_t passCount = std::floor(std::log(lightCount) / std::log(BVH_ELEMENT_COUNT));

                for (uint32_t I = 0; I < passCount; I++)
                {
                    Phase2_Pass(offset, nodeCount);
                    offset      += nodeCount;
                    nodeCount    = std::ceilf(float(nodeCount) / BVH_ELEMENT_COUNT);
                }


                DescriptorHeap createArgumentResources;
                createArgumentResources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                createArgumentResources.SetUAVStructured(ctx, 0, resources.ReadWriteUAV(data.argumentBufferObject, ctx), sizeof(uint4), 0);
                createArgumentResources.SetUAVStructured(ctx, 1, resources.ReadWriteUAV(data.lightCounterObject, ctx), sizeof(uint32_t), 0);
                createArgumentResources.SetStructuredResource(ctx, 4, resources.ReadUAV(data.counterObject, DRS_ShaderResource, ctx), sizeof(uint32_t));
                createArgumentResources.NullFill(ctx);

                ctx.SetComputeDescriptorTable(0, createArgumentResources);
                ctx.Dispatch(CreateLightListArguments, { 1, 1, 1 }); // Two dispatches to sync across calls

                ctx.AddUAVBarrier(resources.GetResource(data.counterObject));
                ctx.AddUAVBarrier(resources.GetResource(data.lightCounterObject));
                ctx.AddUAVBarrier(resources.GetResource(data.argumentBufferObject));

                struct LightListConstructionConstants
                {
                    float4x4 View;
                    uint32_t rootNode;
                } lightListConstants {
                    cameraConstants.View,
                    offset
                };

                // Build Light Lists
                CBPushBuffer constantBuffer3 = data.reserveCB(AlignedSize<LightListConstructionConstants>());
                ConstantBufferDataSet   lightListConstantSet{ lightListConstants, constantBuffer3 };


                DescriptorHeap createLightListResources;
                createLightListResources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                createLightListResources.SetUAVStructured(ctx, 0, resources.ReadWriteUAV(data.clusterBufferObject, ctx), sizeof(GPUCluster), 0);
                createLightListResources.SetUAVStructured(ctx, 1, resources.ReadWriteUAV(data.lightListObject, ctx), sizeof(uint32_t), 0);
                createLightListResources.SetUAVStructured(ctx, 2, resources.ReadWriteUAV(data.lightCounterObject, ctx), sizeof(uint32_t), 0);
                createLightListResources.SetUAVStructured(ctx, 3, resources.ReadWriteUAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);

                createLightListResources.SetStructuredResource(ctx, 5, resources.ReadUAV(data.lightLookupObject, DRS_ShaderResource, ctx), sizeof(uint32_t),0);
                createLightListResources.SetStructuredResource(ctx, 6, resources.ReadUAV(data.lightBufferObject, DRS_ShaderResource, ctx), sizeof(GPUPointLight));
                createLightListResources.SetCBV(ctx, 8, resources.ReadUAV(data.counterObject, DRS_ShaderResource, ctx), 0, 4);
                createLightListResources.SetCBV(ctx, 9, lightListConstantSet);
                createLightListResources.NullFill(ctx);

                ctx.SetComputeDescriptorTable(0, createLightListResources);
                ctx.SetPipelineState(CreateLightLists);
                ctx.ExecuteIndirect(resources.ReadIndirectArgs(data.argumentBufferObject, ctx), data.indirectLayout);

                ctx.TimeStamp(timeStats, 7);
			});

		return lightBufferData;
	}


    /************************************************************************************************/


    DEBUGVIS_DrawBVH& WorldRender::DEBUGVIS_DrawLightBVH(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
		const CameraHandle              camera,
        ResourceHandle                  renderTarget,
        LightBufferUpdate&              lightBufferUpdate,
		ReserveConstantBufferFunction   reserveCB,
		iAllocator*                     tempMemory)
    {
        auto& lightBufferData = frameGraph.AddNode<DEBUGVIS_DrawBVH>(
            DEBUGVIS_DrawBVH{
                reserveCB,
                lightBufferUpdate
            },
            [&, this](FrameGraphNodeBuilder& builder, DEBUGVIS_DrawBVH& data)
            {
                data.lightBVH       = builder.ReadResource(lightBufferUpdate.lightBVH, DRS_ShaderResource);
                data.clusters       = builder.ReadResource(lightBufferUpdate.clusterBufferObject, DRS_ShaderResource);
                data.renderTarget   = builder.WriteRenderTarget(renderTarget);
                data.indirectArgs   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV, true);
                data.counterBuffer  = builder.ReadResource(lightBufferUpdate.counterObject, DRS_ShaderResource);
                data.pointLights    = builder.ReadResource(lightBufferUpdate.lightBufferObject, DRS_ShaderResource);
                data.camera         = camera;
            },
            [&](DEBUGVIS_DrawBVH& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                auto debugBVHVISPSO         = resources.GetPipelineState(LIGHTBVH_DEBUGVIS_PSO);
                auto debugClusterVISPSO     = resources.GetPipelineState(CLUSTER_DEBUGVIS_PSO);
                auto debugClusterArgsVISPSO = resources.GetPipelineState(CLUSTER_DEBUGARGSVIS_PSO);
                auto debugLightVISPSO       = resources.GetPipelineState(CREATELIGHTDEBUGVIS_PSO);

                const auto cameraConstants = GetCameraConstants(data.camera);

                ctx.SetRenderTargets({ resources.GetRenderTarget(data.renderTarget) }, false);
                ctx.SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });

                ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
                ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);

                const auto lightCount = data.lightBufferUpdateData.pointLightHandles->size();

                struct ConstantsLayout
                {
                    float4x4 proj;
                    float4x4 View;
                    float4x4 IView;
                    uint2    LightMapWidthHeight;
                    uint32_t lightCount;
                }constantsValues = {
                    cameraConstants.Proj,
                    cameraConstants.View,
                    cameraConstants.ViewI,
                    { 0, 0 },
                    (uint32_t)lightCount
                };

                CBPushBuffer            constantBuffer = data.reserveCB(1024);
                ConstantBufferDataSet   constants{ constantsValues, constantBuffer };

                [&]()
                {
                    DescriptorHeap descHeap;
                    descHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 2, &allocator);
                    descHeap.SetUAVStructured(ctx, 0, resources.GetResource(data.counterBuffer), sizeof(uint32_t), 0);
                    descHeap.NullFill(ctx, 2);

                    DescriptorHeap UAVHeap;
                    UAVHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 2, &allocator);
                    UAVHeap.SetUAVStructured(ctx, 0, resources.GetResource(data.indirectArgs), sizeof(uint32_t[4]), 0);
                    UAVHeap.NullFill(ctx, 2);

                    ctx.SetComputeDescriptorTable(3, descHeap);
                    ctx.SetComputeDescriptorTable(4, UAVHeap);

                    ctx.Dispatch(debugClusterArgsVISPSO, { 1, 1, 1 });
                }();

                DescriptorHeap descHeap;
                descHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 3, &allocator);
                descHeap.SetStructuredResource(ctx, 0, resources.ReadUAV(data.lightBVH, DRS_ShaderResource, ctx), 4, 0);
                descHeap.SetStructuredResource(ctx, 1, resources.ReadUAV(data.clusters, DRS_ShaderResource, ctx), sizeof(GPUCluster), 0);
                descHeap.SetStructuredResource(ctx, 2, resources.GetResource(data.pointLights), sizeof(float4[2]), 0);
                descHeap.NullFill(ctx, 2);

                DescriptorHeap nullHeap;
                nullHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 2, &allocator);
                nullHeap.NullFill(ctx, 2);

                ctx.SetGraphicsConstantBufferView(0, constants);
                ctx.SetGraphicsConstantBufferView(1, constants);
                ctx.SetGraphicsConstantBufferView(2, constants);

                ctx.SetPrimitiveTopology(EIT_POINT);
                ctx.SetGraphicsDescriptorTable(3, descHeap);
                ctx.SetGraphicsDescriptorTable(4, nullHeap);


                size_t offset       = 0;
                size_t nodeCount    = std::ceilf(float(lightCount) / BVH_ELEMENT_COUNT);

                const uint32_t passCount = std::floor(std::log(lightCount) / std::log(BVH_ELEMENT_COUNT));

                for (uint32_t I = 0; I < passCount; I++)
                {
                    offset      += nodeCount;
                    nodeCount    = std::floor(float(nodeCount) / BVH_ELEMENT_COUNT);
                }

                ctx.SetPipelineState(debugBVHVISPSO);
                ctx.Draw(offset + 1);

                ctx.SetPipelineState(debugClusterVISPSO);
                ctx.ExecuteIndirect(resources.ReadUAV(data.indirectArgs, DRS_INDIRECTARGS, ctx), createDebugDrawLayout);

                ctx.SetPipelineState(debugLightVISPSO);
                ctx.Draw(lightCount);
            });

        return lightBufferData;
    }


	/************************************************************************************************/


	GBufferPass& WorldRender::RenderPBR_GBufferPass(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
		const SceneDescription&         sceneDescription,
		const CameraHandle              camera,
		GBuffer&                        gbuffer,
		ResourceHandle                  depthTarget,
		ReserveConstantBufferFunction   reserveCB,
		iAllocator*                     allocator)
	{
		auto& pass = frameGraph.AddNode<GBufferPass>(
			GBufferPass{
				gbuffer,
				sceneDescription.PVS.GetData().solid,
				sceneDescription.skinned.GetData().skinned,
				reserveCB,
			},
			[&](FrameGraphNodeBuilder& builder, GBufferPass& data)
			{
				builder.AddDataDependency(sceneDescription.cameras);
				builder.AddDataDependency(sceneDescription.PVS);
				builder.AddDataDependency(sceneDescription.skinned);

				data.AlbedoTargetObject      = builder.WriteRenderTarget(gbuffer.albedo);
				data.MRIATargetObject        = builder.WriteRenderTarget(gbuffer.MRIA);
				data.NormalTargetObject      = builder.WriteRenderTarget(gbuffer.normal);
				data.TangentTargetObject     = builder.WriteRenderTarget(gbuffer.tangent);
				data.depthBufferTargetObject = builder.WriteDepthBuffer(depthTarget);
			},
			[camera, timeStats = timeStats](GBufferPass& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
                ctx.TimeStamp(timeStats, 0);

				struct EntityPoses
				{
					float4x4 transforms[128];

					auto& operator [](size_t idx)
					{
						return transforms[idx];
					}
				};

				if (!data.pvs.size())
					return;

				const size_t entityBufferSize =
					AlignedSize<Drawable::VConstantsLayout>() * data.pvs.size() * 30;

				constexpr size_t passBufferSize =
					AlignedSize<Camera::ConstantBuffer>() +
					AlignedSize<ForwardDrawConstants>();

				const size_t poseBufferSize =
					AlignedSize<EntityPoses>() * data.skinned.size();

				auto passConstantBuffer   = data.reserveCB(passBufferSize);
				auto entityConstantBuffer = data.reserveCB(entityBufferSize);
				auto poseBuffer           = data.reserveCB(poseBufferSize);

				const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), passConstantBuffer };
				const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ 1, 1 }, passConstantBuffer };

				ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS));
				ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

				// Setup pipeline resources
				SetScissorAndViewports(
                    ctx,
					std::tuple{
						data.gbuffer.albedo,
						data.gbuffer.MRIA,
						data.gbuffer.normal,
						data.gbuffer.tangent,
					});

				RenderTargetList renderTargets = {
					resources.GetResource(data.AlbedoTargetObject),
					resources.GetResource(data.MRIATargetObject),
					resources.GetResource(data.NormalTargetObject),
					resources.GetResource(data.TangentTargetObject),
				};

				ctx.SetRenderTargets(
					renderTargets,
					true,
					resources.GetResource(data.depthBufferTargetObject));

				// Setup Constants
				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(3, passConstants);

				// submit draw calls
				TriMesh* prevMesh = nullptr;

				// unskinned models
				for (auto& drawable : data.pvs)
				{

					auto constants    = drawable.D->GetConstants();
					auto* triMesh     = GetMeshResource(drawable.D->MeshHandle);

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
							}
						);
					}

                    auto& materials = MaterialComponent::GetComponent();
                    const auto material  = MaterialComponent::GetComponent()[drawable.D->material];
                    const auto subMeshCount = triMesh->subMeshes.size();

                    if (material.SubMaterials.empty())
                    {
                        DescriptorHeap descHeap;

					    descHeap.Init(
						    ctx,
						    resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
						    &allocator);

					    if(material.Textures.size())
						    for(size_t I = 0; I < Min(material.Textures.size(), 4); I++)
							    descHeap.SetSRV(ctx, I, material.Textures[I]);
					    else
					    {
						    descHeap.SetSRV(ctx, 0, resources.renderSystem().DefaultTexture);
						    descHeap.SetSRV(ctx, 1, resources.renderSystem().DefaultTexture);
						    descHeap.SetSRV(ctx, 2, resources.renderSystem().DefaultTexture);
						    descHeap.SetSRV(ctx, 3, resources.renderSystem().DefaultTexture);
					    }

					    descHeap.NullFill(ctx);
                        constants.textureCount = material.Textures.size();

                        ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                        ctx.SetGraphicsDescriptorTable(0, descHeap);
                        ctx.DrawIndexed(triMesh->IndexCount);
                    }
                    else
                    {
                        for (size_t I = 0; I < subMeshCount; I++)
                        {
                            auto& subMesh           = triMesh->subMeshes[I];
                            const auto passMaterial = materials[material.SubMaterials[I]];

                            DescriptorHeap descHeap;

					        descHeap.Init(
						        ctx,
						        resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
						        &allocator);

					        if(passMaterial.Textures.size())
						        for(size_t I = 0; I < Min(passMaterial.Textures.size(), 4); I++)
							        descHeap.SetSRV(ctx, I, passMaterial.Textures[I]);
					        else
					        {
						        descHeap.SetSRV(ctx, 0, resources.renderSystem().DefaultTexture);
						        descHeap.SetSRV(ctx, 1, resources.renderSystem().DefaultTexture);
						        descHeap.SetSRV(ctx, 2, resources.renderSystem().DefaultTexture);
						        descHeap.SetSRV(ctx, 3, resources.renderSystem().DefaultTexture);
					        }

					        descHeap.NullFill(ctx);
                            constants.textureCount = passMaterial.Textures.size();

                            ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                            ctx.SetGraphicsDescriptorTable(0, descHeap);
                            ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
                        }
                    }
				}

				// skinned models
				ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS_SKINNED));

				auto& poses = allocator.allocate<EntityPoses>();

				for (const auto& skinnedDraw : data.skinned)
				{
					const auto constants    = skinnedDraw.drawable->GetConstants();
					auto* triMesh           = GetMeshResource(skinnedDraw.drawable->MeshHandle);

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

					auto pose       = skinnedDraw.pose;
					auto skeleton   = pose->Sk;

					for(size_t I = 0; I < pose->JointCount; I++)
						poses[I] = XMMatrixToFloat4x4(skeleton->IPose[I] * pose->CurrentPose[I]);

					ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
					ctx.SetGraphicsConstantBufferView(4, ConstantBufferDataSet(poses, poseBuffer));

                    for (auto& subMesh : triMesh->subMeshes)
                    {
                        ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
                    }
				}
                ctx.TimeStamp(timeStats, 1);
			}
			);

		return pass;
	}



	/************************************************************************************************/


	TiledDeferredShade& WorldRender::RenderPBR_DeferredShade(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
		const SceneDescription&         sceneDescription,
		PointLightGatherTask&           gather,
		GBuffer&                        gbuffer,
		ResourceHandle                  depthTarget,
		ResourceHandle                  renderTarget,
		ShadowMapPassData&              shadowMaps,
		LightBufferUpdate&              lightPass,
		ReserveConstantBufferFunction   reserveCB,
		ReserveVertexBufferFunction     reserveVB,
		float                           t,
		iAllocator*                     allocator)
	{
		struct PointLight
		{
			float4 KI;	// Color + intensity in W
			float4 PR;	// XYZ + radius in W
		};

		auto& pass = frameGraph.AddNode<TiledDeferredShade>(
			TiledDeferredShade{
				gbuffer,
				gather,
				lightPass,
				sceneDescription.pointLightMaps,
				shadowMaps
			},
			[&](FrameGraphNodeBuilder& builder, TiledDeferredShade& data)
			{
				builder.AddDataDependency(sceneDescription.cameras);
				builder.AddDataDependency(sceneDescription.pointLightMaps);

				auto& renderSystem              = frameGraph.GetRenderSystem();

				data.pointLights                = Vector<FlexKit::GPUPointLight>{ allocator, 1024 };
				data.pointLightHandles          = &sceneDescription.lights.GetData().pointLights;

				data.AlbedoTargetObject         = builder.ReadShaderResource(gbuffer.albedo);
				data.NormalTargetObject         = builder.ReadShaderResource(gbuffer.normal);
				data.TangentTargetObject        = builder.ReadShaderResource(gbuffer.tangent);
				data.MRIATargetObject           = builder.ReadShaderResource(gbuffer.MRIA);
				data.depthBufferTargetObject    = builder.ReadShaderResource(depthTarget);
                data.clusterIndexBufferObject   = builder.ReadResource(lightPass.indexBufferObject, DRS_ShaderResource);
                data.clusterBufferObject        = builder.ReadResource(lightPass.clusterBufferObject, DRS_ShaderResource);
                data.lightListsObject           = builder.ReadResource(lightPass.lightListObject, DRS_ShaderResource);

				data.renderTargetObject         = builder.WriteRenderTarget(renderTarget);
				data.pointLightBufferObject     = builder.ReadResource(lightPass.lightBufferObject, DRS_ShaderResource);

				data.passConstants              = reserveCB(128 * KILOBYTE);
				data.passVertices               = reserveVB(sizeof(float4) * 6);

                for (auto shadowMap : shadowMaps.shadowMapTargets) {
                    builder.ReadResource(shadowMap, DRS_ShaderResource);
                    builder.ReleaseVirtualResource(shadowMap);
                }

                builder.ReleaseVirtualResource(lightPass.clusterBufferObject);
                builder.ReleaseVirtualResource(lightPass.indexBufferObject);
                builder.ReleaseVirtualResource(lightPass.lightListObject);
                builder.ReleaseVirtualResource(lightPass.lightBufferObject);
			},
			[camera = sceneDescription.camera, renderTarget, t, timingReadBack = timingReadBack, timeStats = timeStats]
			(TiledDeferredShade& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				PointLightComponent& pointLights = PointLightComponent::GetComponent();

				for (const auto light : *data.pointLightHandles)
				{
					const auto  pointLight	= pointLights[light];
					float3      position	= GetPositionW(pointLight.Position);

					data.pointLights.push_back(
						{	{ pointLight.K, pointLight.I	},
							{ position,     50    } });
				}

				auto& renderSystem          = resources.renderSystem();
				const auto WH               = resources.renderSystem().GetTextureWH(renderTarget);
				const auto cameraConstants  = GetCameraConstants(camera);

				const auto pointLightCount = pointLights.elements.size();

				struct
				{
					float2      WH;
					float       time;
					uint32_t    lightCount;
                    float4      ambientLight;
					float4x4    PV[1024];
				}passConstants =
                {
                    {(float)WH[0], (float)WH[1]},
                    t,
                    pointLightCount,
                    { 0.1f, 0.1f, 0.1f, 0 }
                };

				for(size_t I = 0; I < pointLightCount; I++)
				{
					auto& light                      = pointLights.elements[I].componentData;
					const float3 Position            = GetPositionW(light.Position);

					const auto matrices = CalculateShadowMapMatrices(Position, light.R, t);

					for(size_t II = 0; II < 6; II++)
						passConstants.PV[6 * I + II] = matrices.PV[II];
				}

				struct
				{
					float4 Position;
				}   vertices[] =
				{
					float4(-1,   1, 0, 1),
					float4( 1,   1, 0, 1),
					float4(-1,  -1, 0, 1),

					float4(-1,  -1, 0, 1),
					float4( 1,   1, 0, 1),
					float4( 1,  -1, 0, 1),
				};


				const size_t descriptorTableSize = 20 + data.shadowMaps.shadowMapTargets.size() * 6;

				DescriptorHeap descHeap;
				descHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), descriptorTableSize, &allocator);
				descHeap.SetSRV(ctx, 0, resources.GetResource(data.AlbedoTargetObject));
				descHeap.SetSRV(ctx, 1, resources.GetResource(data.MRIATargetObject));
				descHeap.SetSRV(ctx, 2, resources.GetResource(data.NormalTargetObject));
				descHeap.SetSRV(ctx, 3, resources.GetResource(data.TangentTargetObject));
				descHeap.SetSRV(ctx, 4, resources.GetResource(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
                descHeap.SetSRV(ctx, 5, resources.GetResource(data.clusterIndexBufferObject));
                descHeap.SetStructuredResource(ctx, 6, resources.GetResource(data.clusterBufferObject), sizeof(GPUCluster));
                descHeap.SetStructuredResource(ctx, 7, resources.GetResource(data.lightListsObject), sizeof(uint32_t));

				descHeap.SetStructuredResource(ctx, 9, resources.ReadUAV(data.pointLightBufferObject, DRS_ShaderResource, ctx), sizeof(GPUPointLight));

				for (size_t shadowMapIdx = 0; shadowMapIdx < pointLightCount; shadowMapIdx++)
				{
					auto shadowMap = resources.GetResource(data.shadowMaps.shadowMapTargets[shadowMapIdx]);
					descHeap.SetSRVCubemap(ctx, 10 + shadowMapIdx, shadowMap, DeviceFormat::R32_FLOAT);
				}

				descHeap.NullFill(ctx, descriptorTableSize);

				ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(resources.GetPipelineState(SHADINGPASS));
				ctx.SetPrimitiveTopology(EIT_TRIANGLELIST);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets({ resources.GetResource({ data.renderTargetObject })}, false, {});
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
				ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
				ctx.SetGraphicsDescriptorTable(3, descHeap);

                ctx.TimeStamp(timeStats, 4);
				ctx.Draw(6);
                ctx.TimeStamp(timeStats, 5);

                ctx.ResolveQuery(timeStats, 0, 8, resources.GetObjectResource(timingReadBack), 0);
                ctx.QueueReadBack(timingReadBack);
			});

		return pass;
	}


	/************************************************************************************************/


	ComputeTiledPass& WorldRender::RenderPBR_ComputeDeferredTiledShade(
			UpdateDispatcher&                       dispatcher,
			FrameGraph&                             frameGraph,
			ReserveConstantBufferFunction&          constantBufferAllocator,
			const ComputeTiledDeferredShadeDesc&    scene)
	{
        FK_ASSERT(0);
		//frameGraph.Resources.AddUAVResource(Buffer, frameGraph.GetRenderSystem().GetObjectState(tempBuffer));

		auto& pass = frameGraph.AddNode<ComputeTiledPass>(
			ComputeTiledPass{
				constantBufferAllocator,
				scene.pointLightGather
			},
			[&](FrameGraphNodeBuilder& builder, ComputeTiledPass& data)
			{
				data.dispatchDims   = { 1, 1, 1 };
				data.activeCamera   = scene.activeCamera;
				data.WH             = 1 * 10;
				// Inputs
				data.albedoObject         = builder.ReadShaderResource(scene.gbuffer.albedo);
				data.MRIAObject           = builder.ReadShaderResource(scene.gbuffer.MRIA);
				data.normalObject         = builder.ReadShaderResource(scene.gbuffer.normal);
				data.tangentObject        = builder.ReadShaderResource(scene.gbuffer.tangent);
				data.depthBufferObject    = builder.ReadShaderResource(scene.depthTarget);

				//data.lightBitBucketObject   = builder.ReadWriteUAV(lightLists,          DRS_ShaderResource);
				//data.lightBuffer            = builder.ReadWriteUAV(pointLightBuffer,    DRS_ShaderResource);

				// Ouputs
				//data.tempBufferObject   = builder.ReadWriteUAV(tempBuffer);
				data.renderTargetObject = builder.WriteRenderTarget(scene.renderTarget);
			},
			[=]
			(ComputeTiledPass& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				CBPushBuffer pushBuffer{ data.constantBufferAllocator(2048) };

				ConstantBufferDataSet cameraConstants{
					CameraComponent::GetComponent().GetCamera(data.activeCamera).GetConstants(),
					pushBuffer
				};

				struct LocalPassConstants
				{
					uint32_t    lightCount;
					uint2       WH;
				};

				ConstantBufferDataSet localConstants{
					LocalPassConstants{
						(uint32_t)data.pointLights.GetData().pointLights.size(),
						data.WH
					},
					pushBuffer
				};

				PointLightComponent& pointLights = PointLightComponent::GetComponent();
				DescriptorHeap srvHeap;
				srvHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 7, &allocator);
				srvHeap.SetSRV(ctx, 0, resources.GetResource(data.albedoObject));
				srvHeap.SetSRV(ctx, 1, resources.GetResource(data.MRIAObject));
				srvHeap.SetSRV(ctx, 2, resources.GetResource(data.normalObject));
				srvHeap.SetSRV(ctx, 3, resources.GetResource(data.tangentObject));
				srvHeap.SetSRV(ctx, 4, resources.GetResource(data.depthBufferObject), DeviceFormat::R32_FLOAT);
				//srvHeap.SetSRV(ctx, 5, data.lightLists);
				//srvHeap.SetSRV(ctx, 6, data.lightBuffer);

				DescriptorHeap uavHeap;
				uavHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 10, &allocator);
				uavHeap.SetUAVTexture(ctx, 0, resources.GetResource(data.tempBufferObject));

				ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(resources.GetPipelineState(COMPUTETILEDSHADINGPASS));

				ctx.SetComputeConstantBufferView(0, cameraConstants);
				ctx.SetComputeConstantBufferView(1, localConstants);
				ctx.SetComputeDescriptorTable(3, srvHeap);
				ctx.SetComputeDescriptorTable(4, uavHeap);
				ctx.Dispatch(data.dispatchDims);

				CopyTexture2D(ctx, 
					resources.CopyToTexture(data.renderTargetObject, ctx),
					resources.CopyUAVTexture(data.tempBufferObject, ctx));
			});

		return pass;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VS_Main", "vs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");
		auto GShader = RS->LoadShader("GS_Main", "gs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");

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
		Rast_Desc.CullMode = D3D12_CULL_MODE_BACK;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RSDefault;
			PSO_Desc.VS                    = VShader;
			PSO_Desc.GS                    = GShader;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets      = 0;
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
			PSO_Desc.RasterizerState       = Rast_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ShadowPassMatrices CalculateShadowMapMatrices(const float3 pos, const float r, const float T)
	{
		const auto M1 = DirectX::XMMatrixRotationY( pi / 2.0f) * DirectX::XMMatrixRotationX(pi); // left Face
		const auto M2 = DirectX::XMMatrixRotationY(-pi / 2.0f) * DirectX::XMMatrixRotationX(pi); // right

		const auto M3 = DirectX::XMMatrixRotationX( pi / 2.0f); // Top Face
		const auto M4 = DirectX::XMMatrixRotationX(-pi / 2.0f); // Bottom Face

		const auto M5 = Float4x4ToXMMATIRX(Quaternion2Matrix(Quaternion(0, 180, 180)));
		const auto M6 = DirectX::XMMatrixRotationY(0) * DirectX::XMMatrixRotationZ(pi); // backward

		const XMMATRIX ViewOrientations[] = {
			M1,
			M2,
			M3,
			M4,
			M5,
			M6,
		};

		ShadowPassMatrices out;

		for (size_t I = 0; I < 6; I++)
		{
			const XMMATRIX ViewI        = ViewOrientations[I] * DirectX::XMMatrixTranslationFromVector(pos);

			const XMMATRIX View           = DirectX::XMMatrixInverse(nullptr, ViewI);
			const XMMATRIX perspective    = DirectX::XMMatrixPerspectiveFovRH(float(-pi/2), 1, 0.1f, r);
			const XMMATRIX PV             = XMMatrixTranspose(perspective) * XMMatrixTranspose(View);

			out.PV[I]       = XMMatrixToFloat4x4(PV).Transpose();
			out.ViewI[I]    = XMMatrixToFloat4x4(ViewI);
		}

		return out;
	}


	/************************************************************************************************/


    float cot(float t)
    {
        return 1.0f / std::tanf(t);
    }

    float ScreenSpaceSize(const Camera c, BoundingSphere bs)
    {
        const auto position = GetPositionW(c.Node);
        const auto d        = (position - bs.xyz()).magnitude();
        const auto r        = bs.r;
        const auto fovy     = c.FOV / c.AspectRatio;
        const auto pr       = cot(fovy / 2) * r / sqrt(d*d - r*r);

        return pi * pr * pr;
    }

	ShadowMapPassData& ShadowMapPass(
		FrameGraph&                     frameGraph,
        const SceneDescription          sceneDesc,
        const uint2                     screenWH,
		ReserveConstantBufferFunction   reserveCB,
		const double                    t,
		iAllocator*                     allocator)
	{
		auto& shadowMapPass     = allocator->allocate<ShadowMapPassData>(ShadowMapPassData{ allocator->allocate<Vector<TemporaryFrameResourceHandle>>(allocator) });
		auto& pointLights       = PointLightComponent::GetComponent();
		const auto lightCount   = pointLights.elements.size();
        const auto frustum      = GetFrustum(sceneDesc.camera);
        const auto camera       = CameraComponent::GetComponent().GetCamera(sceneDesc.camera);

        for(size_t I = 0; I < pointLights.elements.size(); I++)
		{
            auto& pointLight    = pointLights.elements[I].componentData;
            auto& lightHandle   = pointLights.elements[I].handle;

            BoundingSphere boundingSphere{ GetPositionW(pointLight.Position), pointLight.R };
            if(Intersects(frustum, boundingSphere) || true)
            {
			    frameGraph.AddNode<LocalShadowMapPassData>(
				    LocalShadowMapPassData{
					    shadowMapPass,
                        sceneDesc.PVS,
					    reserveCB,
				    },
				    [&](FrameGraphNodeBuilder& builder, LocalShadowMapPassData& data)
				    {
                        builder.AddDataDependency(sceneDesc.cameras);
                        builder.AddDataDependency(sceneDesc.PVS);

                        const float SSSize  = ScreenSpaceSize(camera, boundingSphere);
                        const uint2 WH      = { 128, 128 };

                        auto shadowMap      = builder.AcquireVirtualResource(GPUResourceDesc::DepthTarget(WH, DeviceFormat::D32_FLOAT, 6), DRS_DEPTHBUFFERWRITE, false);
                        builder.SetDebugName(shadowMap, "Shadow Map");

					    data.shadowMapTarget    = shadowMap;
					    data.pointLight         = lightHandle;

                        shadowMapPass.shadowMapTargets.push_back(shadowMap);
				    },
				    [=](LocalShadowMapPassData& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
				    {
					    const auto& drawables  = data.sceneSource.GetData().solid;
					    const auto depthTarget = resources.GetResource(data.shadowMapTarget);

					    const DepthStencilView_Options DSV_desc = {
                            0, 0,
						    depthTarget
					    };

					    ctx.ClearDepthBuffer(depthTarget, 1.0f);

					    if (!drawables.size())
						    return;

					    struct PassConstants
					    {
						    struct PlaneMatrices
						    {
							    float4x4 ViewI;
							    float4x4 PV;
						    }matrices[6];
					    };

					    CBPushBuffer passConstantBuffer = data.reserveCB(
						    AlignedSize<PassConstants>() * lightCount);

					    CBPushBuffer localConstantBuffer = data.reserveCB(AlignedSize<Drawable::VConstantsLayout>() * drawables.size());

                        for (auto& drawable : drawables)
                            ConstantBufferDataSet{ drawable.D->GetConstants(), localConstantBuffer };

					    auto constants = CreateCBIterator<Drawable::VConstantsLayout>(localConstantBuffer);

					    auto PSO = resources.GetPipelineState(SHADOWMAPPASS);

					    ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
					    ctx.SetPipelineState(PSO);
					    ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

					    const auto& pointLights = PointLightComponent::GetComponent();
					    const auto& light       = pointLights[data.pointLight];
					    const float3 Position   = FlexKit::GetPositionW(light.Position);
					    const auto matrices     = CalculateShadowMapMatrices(Position, light.R, t);

					    PassConstants passConstantData;

					    for(size_t I = 0; I < 6; I++)
					    {
						    passConstantData.matrices[I].ViewI  = matrices.ViewI[I];
						    passConstantData.matrices[I].PV     = matrices.PV[I];
					    }

					    ConstantBufferDataSet passConstants{ passConstantData, passConstantBuffer };

					    ctx.SetScissorAndViewports({ depthTarget });
					    ctx.SetRenderTargets2({}, 0, DSV_desc);

					    ctx.SetGraphicsConstantBufferView(0, passConstants);

					    for(size_t drawableIdx = 0; drawableIdx < drawables.size(); drawableIdx++)
					    {
						    auto* const triMesh = GetMeshResource(drawables[drawableIdx].D->MeshHandle);

						    ctx.SetGraphicsConstantBufferView(1, constants[drawableIdx]);

						    ctx.AddIndexBuffer(triMesh);
						    ctx.AddVertexBuffers(triMesh,
							    { VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });

                           ctx.DrawIndexedInstanced(triMesh->IndexCount);
					    }
				    });
		    }
        }
		return shadowMapPass;
	}


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2019-2020 Robert May

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
