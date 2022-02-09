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


    WorldRender::WorldRender(RenderSystem& RS_IN, TextureStreamingEngine& IN_streamingEngine, iAllocator* persistent) :
			renderSystem                { RS_IN },
			enableOcclusionCulling	    { false	},

            UAVPool                     { renderSystem, 512 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVBuffer, persistent },
            UAVTexturePool              { renderSystem, 256 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVTextures, persistent },
            RTPool                      { renderSystem, 2048 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVTextures | DeviceHeapFlags::RenderTarget, persistent },

            timeStats                   { renderSystem.CreateTimeStampQuery(256) },
            timingReadBack              { renderSystem.CreateReadBackBuffer(512) }, 

			streamingEngine		        { IN_streamingEngine },

            lightingEngine              { renderSystem, *persistent, EGITECHNIQUE::DISABLE },
            shadowMapping               { renderSystem, *persistent },
            clusteredRender             { renderSystem, *persistent },
            transparency                { renderSystem }
	{
		RS_IN.RegisterPSOLoader(FORWARDDRAW,			        { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,		  });
		RS_IN.RegisterPSOLoader(FORWARDDRAWINSTANCED,	        { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO });

		RS_IN.RegisterPSOLoader(LIGHTPREPASS,			        { &RS_IN.Library.ComputeSignature,  CreateLightPassPSO			  });
		RS_IN.RegisterPSOLoader(DEPTHPREPASS,			        { &RS_IN.Library.RS6CBVs4SRVs,      CreateDepthPrePassPSO         });

		RS_IN.RegisterPSOLoader(ENVIRONMENTPASS,                { &RS_IN.Library.RS6CBVs4SRVs,      CreateEnvironmentPassPSO      });

                
		RS_IN.RegisterPSOLoader(BILATERALBLURPASSHORIZONTAL,    { &RS_IN.Library.RSDefault, CreateBilaterialBlurHorizontalPSO });
		RS_IN.RegisterPSOLoader(BILATERALBLURPASSVERTICAL,      { &RS_IN.Library.RSDefault, CreateBilaterialBlurVerticalPSO   });

        RS_IN.RegisterPSOLoader(ZPYRAMIDBUILDLEVEL,             { &RS_IN.Library.RSDefault, CreateBuildZLayer });
        RS_IN.RegisterPSOLoader(DEPTHCOPY,                      { &RS_IN.Library.RSDefault, CreateDepthBufferCopy });


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

        pendingGPUTasks.emplace_back(
            [&](FrameGraph& frameGraph, auto& resources)
            {
                lightingEngine.Init(frameGraph, resources.reserveCB);
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
        ProfileFunction();

        auto&       scene           = drawSceneDesc.scene;
        const auto  camera          = drawSceneDesc.camera;
        auto&       gbuffer         = drawSceneDesc.gbuffer;
        const auto  t               = drawSceneDesc.t;

        auto&       depthTarget     = targets.DepthTarget;
        auto        renderTarget    = targets.RenderTarget;

        auto& passes = GatherScene(dispatcher, &scene, camera, temporary);

        passes.AddInput(drawSceneDesc.transformDependency);
        passes.AddInput(drawSceneDesc.cameraDependency);

        auto& pointLightGather      = scene.GetPointLights(dispatcher, temporary);
        auto& sceneBVH              = scene.UpdateSceneBVH(dispatcher, drawSceneDesc.transformDependency, temporary);
        auto& visablePointLights    = scene.GetVisableLights(dispatcher, camera, sceneBVH, temporary);
        auto& pointLightUpdate      = scene.UpdatePointLights(dispatcher, sceneBVH, visablePointLights, temporary, persistent);
        auto& shadowMaps            = shadowMapping.AcquireShadowMaps(dispatcher, frameGraph.GetRenderSystem(), RTPool, pointLightUpdate);

        LoadLodLevels(dispatcher, passes, drawSceneDesc.camera, renderSystem, *persistent);

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
            passes,
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

        PassData data = {
            .passes     = passes,
            .reserveCB  = reserveCB,
            .reserveVB  = reserveVB
        };

        for (auto& task : pendingGPUTasks)
            task(frameGraph, data);

        pendingGPUTasks.clear();

        ClearGBuffer(gbuffer, frameGraph);

        auto& entityConstants =
            BuildEntityConstantsBuffer(
                frameGraph,
                dispatcher,
                passes,
                drawSceneDesc.reserveCB,
                temporary);

        auto& occlutionConstants =
            OcclusionCulling(
                dispatcher,
                frameGraph,
                entityConstants,
                passes,
                camera,
                drawSceneDesc.reserveCB,
                depthTarget,
                temporary);

        auto& gbufferPass =
            clusteredRender.FillGBuffer(
                dispatcher,
                frameGraph,
                passes,
                camera,
                gbuffer,
                depthTarget.Get(),
                reserveCB,
                temporary);

        for (auto& pass : drawSceneDesc.additionalGbufferPasses)
            pass();

        auto& lightPass =
            clusteredRender.UpdateLightBuffers(
                dispatcher,
                frameGraph,
                camera,
                scene,
                sceneDesc.pointLightMaps,
                depthTarget.Get(),
                reserveCB,
                temporary,
                drawSceneDesc.debugDisplay != DebugVisMode::ClusterVIS);

        auto& shadowMapPass =
            shadowMapping.ShadowMapPass(
                frameGraph,
                visablePointLights,
                drawSceneDesc.cameraDependency,
                passes,
                shadowMaps,
                reserveCB,
                reserveVB,
                drawSceneDesc.additionalShadowPasses,
                t,
                &temporary);

        /*
        auto& updateVolumes =
            lightingEngine.UpdateVoxelVolumes(
                dispatcher,
                frameGraph,
                camera,
                depthTarget.Get(),
                reserveCB,
                temporary);
        */

        auto& shadingPass =
            clusteredRender.ClusteredShading(
                dispatcher,
                frameGraph,
                sceneDesc.pointLightMaps,
                pointLightGather,
                gbufferPass,
                depthTarget.Get(),
                renderTarget,
                shadowMapPass,
                lightPass,
                reserveCB, reserveVB,
                t,
                temporary);

        lightingEngine.RayTrace(
                dispatcher,
                frameGraph,
                camera,
                sceneDesc.passes,
                depthTarget.Get(),
                shadingPass.renderTargetObject,
                gbuffer,
                reserveCB,
                temporary);

        auto& OIT_pass =
            transparency.OIT_WB_Pass(
                dispatcher,
                frameGraph,
                passes,
                camera,
                depthTarget.Get(),
                reserveCB,
                temporary);

        auto& OIT_blend =
            transparency.OIT_WB_Blend(
                dispatcher,
                frameGraph,
                OIT_pass,
                shadingPass.renderTargetObject,
                temporary);

        auto& toneMapped =
            RenderPBR_ToneMapping(
                dispatcher,
                frameGraph,
                shadingPass.renderTargetObject,
                renderTarget,
                reserveCB,
                reserveVB,
                drawSceneDesc.dt,
                temporary);


        if (drawSceneDesc.debugDisplay == DebugVisMode::ClusterVIS)
        {
            clusteredRender.DEBUGVIS_DrawLightBVH(
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
            clusteredRender.DEBUGVIS_BVH(
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

        clusteredRender.ReleaseFrameResources(
            frameGraph,
            lightPass,
            shadingPass);

        return DrawOutputs{
                    passes,
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
        GatherPassesTask&               passes,
        ReserveConstantBufferFunction&  reserveConstants,
        iAllocator&                     allocator)
    {
        return frameGraph.AddNode<EntityConstants>(
            EntityConstants
            {
                CreateOnceReserveBuffer(reserveConstants, &allocator),
                passes
            },
            [&](FrameGraphNodeBuilder& builder, EntityConstants& data) {},
            [](EntityConstants& data, const ResourceHandler& resourecs, Context& ctx, iAllocator& allocator)
            {
                auto& constantBuffer    = data.GetConstants();
                auto& brushes           = data.passes.GetData().solid;

                for (auto& brush : brushes)
                    constantBuffer.Push(brush->GetConstants());
            });
    }


    /************************************************************************************************/


    OcclusionCullingResults& WorldRender::OcclusionCulling(
                UpdateDispatcher&               dispatcher,
                FrameGraph&                     frameGraph,
                EntityConstants&                entityConstants,
                GatherPassesTask&               passes,
                CameraHandle                    camera,
                ReserveConstantBufferFunction&  reserveConstants,
                DepthBuffer&                    depthBuffer,
                ThreadSafeAllocator&            temporary)
    {
        auto& occlusion = frameGraph.AddNode<OcclusionCullingResults>(
            OcclusionCullingResults{
                passes,
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
                ProfileFunction();

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
                const auto& brushes         = data.passes.GetData().solid;
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
        GatherPassesTask&               passes,
        const ResourceHandle            depthBufferTarget,
        ReserveConstantBufferFunction   reserveConsantBufferSpace,
        iAllocator*                     allocator)
    {
        const size_t MaxEntityDrawCount = 1000;

        auto& pass = frameGraph.AddNode<DepthPass>(
            passes.GetData().solid,
            [&, camera](FrameGraphNodeBuilder& builder, DepthPass& data)
            {
                const size_t localBufferSize = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

                data.entityConstantsBuffer  = std::move(reserveConsantBufferSpace(sizeof(ForwardDrawConstants) * MaxEntityDrawCount));
                data.passConstantsBuffer    = std::move(reserveConsantBufferSpace(2048));
                data.depthBufferObject      = builder.DepthTarget(depthBufferTarget);
                data.depthPassTarget        = depthBufferTarget;

                builder.AddDataDependency(passes);
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


    ID3D12PipelineState* CreateInitialLumananceLevel(RenderSystem* renderSystem)
    {
        return nullptr;
    }

    ID3D12PipelineState* CreateLumananceLevel(RenderSystem* renderSystem)
    {
        return nullptr;
    }


    /************************************************************************************************/

    // TODO(@RobertMay): Do actual tone mapping
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
            },
			[]
			(ToneMap& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
            {
                ProfileFunction();

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
