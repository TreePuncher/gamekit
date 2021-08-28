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
			PSO_Desc.NumRenderTargets      = 3;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Specular
			PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
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
			PSO_Desc.NumRenderTargets      = 3;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Specular
			PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
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
            RS->Library.RSDefault,
            computeShader
        };

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

        FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateResolutionMatch_PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("ResolutionMatch", "cs_6_0", R"(assets\shaders\ResolutionMatch.hlsl)");

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


    ID3D12PipelineState* CreateClearResolutionMatch_PSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("Clear", "cs_6_0", R"(assets\shaders\ResolutionMatch.hlsl)");

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


    ID3D12PipelineState* CreateClusterBufferPSO(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("CreateClusterBuffer", "cs_6_0", R"(assets\shaders\ClusterBuffer.hlsl)");

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
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
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


    ID3D12PipelineState* CreateBuildZLayer(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("GenerateZLevel", "cs_6_0", R"(assets\shaders\HZB.hlsl)");

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


    ID3D12PipelineState* CreateDepthBufferCopy(RenderSystem* RS)
    {
        Shader computeShader = RS->LoadShader("GenerateZLevel", "cs_6_0", R"(assets\shaders\HZB.hlsl)");

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


    ID3D12PipelineState* CreateDEBUGBVHVIS(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\DebugVISBVH.hlsl");
        auto GShader = RS->LoadShader("GMain", "gs_6_0", "assets\\shaders\\DebugVISBVH.hlsl");
        auto PShader = RS->LoadShader("PMain", "ps_6_0", "assets\\shaders\\DebugVISBVH.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "MIN", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	 D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "MAX", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= false;

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
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/

    WorldRender::WorldRender(RenderSystem& RS_IN, TextureStreamingEngine& IN_streamingEngine, iAllocator* persistent) :
			renderSystem                { RS_IN },
			enableOcclusionCulling	    { false	},

            UAVPool                     { renderSystem, 256 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVBuffer, persistent },
            UAVTexturePool              { renderSystem, 256 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVTextures, persistent },
            RTPool                      { renderSystem, 512 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::RenderTarget, persistent },

            timeStats                   { renderSystem.CreateTimeStampQuery(256) },
            timingReadBack              { renderSystem.CreateReadBackBuffer(512) },

			streamingEngine		        { IN_streamingEngine },
            createClusterLightListLayout{ renderSystem.CreateIndirectLayout({ ILE_DispatchCall }, persistent) },
            createDebugDrawLayout       { renderSystem.CreateIndirectLayout({ ILE_DrawCall }, persistent) }
	{
		RS_IN.RegisterPSOLoader(FORWARDDRAW,			    { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,		  });
		RS_IN.RegisterPSOLoader(FORWARDDRAWINSTANCED,	    { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO });

		RS_IN.RegisterPSOLoader(LIGHTPREPASS,			    { &RS_IN.Library.ComputeSignature,  CreateLightPassPSO			  });
		RS_IN.RegisterPSOLoader(DEPTHPREPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateDepthPrePassPSO         });
        RS_IN.RegisterPSOLoader(SHADOWMAPPASS,              { &RS_IN.Library.RS6CBVs4SRVs,      CreateShadowMapPass           });
        RS_IN.RegisterPSOLoader(SHADOWMAPANIMATEDPASS,      { &RS_IN.Library.RS6CBVs4SRVs,      CreateShadowMapAnimatedPass   });

		RS_IN.RegisterPSOLoader(GBUFFERPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferPassPSO          });
		RS_IN.RegisterPSOLoader(GBUFFERPASS_SKINNED,	    { &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferSkinnedPassPSO   });

		RS_IN.RegisterPSOLoader(SHADINGPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateDeferredShadingPassPSO  });
		RS_IN.RegisterPSOLoader(ENVIRONMENTPASS,            { &RS_IN.Library.RS6CBVs4SRVs,      CreateEnvironmentPassPSO      });
		RS_IN.RegisterPSOLoader(COMPUTETILEDSHADINGPASS,    { &RS_IN.Library.RSDefault,         CreateComputeTiledDeferredPSO });

        RS_IN.RegisterPSOLoader(CREATECLUSTERS,             { &RS_IN.Library.ComputeSignature,  CreateClustersPSO               });
        RS_IN.RegisterPSOLoader(CREATECLUSTERBUFFER,        { &RS_IN.Library.ComputeSignature,  CreateClusterBufferPSO });
        RS_IN.RegisterPSOLoader(CREATECLUSTERLIGHTLISTS,    { &RS_IN.Library.ComputeSignature,  CreateClusterLightListsPSO      });
        RS_IN.RegisterPSOLoader(CREATELIGHTBVH_PHASE1,      { &RS_IN.Library.ComputeSignature,  CreateLightBVH_PHASE1_PSO       });
        RS_IN.RegisterPSOLoader(CREATELIGHTBVH_PHASE2,      { &RS_IN.Library.ComputeSignature,  CreateLightBVH_PHASE2_PSO       });
        RS_IN.RegisterPSOLoader(CREATELIGHTLISTARGS_PSO,    { &RS_IN.Library.ComputeSignature,  CreateLightListArgs_PSO         });
        RS_IN.RegisterPSOLoader(CLEARCOUNTERSPSO,           { &RS_IN.Library.ComputeSignature,  CreateClearClusterCountersPSO   });
                
		RS_IN.RegisterPSOLoader(BILATERALBLURPASSHORIZONTAL, { &RS_IN.Library.RSDefault, CreateBilaterialBlurHorizontalPSO });
		RS_IN.RegisterPSOLoader(BILATERALBLURPASSVERTICAL,   { &RS_IN.Library.RSDefault, CreateBilaterialBlurVerticalPSO   });

		RS_IN.RegisterPSOLoader(LIGHTBVH_DEBUGVIS_PSO,          { &RS_IN.Library.RSDefault, CreateLightBVH_DEBUGVIS_PSO });
		RS_IN.RegisterPSOLoader(CLUSTER_DEBUGVIS_PSO,           { &RS_IN.Library.RSDefault, CreateCluster_DEBUGVIS_PSO });
		RS_IN.RegisterPSOLoader(CLUSTER_DEBUGARGSVIS_PSO,       { &RS_IN.Library.RSDefault, CreateCluster_DEBUGARGSVIS_PSO });
        RS_IN.RegisterPSOLoader(CREATELIGHTDEBUGVIS_PSO,        { &RS_IN.Library.RSDefault, CreateLight_DEBUGARGSVIS_PSO });
        RS_IN.RegisterPSOLoader(RESOLUTIONMATCHSHADOWMAPS,      { &RS_IN.Library.RSDefault, CreateResolutionMatch_PSO });
        RS_IN.RegisterPSOLoader(CLEARSHADOWRESOLUTIONBUFFER,    { &RS_IN.Library.RSDefault, CreateClearResolutionMatch_PSO});

        RS_IN.RegisterPSOLoader(ZPYRAMIDBUILDLEVEL, { &RS_IN.Library.RSDefault, CreateBuildZLayer });
        RS_IN.RegisterPSOLoader(DEPTHCOPY,          { &RS_IN.Library.RSDefault, CreateDepthBufferCopy });

        RS_IN.RegisterPSOLoader(DEBUG_DrawBVH,      { &RS_IN.Library.RSDefault, CreateDEBUGBVHVIS });

        RS_IN.RegisterPSOLoader(OITBLEND,           { &RS_IN.Library.RSDefault, CreateOITBlendPSO });
        RS_IN.RegisterPSOLoader(OITDRAW,            { &RS_IN.Library.RSDefault, CreateOITDrawPSO });
        RS_IN.RegisterPSOLoader(OITDRAWANIMATED,    { &RS_IN.Library.RSDefault, CreateOITDrawAnimatedPSO });

        RS_IN.RegisterPSOLoader(CREATEINITIALLUMANANCELEVEL,    { &RS_IN.Library.RSDefault, CreateInitialLumananceLevel });
        RS_IN.RegisterPSOLoader(CREATELUMANANCELEVEL,           { &RS_IN.Library.RSDefault, CreateLumananceLevel });

		RS_IN.QueuePSOLoad(GBUFFERPASS);
		RS_IN.QueuePSOLoad(GBUFFERPASS_SKINNED);
		RS_IN.QueuePSOLoad(DEPTHPREPASS);
		RS_IN.QueuePSOLoad(LIGHTPREPASS);
		RS_IN.QueuePSOLoad(FORWARDDRAW);
		RS_IN.QueuePSOLoad(FORWARDDRAWINSTANCED);
		RS_IN.QueuePSOLoad(SHADINGPASS);
		RS_IN.QueuePSOLoad(COMPUTETILEDSHADINGPASS);
		RS_IN.QueuePSOLoad(BILATERALBLURPASSHORIZONTAL);
		RS_IN.QueuePSOLoad(BILATERALBLURPASSVERTICAL);
        RS_IN.QueuePSOLoad(SHADOWMAPPASS);
        RS_IN.QueuePSOLoad(CLEARCOUNTERSPSO);
        RS_IN.QueuePSOLoad(RESOLUTIONMATCHSHADOWMAPS);
        RS_IN.QueuePSOLoad(CLEARSHADOWRESOLUTIONBUFFER);
        RS_IN.QueuePSOLoad(ZPYRAMIDBUILDLEVEL);

        RS_IN.QueuePSOLoad(DEBUG_DrawBVH);

        RS_IN.QueuePSOLoad(CREATELIGHTBVH_PHASE1);
        RS_IN.QueuePSOLoad(CREATELIGHTBVH_PHASE2);

        RS_IN.SetReadBackEvent(
            timingReadBack,
            [&](ReadBackResourceHandle resource)
            {
                auto [buffer, bufferSize] = RS_IN.OpenReadBackBuffer(resource);
                EXITSCOPE(RS_IN.CloseReadBackBuffer(resource));

                if(buffer){
                    size_t timePoints[64];
                    memcpy(timePoints, (char*)buffer, sizeof(timePoints));

                    UINT64 timeStampFreq;
                    RS_IN.GraphicsQueue->GetTimestampFrequency(&timeStampFreq);

                    float durations[32];

                    for (size_t I = 0; I < 4; I++)
                        durations[I] = float(timePoints[2 * I + 1] - timePoints[2 * I + 0]) / timeStampFreq * 1000;

                    timingValues.gBufferPass        = durations[0];
                    timingValues.ClusterCreation    = durations[1];
                    timingValues.shadingPass        = durations[2];
                    timingValues.BVHConstruction    = durations[3];
                }
            });

        readBackBuffers.push_back(renderSystem.CreateReadBackBuffer(64 * KILOBYTE));
	}


    WorldRender::~WorldRender()
	{
        Release();
    }


	void WorldRender::HandleTextures()
	{

	}


	void WorldRender::Release()
	{
        if(clusterBuffer != InvalidHandle_t)
            renderSystem.ReleaseResource(clusterBuffer);
	}


    DrawOutputs WorldRender::DrawScene(UpdateDispatcher& dispatcher, FrameGraph& frameGraph, DrawSceneDescription& drawSceneDesc, WorldRender_Targets targets, iAllocator* persistent, ThreadSafeAllocator& temporary)
    {
        auto&       scene           = drawSceneDesc.scene;
        const auto  camera          = drawSceneDesc.camera;
        auto&       gbuffer         = drawSceneDesc.gbuffer;
        const auto  t               = drawSceneDesc.t;

        auto&       depthTarget     = targets.DepthTarget;
        auto        renderTarget    = targets.RenderTarget;

        auto& PVS = GatherScene(dispatcher, &scene, camera, temporary);

        PVS.AddInput(drawSceneDesc.transformDependency);
        PVS.AddInput(drawSceneDesc.cameraDependency);

        auto& pointLightGather      = scene.GetPointLights(dispatcher, temporary);
        auto& sceneBVH              = scene.UpdateSceneBVH(dispatcher, drawSceneDesc.transformDependency, temporary);
        auto& visablePointLights    = scene.GetVisableLights(dispatcher, camera, sceneBVH, temporary);
        auto& pointLightUpdate      = scene.UpdatePointLights(dispatcher, sceneBVH, visablePointLights, temporary, persistent);
        auto& shadowMaps            = AcquireShadowMaps(dispatcher, frameGraph.GetRenderSystem(), RTPool, pointLightUpdate);

        LoadLodLevels(dispatcher, PVS, drawSceneDesc.camera, renderSystem, *persistent);

        pointLightGather.AddInput(drawSceneDesc.transformDependency);
        pointLightGather.AddInput(drawSceneDesc.cameraDependency);

        auto& IKUpdate          = UpdateIKControllers(dispatcher, drawSceneDesc.dt);
        auto& animationUpdate   = UpdateAnimations(dispatcher, drawSceneDesc.dt);
        auto& skinnedObjects    = GatherSkinned(dispatcher, scene, camera, temporary);
        auto& updatedPoses      = UpdatePoses(dispatcher, skinnedObjects);

        // [skinned Objects] -> [update Poses]
        IKUpdate.AddInput(drawSceneDesc.transformDependency);
        updatedPoses.AddInput(IKUpdate);
        updatedPoses.AddInput(drawSceneDesc.transformDependency);
        skinnedObjects.AddInput(drawSceneDesc.cameraDependency);
        updatedPoses.AddInput(skinnedObjects);
        updatedPoses.AddInput(animationUpdate);

        const SceneDescription sceneDesc = {
            drawSceneDesc.camera,
            pointLightGather,
            visablePointLights,
            shadowMaps,
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
        frameGraph.dataDependencies.push_back(&IKUpdate);

        for (auto& task : pendingGPUTasks)
            task(frameGraph);

        pendingGPUTasks.clear();

        ClearGBuffer(gbuffer, frameGraph);

        auto& entityConstants =
            BuildEntityConstantsBuffer(
                frameGraph,
                dispatcher,
                PVS,
                drawSceneDesc.reserveCB,
                temporary);

        auto& occlutionConstants =
            OcclusionCulling(
                dispatcher,
                frameGraph,
                entityConstants,
                PVS,
                camera,
                drawSceneDesc.reserveCB,
                depthTarget,
                temporary);

        auto& gbufferPass =
            RenderPBR_GBufferPass(
                dispatcher,
                frameGraph,
                sceneDesc,
                sceneDesc.camera,
                gbuffer,
                depthTarget.Get(),
                reserveCB,
                temporary);

        for (auto& pass : drawSceneDesc.additionalGbufferPasses)
            pass();

        auto& lightPass =
            UpdateLightBuffers(
                dispatcher,
                frameGraph,
                camera,
                scene,
                sceneDesc,
                depthTarget.Get(),
                reserveCB,
                temporary,
                drawSceneDesc.debugDisplay != DebugVisMode::ClusterVIS);

        auto& shadowMapPass =
            ShadowMapPass(
                frameGraph,
                sceneDesc,
                reserveCB,
                reserveVB,
                drawSceneDesc.additionalShadowPasses,
                t,
                &temporary);

        auto& deferredPass =
            RenderPBR_DeferredShade(
                dispatcher,
                frameGraph,
                sceneDesc,
                pointLightGather,
                gbuffer, depthTarget.Get(), renderTarget, shadowMapPass, lightPass,
                reserveCB, reserveVB,
                t,
                temporary);

        auto& OIT_pass =
            RenderPBR_OITPass(
                dispatcher,
                frameGraph,
                sceneDesc,
                pointLightGather,
                depthTarget.Get(),
                deferredPass.renderTargetObject,
                shadowMapPass,
                lightPass,
                reserveCB,
                temporary);

        auto& toneMapped =
            RenderPBR_ToneMapping(
                dispatcher,
                frameGraph,
                OIT_pass.renderTargetObject,
                renderTarget,
                reserveCB,
                reserveVB,
                drawSceneDesc.dt,
                temporary);


        if (drawSceneDesc.debugDisplay == DebugVisMode::ClusterVIS)
        {
            DEBUGVIS_DrawLightBVH(
                dispatcher,
                frameGraph,
                camera,
                targets.RenderTarget,
                lightPass,
                reserveCB,
                drawSceneDesc.debugDrawMode,
                temporary);
        }
        else if(drawSceneDesc.debugDisplay == DebugVisMode::BVHVIS)
        {
            DEBUGVIS_BVH(
                dispatcher,
                frameGraph,
                *sceneBVH.GetData().bvh,
                camera,
                renderTarget,
                reserveCB,
                reserveVB,
                drawSceneDesc.BVHVisMode,
                temporary);
        }

        return DrawOutputs{
                    PVS,
                    skinnedObjects,
                    entityConstants,
                    visablePointLights,
                    occlutionConstants.ZPyramid
        };
    }


    /************************************************************************************************/


    EntityConstants& WorldRender::BuildEntityConstantsBuffer(
        FrameGraph&                     frameGraph,
        UpdateDispatcher&               dispatcher,
        GatherTask&                     sceneGather,
        ReserveConstantBufferFunction&  reserveConstants,
        iAllocator&                     allocator)
    {
        return frameGraph.AddNode<EntityConstants>(
            EntityConstants
            {
                CreateOnceReserveBuffer(reserveConstants, &allocator),
                sceneGather
            },
            [&](FrameGraphNodeBuilder& builder, EntityConstants& data) {},
            [](EntityConstants& data, const ResourceHandler& resourecs, Context& ctx, iAllocator& allocator)
            {
                auto& constantBuffer    = data.GetConstants();
                auto& brushes           = data.sceneGather.GetData().solid;

                for (auto& brush : brushes)
                    constantBuffer.Push(brush->GetConstants());
            });
    }


    /************************************************************************************************/


    OcclusionCullingResults& WorldRender::OcclusionCulling(
                UpdateDispatcher&               dispatcher,
                FrameGraph&                     frameGraph,
                EntityConstants&                entityConstants,
                GatherTask&                     pvs,
                CameraHandle                    camera,
                ReserveConstantBufferFunction&  reserveConstants,
                DepthBuffer&                    depthBuffer,
                ThreadSafeAllocator&            temporary)
    {
        auto& occlusion = frameGraph.AddNode<OcclusionCullingResults>(
            OcclusionCullingResults{
                pvs,
                entityConstants,
                reserveConstants,
            },
            [&](FrameGraphNodeBuilder& builder, OcclusionCullingResults& data)
            {
                const uint2 WH          = builder.GetRenderSystem().GetTextureWH(depthBuffer.Get());
                const uint MipLevels    = log2(Max(WH[0], WH[1]));

                data.depthBuffer    = builder.AcquireVirtualResource(GPUResourceDesc::DepthTarget(WH, DeviceFormat::D32_FLOAT), DRS_DEPTHBUFFERWRITE);
                data.ZPyramid       = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture(WH, DeviceFormat::R32_FLOAT, true, MipLevels), DRS_UAV);

                builder.SetDebugName(data.depthBuffer, "depthBuffer");
                builder.SetDebugName(data.ZPyramid, "ZPyramid");
            },
            [camera = camera](OcclusionCullingResults& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                return;
                ctx.BeginEvent_DEBUG("Occlusion Culling");
                ctx.BeginEvent_DEBUG("Occluder Pass");

                auto& constants             = data.entityConstants.GetConstants();
                auto ZPassConstantsBuffer   = data.reserveCB(AlignedSize<Camera::ConstantBuffer>());

                const auto cameraConstants = ConstantBufferDataSet{ GetCameraConstants(camera), ZPassConstantsBuffer };

                DescriptorHeap heap{
                    ctx,
                    resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
                    &allocator };

                heap.NullFill(ctx);

                ctx.ClearDepthBuffer(resources.GetResource(data.depthBuffer), 1.0f);
                ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
                ctx.SetPipelineState(resources.GetPipelineState(DEPTHPREPASS));

                ctx.SetScissorAndViewports({ resources.GetResource(data.depthBuffer) });
                ctx.SetRenderTargets(
                    {},
                    true,
                    resources.GetResource(data.depthBuffer));

                ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
                ctx.SetGraphicsDescriptorTable(0, heap);
                ctx.SetGraphicsConstantBufferView(1, cameraConstants);
                ctx.SetGraphicsConstantBufferView(3, cameraConstants);
                ctx.NullGraphicsConstantBufferView(6);

                TriMesh* prevMesh           = nullptr;
                const auto& brushes         = data.sceneGather.GetData().solid;
                const size_t brushCount     = brushes.size();

                auto brushConstants = CreateCBIterator<Brush::VConstantsLayout>(constants);

                for (size_t I = 0; I < brushCount; I++)
                {
                    auto& brush         = brushes[I];
                    auto* const triMesh = GetMeshResource(brush.brush->MeshHandle);
                    const auto& lod     = triMesh->GetLowestLoadedLod();
                    const auto& lodIdx  = triMesh->GetLowestLodIdx();

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh, lodIdx);
                        ctx.AddVertexBuffers(triMesh,
                            lodIdx,
                            { VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });
                    }
                    
                    ctx.SetGraphicsConstantBufferView(2, brushConstants[I]);
                    ctx.DrawIndexedInstanced(lod.GetIndexCount());
                }

                ctx.EndEvent_DEBUG();

                ctx.BeginEvent_DEBUG("Occlusion Culling - Create Z-Pyramid");

                auto& rootSignature = resources.renderSystem().Library.RSDefault;

                ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);

                auto currentConstants   = GetCameraConstants(camera);
                auto previousConstants  = GetCameraPreviousConstants(camera);

                struct ReprojectionConstants
                {
                    float4x4 currentPV;
                    float4x4 previousIView;

                    float3  TLCorner_VS;
                    float3  TRCorner_VS;

                    float3  BLCorner_VS;
                    float3  BRCorner_VS;

                    float3  cameraPOS;
                    float   MaxZ;
                } projectionConstants{
                    .currentPV      = currentConstants.PV,
                    .previousIView  = previousConstants.ViewI,

                    .TLCorner_VS    = currentConstants.TLCorner_VS,
                    .TRCorner_VS    = currentConstants.TRCorner_VS,

                    .BLCorner_VS    = currentConstants.BLCorner_VS,
                    .BRCorner_VS    = currentConstants.BRCorner_VS,

                    .cameraPOS      = currentConstants.WPOS.xyz(),
                    .MaxZ           = currentConstants.MaxZ
                };

                auto CBBuffer   = data.reserveCB(AlignedSize<ReprojectionConstants>());
                auto computeCB  = ConstantBufferDataSet{ projectionConstants, CBBuffer };

                const auto ZPyramid = resources.GetResource(data.ZPyramid);

                DescriptorHeap SRVHeap;
                SRVHeap.Init2(ctx, rootSignature.GetDescHeap(0), 1, &allocator);
                SRVHeap.SetSRV(ctx, 0, resources.Transition(data.depthBuffer, DRS_NonPixelShaderResource, ctx), DeviceFormat::R32_FLOAT);

                DescriptorHeap UAVHeap;
                UAVHeap.Init2(ctx, rootSignature.GetDescHeap(1), 1, &allocator);
                UAVHeap.SetUAVTexture(ctx, 0, 0, resources.Transition(data.ZPyramid, DRS_UAV, ctx), DeviceFormat::R32_FLOAT);

                ctx.SetComputeConstantBufferView(0, computeCB);
                ctx.SetComputeDescriptorTable(4, SRVHeap);
                ctx.SetComputeDescriptorTable(5, UAVHeap);

                const uint2 dispatchWH = resources.GetTextureWH(data.ZPyramid);

                auto copySource     = resources.GetPipelineState(DEPTHCOPY);
                auto buildZLevel    = resources.GetPipelineState(ZPYRAMIDBUILDLEVEL);

                ctx.Dispatch(copySource,    { dispatchWH[0], dispatchWH[1], 1 });

                ctx.AddUAVBarrier(ZPyramid);

                const uint mipCount = 5;
                const uint end      = mipCount - 1;

                for (uint32_t I = 0; I < end; I++)
                {
                    ctx.Dispatch(buildZLevel, { dispatchWH[0], dispatchWH[1], 1 });

                    DescriptorHeap SRVHeap;
                    SRVHeap.Init2(ctx, rootSignature.GetDescHeap(0), 1, &allocator);
                    SRVHeap.SetSRV(ctx, 0, resources.GetResource(data.ZPyramid), I, DeviceFormat::R32_FLOAT);

                    DescriptorHeap UAVHeap;
                    UAVHeap.Init2(ctx, rootSignature.GetDescHeap(1), 1, &allocator);
                    UAVHeap.SetUAVTexture(ctx, 0, I + 1, resources.Transition(data.ZPyramid, DRS_NonPixelShaderResource, ctx), DeviceFormat::R32_FLOAT);

                    ctx.SetComputeDescriptorTable(4, SRVHeap);
                    ctx.SetComputeDescriptorTable(5, UAVHeap);

                    ctx.Dispatch(buildZLevel, { dispatchWH[0], dispatchWH[1], 1 });
                    ctx.AddUAVBarrier(ZPyramid);
                }

                ctx.EndEvent_DEBUG();
                ctx.EndEvent_DEBUG();
            });

        return occlusion;
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

                data.entityConstantsBuffer  = std::move(reserveConsantBufferSpace(sizeof(ForwardDrawConstants) * MaxEntityDrawCount));
                data.passConstantsBuffer    = std::move(reserveConsantBufferSpace(2048));
                data.depthBufferObject      = builder.DepthTarget(depthBufferTarget);
                data.depthPassTarget        = depthBufferTarget;

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

                ctx.BeginEvent_DEBUG("Z-PrePass");

                TriMesh* prevMesh = nullptr;
                for (const auto& brush : data.brushes)
                {
                    const auto  lodIdx  = brush.LODlevel;
                    auto* const triMesh = GetMeshResource(brush->MeshHandle);
                    const auto& lod     = triMesh->lods[lodIdx];

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx.AddIndexBuffer(triMesh, lodIdx);
                        ctx.AddVertexBuffers(triMesh,
                            triMesh->GetHighestLoadedLodIdx(),
                            { VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });
                    }

                    auto constants = ConstantBufferDataSet{ brush->GetConstants(), data.entityConstantsBuffer };
                    ctx.SetGraphicsConstantBufferView(2, constants);
                    ctx.DrawIndexedInstanced(lod.GetIndexCount());
                }

                ctx.EndEvent_DEBUG();
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

				data.renderTargetObject             = builder.RenderTarget(renderTarget);
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
				ctx.SetGraphicsDescriptorTable(5, descHeap);

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
				data.renderTargetObject         = builder.RenderTarget(renderTarget);

				data.AlbedoTargetObject         = builder.PixelShaderResource(gbuffer.albedo);
				data.NormalTargetObject         = builder.PixelShaderResource(gbuffer.normal);
				data.MRIATargetObject           = builder.PixelShaderResource(gbuffer.MRIA);
				data.depthBufferTargetObject    = builder.PixelShaderResource(depthTarget);

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
				descHeap.SetSRV(ctx, 4, frameResources.GetResource(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
				descHeap.NullFill(ctx, 20);

				ctx.SetRootSignature(renderSystem.Library.RSDefault);
				ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
				ctx.SetGraphicsDescriptorTable(5, descHeap);

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
				data.DestinationObject  = builder.RenderTarget(destination);
				data.TempObject1        = builder.RenderTarget(temp1);
				data.TempObject2        = builder.RenderTarget(temp2); // 2 channel
				data.TempObject3        = builder.RenderTarget(temp3); // 2 channel

				data.DepthSource        = builder.PixelShaderResource(depthBuffer);
				data.NormalSource       = builder.PixelShaderResource(gbuffer.normal);
				data.Source             = builder.PixelShaderResource(source);
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
				ctx.SetGraphicsDescriptorTable(5, descHeap);

				ctx.SetScissorAndViewports({ destination });
				ctx.SetRenderTargets({ frameResources.GetResource(data.TempObject1), frameResources.GetResource(data.TempObject2) }, false);
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, 6, vertexBuffer } });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, constantBuffer });

				ctx.Draw(6);

				DescriptorHeap descHeap2;
				descHeap2.Init2(ctx, renderSystem.Library.RSDefault.GetDescHeap(0), 5, &allocator);

				descHeap2.SetSRV(ctx, 0, frameResources.Transition(data.TempObject1, DRS_PixelShaderResource, ctx));
				descHeap2.SetSRV(ctx, 1, frameResources.GetResource(data.NormalSource));
				descHeap2.SetSRV(ctx, 2, frameResources.GetResource(data.DepthSource), DeviceFormat::R32_FLOAT);
				descHeap2.SetSRV(ctx, 3, frameResources.Transition(data.TempObject2, DRS_PixelShaderResource, ctx));

				ctx.SetPipelineState(frameResources.GetPipelineState(BILATERALBLURPASSVERTICAL));
				ctx.SetGraphicsDescriptorTable(5, descHeap2);
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

		typedef Vector<ForwardPlusPass> ForwardBrushList;
		
        FK_ASSERT(0, "NOT FULLY IMPLEMENTED!");

		auto& pass = frameGraph.AddNode<ForwardPlusPass>(
			ForwardPlusPass{
				desc.lights.GetData().pointLights,
				depthPass.brushes,
				depthPass.entityConstantsBuffer },
			[&](FrameGraphNodeBuilder& builder, ForwardPlusPass& data)
			{
				builder.AddDataDependency(desc.PVS);
				builder.AddDataDependency(desc.cameras);

				data.BackBuffer			        = builder.RenderTarget(Targets.RenderTarget);
				data.DepthBuffer                = builder.DepthTarget(Targets.DepthTarget.Get());
				const size_t localBufferSize    = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

				data.passConstantsBuffer    = std::move(reserveCB(localBufferSize * 2));

				//data.lightLists			    = builder.ReadWriteUAV(lightLists,			DRS_ShaderResource);
				//data.pointLightBuffer	    = builder.ReadWriteUAV(pointLightBuffer,	DRS_ShaderResource);
			},
			[=](ForwardPlusPass& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
                GetCameraMatrices(camera);

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

				for (size_t itr = 0; itr < data.brushes.size(); ++itr)
				{
					auto& brush         = data.brushes[itr];
					TriMesh* triMesh    = GetMeshResource(brush->MeshHandle);

					if (triMesh != prevMesh)
					{
						prevMesh = triMesh;

						ctx.AddIndexBuffer(triMesh);
                        ctx.AddVertexBuffers(triMesh,
                            triMesh->GetHighestLoadedLodIdx(),
							{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							});
					}

					auto proxy = CreateCBIterator<decltype(brush->GetConstants())>(data.entityConstants)[itr];

					ctx.SetGraphicsConstantBufferView(2, proxy);
					ctx.DrawIndexedInstanced(triMesh->GetHighestLoadedLod().GetIndexCount(), 0, 0, 1, 0);
				}
			});

		return pass;
	}


    /************************************************************************************************/


    void WorldRender::CreateClusterBuffer(uint2 WH, CameraHandle camera, ReserveConstantBufferFunction& reserveCB)
    {
        pendingGPUTasks.emplace_back(
            [&, camera = camera, WH = WH](FrameGraph& frameGraph)
            {
                if (clusterBuffer != InvalidHandle_t)
                    UAVPool.Release(clusterBuffer, false, false);

                struct GPUCluster { float4 min, max; };

                const uint2 XY          { (Vect2{ WH } / 32.0f).Ceil() };
                const uint bufferSize   { XY.Product() * sizeof(GPUCluster) * 24 };
                const uint2 dim         { (Vect2{ XY } / 32.0f).Ceil() };

                auto [resource, overlap] = UAVPool.Acquire(GPUResourceDesc::UAVResource(bufferSize), false);

                clusterBuffer = resource;

                renderSystem.SetDebugName(resource, "ClusterBuffer");

                struct _CreateClusterBuffer_Desc
                {
                    ReserveConstantBufferFunction   reserveCB;
                    FrameResourceHandle             clusterBuffer;
                };

                frameGraph.AddNode<_CreateClusterBuffer_Desc>(
                    _CreateClusterBuffer_Desc
                    {   reserveCB   },
                    [&](FrameGraphNodeBuilder& builder, _CreateClusterBuffer_Desc& data)
                    {
                        data.clusterBuffer = builder.UnorderedAccess(resource);
                    },
                    [=](_CreateClusterBuffer_Desc& data, ResourceHandler& resources, Context& ctx, iAllocator& tempAllocator)
                    {
                        ctx.BeginEvent_DEBUG("Create Cluster");

                        auto  cameraValues    = GetCameraConstants(camera);

                        struct
                        {
                            float4x4    iproj;
                            float4x4    pad;
                            uint2       WH;
                            uint2       Dim;
                            uint        rowPitch;
                            uint        slicePitch;
                        } constantValues = {
                                Inverse(cameraValues.Proj),
                                float4x4{},
                                WH,
                                XY,
                                XY[0],
                                XY.Product(),
                        };

                        auto constantBuffer     = data.reserveCB(AlignedSize(sizeof(cameraValues)) + AlignedSize(sizeof(constantValues)));
                        auto passConstants      = FlexKit::ConstantBufferDataSet{ constantValues, constantBuffer };
                        auto cameraConstants    = FlexKit::ConstantBufferDataSet{ cameraValues, constantBuffer };

                        const auto PSO      = resources.GetPipelineState(CREATECLUSTERBUFFER);
                        const auto& rootSig = resources.renderSystem().Library.ComputeSignature;

                        DescriptorHeap heap;
                        heap.Init2(ctx, rootSig.GetDescHeap(0), 10, &tempAllocator);
                        heap.SetUAVStructured(ctx, 0, resources.UAV(data.clusterBuffer, ctx), sizeof(GPUCluster));
                        heap.SetCBV(ctx, 8, passConstants);
                        heap.NullFill(ctx, 10);

                        ctx.SetComputeRootSignature(rootSig);
                        ctx.SetComputeDescriptorTable(0, heap);


                        ctx.AddAliasingBarrier(overlap, resource);
                        ctx.DiscardResource(resources.GetResource(data.clusterBuffer));
                        ctx.Dispatch(PSO, { dim, 24 });

                        ctx.EndEvent_DEBUG();
                    });
            });
    }

	/************************************************************************************************/

    
    struct alignas(16) BVH_Node
    {
        float4 MinPoint;
        float4 MaxPoint;

        uint32_t Offset;
        uint32_t Count; // max child count is 16
        uint32_t Leaf;
        uint32_t Padding;
    };

    constexpr size_t sizeofBVH_Node     = sizeof(BVH_Node);
    constexpr size_t BVH_ELEMENT_COUNT  = 32;

	LightBufferUpdate& WorldRender::UpdateLightBuffers(
		UpdateDispatcher&		        dispatcher,
		FrameGraph&				        graph,
		const CameraHandle	            camera,
		const Scene&	                scene,
		const SceneDescription&         sceneDescription,
        ResourceHandle                  depthBuffer,
		ReserveConstantBufferFunction   reserveCB,
		iAllocator*				        tempMemory,
        bool                            releaseTemporaries)

    {
        auto WH = graph.GetRenderSystem().GetTextureWH(depthBuffer);

		auto& lightBufferData = graph.AddNode<LightBufferUpdate>(
			LightBufferUpdate{
					sceneDescription.pointLightMaps.GetData().pointLightShadows,
					camera,
					reserveCB,
                    createClusterLightListLayout
			},
			[&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
			{
				auto& renderSystem          = graph.GetRenderSystem();

                // Output Objects
                data.clusterBufferObject	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1 * MEGABYTE), DRS_UAV);
                data.lightLists             = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(2 * MEGABYTE), DRS_UAV, false);
                data.indexBufferObject      = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture({ WH }, DeviceFormat::R32_UINT), DRS_UAV, false);
				data.lightListBuffer        = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1024 * KILOBYTE), DRS_UAV, false);
                data.lightBufferObject      = builder.AcquireVirtualResource(GPUResourceDesc::StructuredResource(512 * KILOBYTE), DRS_CopyDest, false);

                // Temporaries
                data.lightBVH               = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE),  DRS_UAV, releaseTemporaries);
                data.lightLookupObject      = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE),  DRS_UAV, releaseTemporaries);
                data.lightCounterObject     = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512),             DRS_UAV, releaseTemporaries);
                data.lightResolutionObject  = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(64 * KILOBYTE),   DRS_UAV);
                data.counterObject          = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV, releaseTemporaries);
                data.argumentBufferObject   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV, releaseTemporaries);

                data.depthBufferObject      = builder.DepthTarget(depthBuffer);
				data.camera				    = camera;

                if (readBackBuffers.size())
                    data.readBackHandle = readBackBuffers.pop_front();
                else
                    data.readBackHandle = InvalidHandle_t;


                builder.SetDebugName(data.clusterBufferObject,      "ClusterBufferObject");
                builder.SetDebugName(data.lightLists,               "lightLists");
                builder.SetDebugName(data.indexBufferObject,        "indexBufferObject");
                builder.SetDebugName(data.lightListBuffer,          "lightListBuffer");
                builder.SetDebugName(data.lightBufferObject,        "lightBufferObject");
                builder.SetDebugName(data.lightBVH,                 "lightBVH");
                builder.SetDebugName(data.lightLookupObject,        "lightLookupObject");
                builder.SetDebugName(data.lightCounterObject,       "lightCounterObject");
                builder.SetDebugName(data.lightResolutionObject,    "lightResolutionObject");
                builder.SetDebugName(data.counterObject,            "counterObject");
                builder.SetDebugName(data.argumentBufferObject,     "argumentBufferObject");

				builder.AddDataDependency(sceneDescription.lights);
				builder.AddDataDependency(sceneDescription.cameras);
			},
			[timeStats = timeStats, this](LightBufferUpdate& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
                ctx.BeginEvent_DEBUG("Update Light Buffers");

                auto        CreateClusters              = resources.GetPipelineState(CREATECLUSTERS);
                auto        ClearCounters               = resources.GetPipelineState(CLEARCOUNTERSPSO);
                auto        CreateBVH_Phase1            = resources.GetPipelineState(CREATELIGHTBVH_PHASE1);
                auto        CreateBVH_Phase2            = resources.GetPipelineState(CREATELIGHTBVH_PHASE2);
                auto        CreateLightLists            = resources.GetPipelineState(CREATECLUSTERLIGHTLISTS);
                auto        CreateLightListArguments    = resources.GetPipelineState(CREATELIGHTLISTARGS_PSO);
                auto        resolutionMatchShadowsMaps  = resources.GetPipelineState(RESOLUTIONMATCHSHADOWMAPS);
                auto        clearShadowMapBuffer        = resources.GetPipelineState(CLEARSHADOWRESOLUTIONBUFFER);

                const auto  cameraConstants     = GetCameraConstants(data.camera);
                const auto  lightCount          = data.visableLights.size();

                if (!lightCount)
                    return;

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

                uint32_t nodeReservation = ceilf(std::logf(float(lightCount)) / std::logf(BVH_ELEMENT_COUNT));

				CBPushBuffer    constantBuffer = data.reserveCB(
					AlignedSize( sizeof(FlexKit::GPUPointLight) * data.visableLights.size() ) +
                    AlignedSize<ConstantsLayout>() * (1 + nodeReservation) +
                    AlignedSize<Camera::ConstantBuffer>());

                auto constantBuffer2ReserveSize = AlignedSize<ConstantsLayout>() * (1 + nodeReservation);
                CBPushBuffer    constantBuffer2 = data.reserveCB(constantBuffer2ReserveSize);

                const ConstantBufferDataSet constants{ constantsValues, constantBuffer };
                const ConstantBufferDataSet cameraConstantsBuffer{ GetCameraConstants(data.camera), constantBuffer };

				Vector<FlexKit::GPUPointLight> pointLightValues{ &allocator };

				for (const auto light : data.visableLights)
				{
					const PointLight& pointLight	= pointLights[light];
					const float3 WS_position	    = GetPositionW(pointLight.Position);
                    const float3 VS_position        = (constantsValues.view * float4(WS_position, 1)).xyz();

					pointLightValues.push_back(
						{	{ pointLight.K, pointLight.I },
							{ WS_position, pointLight.R } });
				}

				const size_t uploadSize = pointLightValues.size() * sizeof(GPUPointLight);
				const ConstantBufferDataSet pointLights_GPU{ (char*)pointLightValues.data(), uploadSize, constantBuffer };


                ctx.ClearUAVTextureUint(resources.UAV(data.indexBufferObject, ctx), uint4{(uint)-1, (uint)-1, (uint)-1, (uint)-1 });

                ctx.DiscardResource(resources.GetResource(data.clusterBufferObject));
                ctx.DiscardResource(resources.GetResource(data.lightListBuffer));
                ctx.DiscardResource(resources.GetResource(data.lightBufferObject));

                ctx.DiscardResource(resources.GetResource(data.lightLists));
                ctx.DiscardResource(resources.GetResource(data.lightBVH));
                ctx.DiscardResource(resources.GetResource(data.lightLookupObject));
                ctx.DiscardResource(resources.GetResource(data.lightCounterObject));
                ctx.DiscardResource(resources.GetResource(data.lightResolutionObject));
                ctx.DiscardResource(resources.GetResource(data.counterObject));
                ctx.DiscardResource(resources.GetResource(data.argumentBufferObject));

				ctx.CopyBufferRegion(
					{ resources.GetObjectResource(pointLights_GPU.Handle()) },
					{ pointLights_GPU.Offset() },
					{ resources.GetObjectResource(resources.CopyDest(data.lightBufferObject, ctx)) },
					{ 0 },
					{ uploadSize },
					{ DRS_CopyDest },
					{ DRS_CopyDest }
				);

                ctx.SetComputeRootSignature(resources.renderSystem().Library.ComputeSignature);

                DescriptorHeap clearHeap;
                clearHeap.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                clearHeap.SetUAVStructured(ctx, 2, resources.UAV(data.counterObject, ctx), sizeof(UINT), 0);
                clearHeap.NullFill(ctx);


                ctx.SetComputeDescriptorTable(0, clearHeap);
                ctx.Dispatch(ClearCounters, uint3{ 1, 1, 1 });
                ctx.AddUAVBarrier(resources[data.counterObject]);


				DescriptorHeap clusterCreationResources;
                clusterCreationResources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);

                // UAVs start at 0
                clusterCreationResources.SetUAVStructured(ctx, 0, resources.UAV(data.clusterBufferObject, ctx), resources.UAV(data.counterObject, ctx), sizeof(GPUCluster), 0);
                clusterCreationResources.SetUAVTexture(ctx, 1, resources.UAV(data.indexBufferObject, ctx));

                // SRV's start at 4
                clusterCreationResources.SetSRV(ctx, 4, resources.Transition(data.depthBufferObject, DRS_NonPixelShaderResource, ctx), DeviceFormat::R32_FLOAT);

                // CBV's start at 8
                clusterCreationResources.SetCBV(ctx, 8, cameraConstantsBuffer);
                clusterCreationResources.SetCBV(ctx, 9, constants);
                clusterCreationResources.NullFill(ctx);

                ctx.TimeStamp(timeStats, 2);
                
                const auto threadsWH = resources.GetTextureWH(data.depthBufferObject) / 32;
                ctx.SetComputeDescriptorTable(0, clusterCreationResources);
                ctx.Dispatch(CreateClusters, uint3{ 60, 34, 1 });
                ctx.AddUAVBarrier(resources[data.lightBVH]);

                ctx.TimeStamp(timeStats, 3);

                // Create light BVH
                // TODO: allow for more than 1024 point lights
                //          move sorting of lights to a separate pass
                //          then pull sorted light list into the two pass BVH construct
                DescriptorHeap BVH_Phase1_Resources;
                BVH_Phase1_Resources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                BVH_Phase1_Resources.SetUAVStructured       (ctx, 0, resources.UAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);
                BVH_Phase1_Resources.SetUAVStructured       (ctx, 1, resources.UAV(data.lightLookupObject, ctx), sizeof(uint), 0);
                BVH_Phase1_Resources.SetStructuredResource  (ctx, 4, resources.Transition(data.lightBufferObject, DRS_NonPixelShaderResource, ctx), sizeof(GPUPointLight));
                BVH_Phase1_Resources.SetCBV(ctx, 8, constants);

                ctx.BeginEvent_DEBUG("Build BVH");

                ctx.TimeStamp(timeStats, 6);
                ctx.SetComputeDescriptorTable(0, BVH_Phase1_Resources);
                ctx.Dispatch(CreateBVH_Phase1, { 1, 1, 1 }); // Two dispatches to sync across calls
                
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
                        BVH_Phase2_Resources.SetUAVStructured(ctx, 0,       resources.UAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);
                        BVH_Phase2_Resources.SetStructuredResource(ctx, 4,  resources.Transition(data.lightBufferObject, DRS_NonPixelShaderResource, ctx), sizeof(GPUPointLight));
                        BVH_Phase2_Resources.SetCBV(ctx, 8, constantSet);

                        ctx.BeginEvent_DEBUG("BVH Phase2");

                        ctx.SetComputeDescriptorTable(0, BVH_Phase2_Resources);
                        ctx.Dispatch(CreateBVH_Phase2, { 1, 1, 1 });

                        ctx.AddUAVBarrier();

                        ctx.EndEvent_DEBUG();
                    };

                uint32_t offset       = 0;
                uint32_t nodeCount    = std::ceilf(float(lightCount) / BVH_ELEMENT_COUNT);
                const uint32_t passCount = std::floor(std::log(lightCount) / std::log(BVH_ELEMENT_COUNT));

                for (uint32_t I = 0; I < passCount; I++)
                {
                    Phase2_Pass(offset, nodeCount);
                    offset      += nodeCount;
                    nodeCount    = std::ceilf(float(nodeCount) / BVH_ELEMENT_COUNT);
                }

                ctx.EndEvent_DEBUG();

                DescriptorHeap createArgumentResources;
                createArgumentResources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
                createArgumentResources.SetUAVStructured(ctx, 0, resources.Transition(data.argumentBufferObject, DRS_UAV, ctx), sizeof(uint4), 0);
                createArgumentResources.SetUAVStructured(ctx, 1, resources.Transition(data.lightCounterObject, DRS_UAV, ctx), sizeof(uint32_t), 0);
                createArgumentResources.SetStructuredResource(ctx, 4, resources.Transition(data.counterObject, DRS_NonPixelShaderResource, ctx), sizeof(uint32_t));
                createArgumentResources.NullFill(ctx);

                ctx.BeginEvent_DEBUG("Create Light Lists");

                ctx.AddUAVBarrier();
                ctx.SetComputeDescriptorTable(0, createArgumentResources);
                ctx.Dispatch(CreateLightListArguments, { 1, 1, 1 });
                ctx.AddUAVBarrier();

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

                DescriptorHeap createLightList_ShaderResources;
                createLightList_ShaderResources.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 4, &allocator);
                createLightList_ShaderResources.SetStructuredResource(ctx, 0, resources.Transition(data.lightBVH,               DRS_NonPixelShaderResource, ctx), sizeofBVH_Node);
                createLightList_ShaderResources.SetStructuredResource(ctx, 1, resources.Transition(data.lightLookupObject,      DRS_NonPixelShaderResource, ctx), sizeof(uint32_t));
                createLightList_ShaderResources.SetStructuredResource(ctx, 2, resources.Transition(data.lightBufferObject,      DRS_NonPixelShaderResource, ctx), sizeof(GPUPointLight));
                createLightList_ShaderResources.SetStructuredResource(ctx, 3, resources.Transition(data.clusterBufferObject,    DRS_NonPixelShaderResource, ctx), sizeof(GPUCluster), 0);

                DescriptorHeap createLightList_UAVResources;
                createLightList_UAVResources.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 3, &allocator);
                createLightList_UAVResources.SetUAVStructured(ctx, 0, resources.Transition(data.lightLists,             DRS_UAV, ctx), sizeof(uint2), 0);
                createLightList_UAVResources.SetUAVStructured(ctx, 1, resources.Transition(data.lightListBuffer,        DRS_UAV, ctx), sizeof(uint32_t), 0);
                createLightList_UAVResources.SetUAVStructured(ctx, 2, resources.Transition(data.lightCounterObject,     DRS_UAV, ctx), sizeof(uint32_t), 0);

                ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);
                ctx.SetPipelineState(CreateLightLists);

                ctx.SetComputeConstantBufferView(0, lightListConstantSet);
                ctx.SetComputeConstantBufferView(1, resources.Transition(data.counterObject, DRS_NonPixelShaderResource, ctx), 0, 256);

                ctx.SetComputeDescriptorTable(4, createLightList_ShaderResources);
                ctx.SetComputeDescriptorTable(5, createLightList_UAVResources);

                ctx.ExecuteIndirect(resources.Transition(data.argumentBufferObject, DRS_INDIRECTARGS, ctx), data.indirectLayout);

                ctx.TimeStamp(timeStats, 7);

                ctx.EndEvent_DEBUG();


                if(data.readBackHandle != InvalidHandle_t && false)
                {
                    ctx.BeginEvent_DEBUG("Resolution Match Shadow Maps");
                    FK_LOG_INFO("ReadBack Buffer : %u", data.readBackHandle.to_uint());

                    struct Constants
                    {
                        float FOV;
                        uint2 WH;
                    } constants{
                        cameraConstants.FOV,
                        resources.GetTextureWH(data.depthBufferObject)
                    };

                    // Build Light Lists
                    CBPushBuffer constantBuffer4 = data.reserveCB(AlignedSize<Constants>());
                    ConstantBufferDataSet   lightListConstantSet{ constants, constantBuffer4 };

                    DescriptorHeap resolutionMatchShadowMaps;
                    resolutionMatchShadowMaps.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);

                    resolutionMatchShadowMaps.SetUAVStructured(ctx, 0,      resources.Transition(data.lightResolutionObject,    DRS_UAV, ctx), 4, 0);
                    resolutionMatchShadowMaps.SetStructuredResource(ctx, 4, resources.Transition(data.clusterBufferObject,      DRS_NonPixelShaderResource, ctx), sizeof(GPUCluster), 0);
                    resolutionMatchShadowMaps.SetStructuredResource(ctx, 5, resources.Transition(data.lightListBuffer,          DRS_NonPixelShaderResource, ctx), sizeof(uint32_t), 0);
                    resolutionMatchShadowMaps.SetStructuredResource(ctx, 6, resources.Transition(data.lightBufferObject,        DRS_NonPixelShaderResource, ctx), sizeof(GPUPointLight));
                    resolutionMatchShadowMaps.SetStructuredResource(ctx, 7, resources.Transition(data.counterObject,            DRS_NonPixelShaderResource, ctx), sizeof(uint32_t));

                    resolutionMatchShadowMaps.SetCBV(ctx, 8, lightListConstantSet);

                    resolutionMatchShadowMaps.NullFill(ctx);

                    ctx.SetComputeDescriptorTable(0, resolutionMatchShadowMaps);
                    ctx.Dispatch(clearShadowMapBuffer, { 1, 1, 1 });

                    ctx.AddUAVBarrier(resources.GetResource(data.lightResolutionObject));

                    ctx.SetPipelineState(resolutionMatchShadowsMaps);
                    ctx.ExecuteIndirect(resources.Transition(data.argumentBufferObject, DRS_INDIRECTARGS, ctx), data.indirectLayout);

                    // Write out
                    ctx.CopyBufferRegion(
                        { resources.GetObjectResource(resources.Transition(data.lightResolutionObject, DRS_CopySrc, ctx)) }, // Sources Offsets
                        { 0 },  // Source Offsets
                        { resources.GetObjectResource(data.readBackHandle) }, // Destination
                        { 0 },  // Destination Offsets
                        { 64 * KILOBYTE },
                        { DRS_CopyDest },
                        { DRS_CopyDest });

                    if(false)
                    ctx.QueueReadBack(data.readBackHandle,
                        [&, &readBackBuffers = readBackBuffers, &renderSystem = *ctx.renderSystem](ReadBackResourceHandle readBack)
                        {
                            auto [buffer, bufferSize] = renderSystem.OpenReadBackBuffer(readBack);

                            auto temp = malloc(bufferSize);
                            memcpy(temp, buffer, bufferSize);

                            readBackBuffers.push_back(readBack);
                        });

                    ctx.EndEvent_DEBUG();
                }

                ctx.EndEvent_DEBUG();
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
        ClusterDebugDrawMode            mode,
		iAllocator*                     tempMemory)
    {
        auto& lightBufferData = frameGraph.AddNode<DEBUGVIS_DrawBVH>(
            DEBUGVIS_DrawBVH{
                reserveCB,
                lightBufferUpdate
            },
            [&, this](FrameGraphNodeBuilder& builder, DEBUGVIS_DrawBVH& data)
            {
                data.lightBVH       = builder.ReadTransition(lightBufferUpdate.lightBVH,              DRS_NonPixelShaderResource);
                data.clusters       = builder.ReadTransition(lightBufferUpdate.clusterBufferObject,   DRS_NonPixelShaderResource);
                data.renderTarget   = builder.RenderTarget(renderTarget);
                data.indirectArgs   = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DRS_UAV, true);
                data.counterBuffer  = builder.ReadTransition(lightBufferUpdate.counterObject,     DRS_NonPixelShaderResource);
                data.pointLights    = builder.ReadTransition(lightBufferUpdate.lightBufferObject, DRS_NonPixelShaderResource);
                data.camera         = camera;


                builder.SetDebugName(data.indirectArgs, "indirectArgs");

                builder.ReleaseVirtualResource(lightBufferUpdate.counterObject);
                builder.ReleaseVirtualResource(lightBufferUpdate.lightBVH);
                builder.ReleaseVirtualResource(lightBufferUpdate.lightLookupObject);
                builder.ReleaseVirtualResource(lightBufferUpdate.lightCounterObject);
                builder.ReleaseVirtualResource(lightBufferUpdate.argumentBufferObject);
            },
            [&, mode = mode](DEBUGVIS_DrawBVH& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
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

                const auto lightCount = data.lightBufferUpdateData.visableLights.size();

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

                auto getArgs = [&]()
                {
                    DescriptorHeap descHeap;
                    descHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 2, &allocator);
                    descHeap.SetStructuredResource(ctx, 0, resources.GetResource(data.counterBuffer), sizeof(uint32_t), 0);
                    descHeap.NullFill(ctx, 2);

                    DescriptorHeap UAVHeap;
                    UAVHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 2, &allocator);
                    UAVHeap.SetUAVStructured(ctx, 0, resources.GetResource(data.indirectArgs), sizeof(uint32_t[4]), 0);
                    UAVHeap.NullFill(ctx, 2);

                    ctx.SetComputeDescriptorTable(4, descHeap);
                    ctx.SetComputeDescriptorTable(5, UAVHeap);

                    ctx.Dispatch(debugClusterArgsVISPSO, { 1, 1, 1 });
                };

                getArgs();

                DescriptorHeap descHeap;
                descHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), 3, &allocator);
                descHeap.SetStructuredResource(ctx, 0, resources.Transition(data.lightBVH, DRS_NonPixelShaderResource, ctx), 4, 0);
                descHeap.SetStructuredResource(ctx, 1, resources.Transition(data.clusters, DRS_NonPixelShaderResource, ctx), sizeof(GPUCluster), 0);
                descHeap.SetStructuredResource(ctx, 2, resources.GetResource(data.pointLights), sizeof(float4[2]), 0);
                descHeap.NullFill(ctx, 2);

                DescriptorHeap nullHeap;
                nullHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 2, &allocator);
                nullHeap.NullFill(ctx, 2);

                ctx.SetGraphicsConstantBufferView(0, constants);
                ctx.SetGraphicsConstantBufferView(1, constants);
                ctx.SetGraphicsConstantBufferView(2, constants);

                ctx.SetPrimitiveTopology(EIT_POINT);
                ctx.SetGraphicsDescriptorTable(4, descHeap);
                ctx.SetGraphicsDescriptorTable(5, nullHeap);

                size_t accumlator   = 0;
                size_t nodeCount    = std::ceilf(float(lightCount) / BVH_ELEMENT_COUNT);

                const uint32_t passCount = std::floor(std::log(lightCount) / std::log(BVH_ELEMENT_COUNT));

                for (uint32_t I = 0; I < passCount; I++)
                {
                    nodeCount    = std::floor(float(nodeCount) / BVH_ELEMENT_COUNT);
                    accumlator  += nodeCount;
                }

                switch (mode)
                {
                case ClusterDebugDrawMode::BVH:
                    ctx.SetPipelineState(debugBVHVISPSO);
                    ctx.Draw(3);
                    break;

                case ClusterDebugDrawMode::Clusters:
                    ctx.SetPipelineState(debugClusterVISPSO);
                    ctx.ExecuteIndirect(resources.Transition(data.indirectArgs, DRS_INDIRECTARGS, ctx), createDebugDrawLayout);
                    break;

                case ClusterDebugDrawMode::Lights:
                    ctx.SetPipelineState(debugLightVISPSO);
                    ctx.Draw(lightCount);
                    break;
                }
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

				data.AlbedoTargetObject      = builder.RenderTarget(gbuffer.albedo);
				data.MRIATargetObject        = builder.RenderTarget(gbuffer.MRIA);
				data.NormalTargetObject      = builder.RenderTarget(gbuffer.normal);
				data.depthBufferTargetObject = builder.DepthTarget(depthTarget);
			},
			[camera, timeStats = timeStats](GBufferPass& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
                ctx.TimeStamp(timeStats, 0);

				struct EntityPoses
				{
					float4x4 transforms[512];

					auto& operator [](size_t idx)
					{
						return transforms[idx];
					}
				};

				if (!data.pvs.size())
					return;

                size_t brushCount = 0;
                for (auto& brush : data.pvs)
                {
                    auto* triMesh       = GetMeshResource(brush->MeshHandle);
                    const auto lodIdx   = brush.LODlevel;
                    auto& detailLevel   = triMesh->lods[lodIdx];

                    brushCount += Max(detailLevel.subMeshes.size(), 1);
                }

                for (auto& skinnedBrushes : data.skinned)
                {
                    auto* triMesh       = GetMeshResource(skinnedBrushes.brush->MeshHandle);
                    const auto lodIdx   = skinnedBrushes.lodLevel;
                    auto& detailLevel   = triMesh->lods[lodIdx];

                    brushCount += Max(detailLevel.subMeshes.size(), 1);
                }


				const size_t entityBufferSize =
					AlignedSize<Brush::VConstantsLayout>() * brushCount;

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
					});

				RenderTargetList renderTargets = {
					resources.GetResource(data.AlbedoTargetObject),
					resources.GetResource(data.MRIATargetObject),
					resources.GetResource(data.NormalTargetObject),
				};

				ctx.SetRenderTargets(
					renderTargets,
					true,
					resources.GetResource(data.depthBufferTargetObject));

				// Setup Constants
				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(3, passConstants);

				// submit draw calls
				TriMesh*                    prevMesh = nullptr;
                const TriMesh::LOD_Runtime* prevLOD  = nullptr;

                ctx.BeginEvent_DEBUG("G-Buffer Pass");

                DescriptorHeap defaultHeap{};
                defaultHeap.Init(
                    ctx,
                    resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0u),
                    &allocator);

                defaultHeap.SetSRV(ctx, 0, resources.renderSystem().DefaultTexture);
                defaultHeap.SetSRV(ctx, 1, resources.renderSystem().DefaultTexture);
                defaultHeap.SetSRV(ctx, 2, resources.renderSystem().DefaultTexture);
                defaultHeap.SetSRV(ctx, 3, resources.renderSystem().DefaultTexture);
                defaultHeap.NullFill(ctx);

				// unskinned models
				for (auto& brush : data.pvs)
				{
					auto constants    = brush->GetConstants();
					auto* triMesh     = GetMeshResource(brush->MeshHandle);
                    auto  lodIdx      = brush.LODlevel;
                    auto  lod         = &triMesh->lods[lodIdx];

					if (triMesh != prevMesh || prevLOD != lod)
					{
                        prevMesh    = triMesh;
                        prevLOD     = lod;

						ctx.AddIndexBuffer(triMesh, lodIdx);
						ctx.AddVertexBuffers(
							triMesh,
                            lodIdx,
							{
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							}
						);
					}

                    auto& materials         = MaterialComponent::GetComponent();
                    const auto material     = MaterialComponent::GetComponent()[brush->material];
                    const auto subMeshCount = lod->subMeshes.size();

                    if (material.SubMaterials.empty())
                    {
                        

                        if (material.Textures.size())
                        {
                            DescriptorHeap descHeap;

                            descHeap.Init(
                                ctx,
                                resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0u),
                                &allocator);

                            for (size_t I = 0; I < Min(material.Textures.size(), 4u); I++)
                                descHeap.SetSRV(ctx, I, material.Textures[I]);

                            descHeap.NullFill(ctx);

                            ctx.SetGraphicsDescriptorTable(0, descHeap);
                        }
					    else
                            ctx.SetGraphicsDescriptorTable(0, defaultHeap);

                        constants.textureCount = (uint32_t)material.Textures.size();

                        ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                        ctx.DrawIndexed(lod->GetIndexCount());
                    }
                    else
                    {
                        for (size_t I = 0; I < subMeshCount; I++)
                        {
                            auto& subMesh           = triMesh->GetHighestLoadedLod().subMeshes[I];
                            const auto passMaterial = materials[material.SubMaterials[I]];

                            DescriptorHeap descHeap;

					        descHeap.Init(
						        ctx,
						        resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
						        &allocator);

                            if (passMaterial.Textures.size())
                            {
                                for (size_t I = 0; I < Min(passMaterial.Textures.size(), 4); I++)
                                    descHeap.SetSRV(ctx, I, passMaterial.Textures[I]);

                                descHeap.NullFill(ctx);

                                ctx.SetGraphicsDescriptorTable(0, descHeap);
                            }
					        else
                                ctx.SetGraphicsDescriptorTable(0, defaultHeap);

                            constants.textureCount = (uint32_t)passMaterial.Textures.size();

                            ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
                            ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
                        }
                    }
				}

				// skinned models
				ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS_SKINNED));

				auto& poses = allocator.allocate<EntityPoses>();

				for (const auto& skinnedBrush : data.skinned)
				{
					const auto constants    = skinnedBrush.brush->GetConstants();
					auto* triMesh           = GetMeshResource(skinnedBrush.brush->MeshHandle);

					if (triMesh != prevMesh)
					{
						prevMesh = triMesh;

						ctx.AddIndexBuffer(triMesh, triMesh->GetHighestLoadedLodIdx());
						ctx.AddVertexBuffers(
							triMesh,
                            triMesh->GetHighestLoadedLodIdx(),
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

					auto pose       = skinnedBrush.pose;
					auto skeleton   = pose->Sk;

					for(size_t I = 0; I < pose->JointCount; I++)
						poses[I] = skeleton->IPose[I] * pose->CurrentPose[I];

					ctx.SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, entityConstantBuffer));
					ctx.SetGraphicsConstantBufferView(4, ConstantBufferDataSet(poses, poseBuffer));

                    for (auto& subMesh : triMesh->GetHighestLoadedLod().subMeshes)
                        ctx.DrawIndexed(subMesh.IndexCount, subMesh.BaseIndex);
				}

                ctx.EndEvent_DEBUG();

                ctx.TimeStamp(timeStats, 1u);
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

				data.pointLightHandles          = &sceneDescription.pointLightMaps.GetData().pointLightShadows;

				data.AlbedoTargetObject         = builder.PixelShaderResource(gbuffer.albedo);
				data.NormalTargetObject         = builder.PixelShaderResource(gbuffer.normal);
				data.MRIATargetObject           = builder.PixelShaderResource(gbuffer.MRIA);
				data.depthBufferTargetObject    = builder.PixelShaderResource(depthTarget);
                data.lightMapObject             = builder.ReadTransition(lightPass.indexBufferObject,       DRS_PixelShaderResource);
                data.lightListBuffer            = builder.ReadTransition(lightPass.lightListBuffer,         DRS_PixelShaderResource);
                data.lightLists                 = builder.ReadTransition(lightPass.lightLists,              DRS_PixelShaderResource);

				data.renderTargetObject         = builder.AcquireVirtualResource(
                                                    GPUResourceDesc::RenderTarget(
                                                        builder.GetRenderSystem().GetTextureWH(renderTarget),
                                                        DeviceFormat::R16G16B16A16_FLOAT), FlexKit::DRS_RenderTarget, false);//builder.RenderTarget(renderTarget);

				data.pointLightBufferObject     = builder.ReadTransition(lightPass.lightBufferObject,       DRS_PixelShaderResource);

				data.passConstants              = reserveCB(128 * KILOBYTE);
				data.passVertices               = reserveVB(sizeof(float4) * 6);

                builder.SetDebugName(data.renderTargetObject, "renderTargetObject");

                builder.ReleaseVirtualResource(lightPass.lightLists);
                builder.ReleaseVirtualResource(lightPass.indexBufferObject);
                builder.ReleaseVirtualResource(lightPass.lightListBuffer);
                builder.ReleaseVirtualResource(lightPass.lightBufferObject);
			},
			[camera = sceneDescription.camera, renderTarget, t, timingReadBack = timingReadBack, timeStats = timeStats]
			(TiledDeferredShade& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				PointLightComponent&    lightComponent  = PointLightComponent::GetComponent();
                const auto&             visableLights   = *data.pointLightHandles;

				auto& renderSystem          = resources.renderSystem();
				const auto WH               = resources.renderSystem().GetTextureWH(renderTarget);
				const auto cameraConstants  = GetCameraConstants(camera);
				const auto pointLightCount  = (uint32_t)visableLights.size();

                //if (!pointLightCount)
                //    return;

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

                {
                    uint32_t lightID = 0;
                    for (auto lightHandle : visableLights)
                    {
                        auto& light             = lightComponent[lightHandle];
                        const float3 position   = GetPositionW(light.Position);
                        const auto matrices     = CalculateShadowMapMatrices(position, light.R, t);

                        for (size_t II = 0; II < 6; II++)
                            passConstants.PV[6 * lightID + II] = matrices.PV[II];

                        lightID++;
                    }
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


				const size_t descriptorTableSize = 20 + pointLightCount;

				DescriptorHeap descHeap;
				descHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(0), descriptorTableSize, &allocator);
				descHeap.SetSRV(ctx, 0, resources.GetResource(data.AlbedoTargetObject));
				descHeap.SetSRV(ctx, 1, resources.GetResource(data.MRIATargetObject));
				descHeap.SetSRV(ctx, 2, resources.GetResource(data.NormalTargetObject));
				descHeap.SetSRV(ctx, 4, resources.GetResource(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
                descHeap.SetSRV(ctx, 5, resources.GetResource(data.lightMapObject));
                descHeap.SetStructuredResource(ctx, 6, resources.GetResource(data.lightLists), sizeof(uint2));
                descHeap.SetStructuredResource(ctx, 7, resources.GetResource(data.lightListBuffer), sizeof(uint32_t));
				descHeap.SetStructuredResource(ctx, 8, resources.Transition(data.pointLightBufferObject, DRS_PixelShaderResource, ctx), sizeof(GPUPointLight));


				for (size_t shadowMapIdx = 0; shadowMapIdx < pointLightCount; shadowMapIdx++)
				{
                    auto shadowMap = lightComponent[visableLights[shadowMapIdx]].shadowMap;
					descHeap.SetSRVCubemap(ctx, 10 + shadowMapIdx, shadowMap, DeviceFormat::R32_FLOAT);
				}

				descHeap.NullFill(ctx, descriptorTableSize);

                ctx.ClearRenderTarget(resources.GetResource({ data.renderTargetObject }));

				ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(resources.GetPipelineState(SHADINGPASS));
				ctx.SetPrimitiveTopology(EIT_TRIANGLELIST);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets({ resources.GetResource({ data.renderTargetObject })}, false, {});
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
				ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
				ctx.SetGraphicsDescriptorTable(4, descHeap);

                ctx.BeginEvent_DEBUG("Clustered Shading");

                ctx.TimeStamp(timeStats, 4);
				ctx.Draw(6);
                ctx.TimeStamp(timeStats, 5);

                ctx.EndEvent_DEBUG();

                ctx.ResolveQuery(timeStats, 0, 8, resources.GetObjectResource(timingReadBack), 0);
                ctx.QueueReadBack(timingReadBack);
			});

		return pass;
	}


    /************************************************************************************************/


    ID3D12PipelineState* CreateOITDrawPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain",      "vs_6_0", "assets\\shaders\\OITPass.hlsl");
		auto PShader = RS->LoadShader("PassMain",   "ps_6_0", "assets\\shaders\\OITPass.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    2, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,       3, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Rast_Desc.CullMode                      = D3D12_CULL_MODE_BACK;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = true;
        Depth_Desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;

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
			PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // count
			PSO_Desc.SampleDesc.Count      = 1;
			PSO_Desc.SampleDesc.Quality    = 0;
			PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.IndependentBlendEnable          = true;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlend        = D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend       = D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[1].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[1].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[1].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[1].SrcBlend        = D3D12_BLEND_ZERO;
            PSO_Desc.BlendState.RenderTarget[1].SrcBlendAlpha   = D3D12_BLEND_ZERO;

            PSO_Desc.BlendState.RenderTarget[1].DestBlend       = D3D12_BLEND_INV_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[1].DestBlendAlpha  = D3D12_BLEND_INV_SRC_ALPHA;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateOITDrawAnimatedPSO(RenderSystem* RS)
    {
        return nullptr;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateOITBlendPSO(RenderSystem* RS)
    {
        auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\OITBlend.hlsl");
		auto PShader = RS->LoadShader("BlendMain", "ps_6_0", "assets\\shaders\\OITBlend.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",	    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,    2, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,       3, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

            PSO_Desc.BlendState.IndependentBlendEnable          = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable     = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp         = D3D12_BLEND_OP_ADD;
            PSO_Desc.BlendState.RenderTarget[0].BlendOpAlpha    = D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlend        = D3D12_BLEND_INV_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend       = D3D12_BLEND_SRC_ALPHA;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND_ONE;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
    }


    /************************************************************************************************/


    OITPass& WorldRender::RenderPBR_OITPass(
		UpdateDispatcher&               dispatcher,
		FrameGraph&                     frameGraph,
		const SceneDescription&         sceneDesc,
		const PointLightGatherTask&     pointLightGather,
		ResourceHandle                  depthTarget,
		FrameResourceHandle             renderTarget,
		const ShadowMapPassData&        shadowMaps,
		const LightBufferUpdate&        lightPass,
		ReserveConstantBufferFunction   reserveCB,
		iAllocator*                     allocator)
    {
        return frameGraph.AddNode<OITPass>(
            OITPass{
                pointLightGather,
                lightPass,
                sceneDesc.pointLightMaps,
                shadowMaps,
                sceneDesc.lights.GetData().pointLights,

                reserveCB,

                sceneDesc.PVS.GetData().transparent,
                sceneDesc.camera
            },
			[&](FrameGraphNodeBuilder& builder, OITPass& data)
			{
                const auto WH = builder.GetRenderSystem().GetTextureWH(depthTarget);

                data.depthTarget                = builder.DepthRead(depthTarget);
                data.renderTargetObject         = renderTarget;
                data.accumalatorObject          = builder.AcquireVirtualResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT), DRS_RenderTarget);
                data.counterObject              = builder.AcquireVirtualResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT), DRS_RenderTarget);

                builder.SetDebugName(data.accumalatorObject,    "Accumalator");
                builder.SetDebugName(data.counterObject,        "counterObject");
			},
            [=](OITPass& data, ResourceHandler& resources, Context& ctx, iAllocator& tempAllocator)
            {
                ctx.BeginEvent_DEBUG("OIT");

                const RootSignature&    rootSig     = resources.renderSystem().Library.RSDefault;
                auto&                   materials   = MaterialComponent::GetComponent();

                ctx.SetRootSignature(rootSig);

                TriMeshHandle   previous        = InvalidHandle_t;
                TriMesh*        triMesh         = nullptr;
                MaterialHandle  prevMaterial    = InvalidHandle_t;

                ctx.SetPipelineState(resources.GetPipelineState(OITDRAW));

                CBPushBuffer constantBuffer{
                    data.reserveCB(
                        AlignedSize<Brush::VConstantsLayout>() * data.pvs.size() +
                        AlignedSize<Camera::ConstantBuffer>() )};

                const auto cameraConstantValues = GetCameraConstants(data.camera);
                const ConstantBufferDataSet cameraConstants{ cameraConstantValues, constantBuffer };

                ctx.ClearRenderTarget(resources.GetRenderTarget(data.accumalatorObject));
                ctx.ClearRenderTarget(resources.GetRenderTarget(data.counterObject), float4{ 1, 1, 1, 1 });

                ctx.SetGraphicsConstantBufferView(0, cameraConstants);

                ctx.SetScissorAndViewports({
                        resources.GetRenderTarget(data.accumalatorObject),
                        resources.GetRenderTarget(data.counterObject),
                    });

                ctx.SetRenderTargets(
                    {
                        resources.GetRenderTarget(data.accumalatorObject),
                        resources.GetRenderTarget(data.counterObject),
                    },
                    true,
                    resources.GetRenderTarget(data.depthTarget)
                );

                ctx.BeginEvent_DEBUG("OIT Pass");

                for (auto& draw : data.pvs)
                {
                    const auto mesh         = draw.brush->MeshHandle;
                    const auto material     = draw.brush->material;

                    if (mesh != previous)
                    {
                        previous    = mesh;
                        triMesh     = GetMeshResource(mesh);

                        auto& lod = triMesh->lods[draw.LODlevel];
                        
                        ctx.AddIndexBuffer(
                            triMesh,
                            draw.LODlevel);

                        ctx.AddVertexBuffers(
                            triMesh,
                            draw.LODlevel,
                            {
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                            }
                        );
                    }


                    if (material != prevMaterial)
                    {
                        const auto& textures = materials[material].Textures;

                        DescriptorHeap heap;

                        heap.Init2(ctx, rootSig.GetDescHeap(0), textures.size(), &tempAllocator);

                        for (size_t I = 0; I < textures.size(); ++I)
                            heap.SetSRV(ctx, I, textures[I]);

                        ctx.SetGraphicsDescriptorTable(4, heap);
                    }

                    const auto textureCount     = materials[material].Textures.size();
                    const auto constantValues   = draw.brush->GetConstants();

                    const ConstantBufferDataSet localConstants{ constantValues, constantBuffer };

                    ctx.SetGraphicsConstantBufferView(1, localConstants);

                    for (auto& subMesh : triMesh->GetHighestLoadedLod().subMeshes)
                        ctx.DrawIndexed(
                            subMesh.IndexCount,
                            subMesh.BaseIndex);

                }

                ctx.EndEvent_DEBUG();
                ctx.BeginEvent_DEBUG("OIT Blend");

                ctx.SetPipelineState(resources.GetPipelineState(OITBLEND));

                auto& descHeapLayout = rootSig.GetDescHeap(0);
                DescriptorHeap descHeap;

                descHeap.Init2(ctx, descHeapLayout, 2, &tempAllocator);
                descHeap.SetSRV(ctx, 0, resources.PixelShaderResource(data.accumalatorObject, ctx));
                descHeap.SetSRV(ctx, 1, resources.PixelShaderResource(data.counterObject, ctx));

                ctx.SetGraphicsDescriptorTable(4, descHeap);

                ctx.SetRenderTargets(
                    {
                        resources.GetRenderTarget(data.renderTargetObject),
                    },
                    false);

                ctx.Draw(6);

                ctx.EndEvent_DEBUG();
                ctx.EndEvent_DEBUG();
            });
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateInitialLumananceLevel(RenderSystem* renderSystem)
    {
        return nullptr;
    }

    ID3D12PipelineState* CreateLumananceLevel(RenderSystem* renderSystem)
    {
        return nullptr;
    }


    /************************************************************************************************/

    // TODO: Do actual tone mapping
    ToneMap& WorldRender::RenderPBR_ToneMapping(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
                FrameResourceHandle             source,
			    ResourceHandle                  target,
			    ReserveConstantBufferFunction   reserveCB,
			    ReserveVertexBufferFunction     reserveVB,
			    float                           t,
			    iAllocator*                     allocator)
    {
        // Does a simple resource copy for the moment
        return frameGraph.AddNode<ToneMap>(
            ToneMap{},
			[&](FrameGraphNodeBuilder& builder, ToneMap& data)
			{
                const auto WH = frameGraph.GetRenderSystem().GetTextureWH(target) / 2;

                data.outputTarget   = builder.CopyDest(target);
                data.sourceTarget   = builder.ReadTransition(source, DRS_CopySrc);

                //data.outputTarget = builder.RenderTarget(target);
                //data.sourceTarget   = builder.ReadTransition(source, DRS_NonPixelShaderResource);
                //data.temp1Buffer    = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture(WH, DeviceFormat::R32_FLOAT), DRS_UAV);
                //data.temp2Buffer    = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture(WH, DeviceFormat::R32_FLOAT), DRS_UAV);

                builder.ReleaseVirtualResource(source);
            },
			[]
			(ToneMap& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {

                ctx.CopyResource(resources.GetResource(data.outputTarget), resources.GetResource(data.sourceTarget));

                /*
                auto& rootSig = resources.renderSystem().Library.RSDefault;

                DescriptorHeap heap1{};
                heap1.Init2(ctx, rootSig.GetDescHeap(0), 3, &allocator);

                DescriptorHeap heap2{};
                heap2.Init2(ctx, rootSig.GetDescHeap(1), 3, &allocator);

                ID3D12PipelineState* createInitialLevel = nullptr;
                ID3D12PipelineState* createLumananceLevel = nullptr;

                const uint2 WH = resources.GetTextureWH(data.sourceTarget);

                ctx.SetComputeRootSignature(rootSig);
                ctx.SetComputeDescriptorTable(0, heap1);
                ctx.SetComputeDescriptorTable(1, heap2);
                ctx.Dispatch(createInitialLevel, { WH, 0 });

                for (uint I = 0; I < log2(ceil(WH.Max())); I++)
                {

                    ctx.Dispatch(createLumananceLevel, { WH, 0 });
                }
                */
            });
    }


    /************************************************************************************************/


    void WorldRender::DEBUGVIS_BVH(
			UpdateDispatcher&               dispatcher,
			FrameGraph&                     frameGraph,
            SceneBVH&                       bvh,
            CameraHandle                    camera,
			ResourceHandle                  renderTarget,
			ReserveConstantBufferFunction   reserveCB,
			ReserveVertexBufferFunction     reserveVB,
            BVHVisMode                      mode,
			iAllocator*                     allocator)
    {
        struct DebugVisDesc
        {
            ReserveConstantBufferFunction   reserveCB;
            ReserveVertexBufferFunction     reserveVB;

            FrameResourceHandle             renderTargetObject;
        };

        struct Vertex {
            float4 Min;
            float4 Max;
            float3 Color;
        };


        frameGraph.AddNode<DebugVisDesc>(
            DebugVisDesc{
                reserveCB,
                reserveVB,
            },
            [&](FrameGraphNodeBuilder& builder, DebugVisDesc& desc)
            {
                desc.renderTargetObject = builder.RenderTarget(renderTarget);
            },
            [=, &bvh = bvh](DebugVisDesc& desc, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                auto drawElementsState  = resources.GetPipelineState(DEBUG_DrawBVH);

                const auto vertexCount      = bvh.elements.size() + bvh.nodes.size();
                const auto vertexBufferSize = vertexCount * sizeof(Vertex);
                auto constantBuffer         = desc.reserveCB(1024);
                auto vertexBuffer           = desc.reserveVB(vertexBufferSize);
                auto cameraConstants        = GetCameraConstants(camera);

                struct {
                    float4x4 proj;
                    float4x4 view;
                }   constantData = {
                        cameraConstants.Proj,
                        cameraConstants.View,
                };

                auto& VisibilityComponent = SceneVisibilityComponent::GetComponent();

                Vector<Vertex> verts{ &allocator };

                for (auto& bv : bvh.elements)
                {
                    Vertex v;
                    AABB aabb   = VisibilityComponent[bv.handle].GetAABB();
                    v.Min       = aabb.Min;
                    v.Max       = aabb.Max;
                    v.Color     = WHITE;

                    if(mode == BVHVisMode::BoundingVolumes || mode == BVHVisMode::Both)
                        verts.push_back(v);
                }

                for (auto& bv : bvh.nodes)
                {
                    Vertex v;
                    auto aabb   = bv.boundingVolume;
                    v.Min       = aabb.Min;
                    v.Max       = aabb.Max;
                    v.Color     = BLUE;

                    if (mode == BVHVisMode::BVH || mode == BVHVisMode::Both)
                        verts.push_back(v);
                }

                auto vertices       = VertexBufferDataSet{ verts.data(), verts.size() * sizeof(Vertex), vertexBuffer };
                auto constants      = ConstantBufferDataSet{ constantData, constantBuffer };
                auto renderTargets  = { resources.GetRenderTarget(desc.renderTargetObject) };

                ctx.SetRenderTargets(renderTargets, false);
                ctx.SetScissorAndViewports(renderTargets);

                ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
                ctx.SetPipelineState(drawElementsState);

                ctx.SetPrimitiveTopology(EIT_POINT);
                ctx.SetGraphicsConstantBufferView(0, constants);
                ctx.SetVertexBuffers({ vertices });

                ctx.Draw(verts.size());
            });
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
			PSO_Desc.pRootSignature                         = RS->Library.RSDefault;
			PSO_Desc.VS                                     = VShader;
			PSO_Desc.GS                                     = GShader;
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


	ID3D12PipelineState* CreateShadowMapAnimatedPass(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VS_Skinned_Main",    "vs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");
		auto GShader = RS->LoadShader("GS_Main",            "gs_6_0", "assets\\shaders\\CubeMapShadowMapping.hlsl");

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
		Rast_Desc.CullMode = D3D12_CULL_MODE_BACK;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                         = RS->Library.RSDefault;
			PSO_Desc.VS                                     = VShader;
			PSO_Desc.GS                                     = GShader;
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
		static const auto M1 = DirectX::XMMatrixRotationY((float) pi / 2.0f) * DirectX::XMMatrixRotationX((float)pi); // left Face
        static const auto M2 = DirectX::XMMatrixRotationY((float)-pi / 2.0f) * DirectX::XMMatrixRotationX((float)pi); // right

        static const auto M3 = DirectX::XMMatrixRotationX((float) pi / 2.0f); // Top Face
        static const auto M4 = DirectX::XMMatrixRotationX((float)-pi / 2.0f); // Bottom Face

        static const auto M5 = Float4x4ToXMMATIRX(Quaternion2Matrix(Quaternion(0, 180, 180)));
        static const auto M6 = DirectX::XMMatrixRotationY(0) * DirectX::XMMatrixRotationZ((float)pi); // backward

		static const XMMATRIX ViewOrientations[] = {
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
			const XMMATRIX ViewI            = ViewOrientations[I] * DirectX::XMMatrixTranslationFromVector(pos);
			const XMMATRIX View             = DirectX::XMMatrixInverse(nullptr, ViewI);
			const XMMATRIX perspective      = DirectX::XMMatrixPerspectiveFovRH(float(-pi/2), 1, 0.1f, r);
			const XMMATRIX PV               = XMMatrixTranspose(perspective) * XMMatrixTranspose(View);

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


    /************************************************************************************************/


    AcquireShadowMapTask& AcquireShadowMaps(UpdateDispatcher& dispatcher, RenderSystem& renderSystem, MemoryPoolAllocator& RTPool, PointLightUpdate& pointLightUpdate)
    {
        return dispatcher.Add<AcquireShadowMapResources>(
            [&](UpdateDispatcher::UpdateBuilder& builder, AcquireShadowMapResources& data)
            {
                builder.AddInput(pointLightUpdate);
            },
            [&dirtyList = pointLightUpdate.GetData().dirtyList, &shadowMapAllocator = RTPool, &renderSystem = renderSystem]
            (AcquireShadowMapResources& data, iAllocator& threadAllocator)
            {
                auto& lights = PointLightComponent::GetComponent();

                for (auto lightHandle : dirtyList)
                {
                    auto& light = lights[lightHandle];

                    if (light.shadowMap != InvalidHandle_t) {
                        shadowMapAllocator.Release(light.shadowMap, false, false);
                        renderSystem.ReleaseResource(light.shadowMap);
                    }

                    auto [shadowMap, _] = shadowMapAllocator.Acquire(GPUResourceDesc::DepthTarget({ 512, 512 }, DeviceFormat::D32_FLOAT, 6), false);
                    renderSystem.SetDebugName(shadowMap, "Shadow Map");

                    light.shadowMap = shadowMap;
                }

            });
    }


    /************************************************************************************************/


	ShadowMapPassData& ShadowMapPass(
		FrameGraph&                             frameGraph,
        const SceneDescription                  sceneDesc,
        ReserveConstantBufferFunction           reserveCB,
        ReserveVertexBufferFunction             reserveVB,
        static_vector<AdditionalShadowMapPass>& additional,
		const double                            t,
		iAllocator*                             allocator)
	{
		auto& shadowMapPass = allocator->allocate<ShadowMapPassData>(ShadowMapPassData{ allocator->allocate<Vector<TemporaryFrameResourceHandle>>(allocator) });

		frameGraph.AddNode<LocalShadowMapPassData>(
				LocalShadowMapPassData{
                    sceneDesc.pointLightMaps.GetData().pointLightShadows,
					shadowMapPass,
					reserveCB,
                    reserveVB,
                    additional
				},
				[&](FrameGraphNodeBuilder& builder, LocalShadowMapPassData& data)
				{
                    builder.AddDataDependency(sceneDesc.cameras);
                    builder.AddDataDependency(sceneDesc.PVS);
                    builder.AddDataDependency(sceneDesc.pointLightMaps);
                    builder.AddDataDependency(sceneDesc.shadowMapAcquire);
				},
				[=](LocalShadowMapPassData& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
				{
                    auto& visableLights = data.pointLightShadows;
                    auto& pointLights   = PointLightComponent::GetComponent();
                    
                    Vector<ResourceHandle>          shadowMaps      { &allocator, visableLights.size() };
                    Vector<ConstantBufferDataSet>   passConstant2   { &allocator, visableLights.size() };

                    for (size_t I = 0; I < visableLights.size(); I++)
                    {
                        auto& lightHandle   = visableLights[I];
                        auto& pointLight    = pointLights[lightHandle];

                        if (pointLight.state == LightStateFlags::Dirty && pointLight.shadowMap != InvalidHandle_t)
                        {
                            pointLight.state = LightStateFlags::Clean;

                            const auto& visibilityComponent = SceneVisibilityComponent::GetComponent();
                            const auto& visables            = pointLight.shadowState->visableObjects;
					        const auto depthTarget          = pointLight.shadowMap;
                            const float3 pointLightPosition = GetPositionW(pointLight.Position);

					        ctx.ClearDepthBuffer(depthTarget, 1.0f);

					        if (!visables.size())
						        return;


                            PVS                 brushes         { &allocator, visables.size() };
                            Vector<GameObject*> animatedBrushes { &allocator, visables.size() };

                            for (auto& visable : visables)
                            {
                                auto entity = visibilityComponent[visable].entity;

                                Apply(*entity,
                                    [&](BrushView& view)
                                    {
                                        if (!view.GetBrush().Skinned)
                                            PushPV(view.GetBrush(), brushes, pointLightPosition);
                                        else
                                            animatedBrushes.push_back(entity);
                                    });
                            }

                            struct PassConstants
                            {
                                struct PlaneMatrices
                                {
                                    float4x4 ViewI;
                                    float4x4 PV;
                                }matrices[6];
                            };

                            struct PoseConstants
                            {
                                float4x4 M[256];
                            };


                            CBPushBuffer passConstantBuffer = data.reserveCB(
                                AlignedSize<PassConstants>());

                            CBPushBuffer localConstantBuffer        = data.reserveCB(AlignedSize<Brush::VConstantsLayout>() * visables.size());
                            CBPushBuffer animatedConstantBuffer     = data.reserveCB(AlignedSize<Brush::VConstantsLayout>() * animatedBrushes.size() + AlignedSize<PoseConstants>() * animatedBrushes.size());

                            for (auto& brush : brushes)
                                ConstantBufferDataSet{ brush->GetConstants(), localConstantBuffer };

					        auto constants  = CreateCBIterator<Brush::VConstantsLayout>(localConstantBuffer);
					        auto PSO        = resources.GetPipelineState(SHADOWMAPPASS);

					        ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
					        ctx.SetPipelineState(PSO);
					        ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

					        const float3 Position   = FlexKit::GetPositionW(pointLight.Position);
					        const auto matrices     = CalculateShadowMapMatrices(Position, pointLight.R, t);

					        PassConstants passConstantData;

					        for(size_t I = 0; I < 6; I++)
					        {
						        passConstantData.matrices[I].ViewI  = matrices.ViewI[I];
						        passConstantData.matrices[I].PV     = matrices.PV[I];
					        }

					        ConstantBufferDataSet passConstants{ passConstantData, passConstantBuffer };

                            passConstant2.push_back(passConstants);

                            const DepthStencilView_Options DSV_desc = {
                                0, 0,
                                depthTarget };


					        ctx.SetScissorAndViewports({ depthTarget });
					        ctx.SetRenderTargets2({}, 0, DSV_desc);

					        ctx.SetGraphicsConstantBufferView(0, passConstants);

                            ctx.BeginEvent_DEBUG("Shadow Map Pass");

                            if (auto state = resources.renderSystem().GetObjectState(depthTarget); state != DRS_DEPTHBUFFERWRITE)
                                ctx.AddResourceBarrier(depthTarget, state, DRS_DEPTHBUFFERWRITE);

					        for(size_t brushIdx = 0; brushIdx < brushes.size(); brushIdx++)
					        {
                                auto& brush         = brushes[brushIdx];
                                const auto lodLevel = brush.LODlevel;
						        auto* const triMesh = GetMeshResource(brush->MeshHandle);
                                auto& lod           = triMesh->lods[lodLevel];

						        ctx.SetGraphicsConstantBufferView(1, constants[brushIdx]);

						        ctx.AddIndexBuffer(triMesh, lodLevel);
						        ctx.AddVertexBuffers(triMesh,
                                    lodLevel,
							        { VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });

                                
                                ctx.DrawIndexedInstanced(triMesh->GetHighestLoadedLod().GetIndexCount());
					        }


                            auto PSOAnimated = resources.GetPipelineState(SHADOWMAPANIMATEDPASS);
                            ctx.SetPipelineState(PSOAnimated);
                            ctx.SetScissorAndViewports({ depthTarget });
                            ctx.SetRenderTargets2({}, 0, DSV_desc);
                            ctx.SetGraphicsConstantBufferView(0, passConstants);
                            ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

                            for (auto& brush : animatedBrushes)
                            {
                                Apply(*brush,
                                    [&](BrushView&      brushView,
                                        SkeletonView&   poseView)
                                    {
                                        auto& draw      = brushView.GetBrush();
                                        auto& pose      = poseView.GetPoseState();
                                        auto& skeleton  = *pose.Sk;

                                        struct poses
                                        {
                                            float4x4 M[256];
                                        }poseTemp;

                                        const size_t end = pose.JointCount;
                                        for (size_t I = 0; I < end; ++I)
                                            poseTemp.M[I] = skeleton.IPose[I] * pose.CurrentPose[I];

                                        const auto constants        = ConstantBufferDataSet{ draw.GetConstants(), animatedConstantBuffer };
                                        const auto poseConstants    = ConstantBufferDataSet{ poseTemp, animatedConstantBuffer };

                                        ctx.SetGraphicsConstantBufferView(1, constants);
                                        ctx.SetGraphicsConstantBufferView(2, poseConstants);
                                        ctx.SetGraphicsConstantBufferView(3, poseConstants);

                                        auto triMesh            = GetMeshResource(draw.MeshHandle);
                                        const auto lodLevelIdx  = triMesh->GetHighestLoadedLodIdx();
                                        const auto lodLevel     = triMesh->GetHighestLoadedLod();

                                        ctx.AddIndexBuffer(triMesh, lodLevelIdx);
						                ctx.AddVertexBuffers(triMesh,
                                            lodLevelIdx,
							                {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
                                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
                                            });

                                
                                        ctx.DrawIndexedInstanced(triMesh->GetHighestLoadedLod().GetIndexCount());
                                    });
                            }

                            shadowMaps.push_back(depthTarget);

                            ctx.EndEvent_DEBUG();
                        }
                    }

                    if(shadowMaps.size())
                    {
                        for (auto& additionalPass : data.additionalShadowPass)
                        {
                            ctx.BeginEvent_DEBUG("Additional Shadow Map Pass");

                            additionalPass(
                                data.reserveCB,
                                data.reserveVB,
                                passConstant2.data(),
                                shadowMaps.data(),
                                shadowMaps.size(),
                                resources,
                                ctx,
                                allocator);

                            ctx.EndEvent_DEBUG();
                        }

                        for (auto target : shadowMaps)
                            ctx.AddResourceBarrier(target, DRS_DEPTHBUFFERWRITE, DRS_PixelShaderResource);
                    }
				});

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
