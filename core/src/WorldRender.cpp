#include "AnimationRendering.hpp"
#include "TextureStreamingUtilities.hpp"
#include "WorldRender.hpp"

namespace FlexKit
{	/************************************************************************************************/


	LoadPipelineStateRes CreateLightPassPSO(RenderSystem* RS, iAllocator& allocator)
	{
		auto lightPassShader = RS->LoadShader("tiledLightCulling", "cs_6_0", "assets\\shaders\\lightPass.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
		PSO_desc.CS				= lightPassShader;
		PSO_desc.pRootSignature = *RS->Library.ComputeSignature;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, RS->Library.ComputeSignature };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateForwardDrawPSO(RenderSystem* RS, iAllocator&)
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
			PSO_Desc.pRootSignature        = *RS->Library.RS6CBVs4SRVs;
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
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "ForwardDraw");

		return { PSO, RS->Library.RS6CBVs4SRVs };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateDepthPrePassPSO(RenderSystem* RS, iAllocator& allocator)
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
			PSO_Desc.pRootSignature        = *RS->Library.RS6CBVs4SRVs;
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

		SETDEBUGNAME(PSO, "DepthPrePass");

		return { PSO, RS->Library.RS6CBVs4SRVs };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateBilaterialBlurHorizontalPSO(RenderSystem* RS, iAllocator& allocator)
	{
		auto VShader = RS->LoadShader("BlurVShader",				"vs_6_0", "assets\\shaders\\BilateralBlur.hlsl");
		auto PShader = RS->LoadShader("BilateralBlurHorizontal_PS",	"ps_6_0", "assets\\shaders\\BilateralBlur.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = *RS->Library.RSDefault;
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

		SETDEBUGNAME(PSO, "HorizontalBlur");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateBilaterialBlurVerticalPSO(RenderSystem* RS, iAllocator& allocator)
	{
		auto VShader = RS->LoadShader("BlurVShader",				"vs_6_0", "assets\\shaders\\BilateralBlur.hlsl");
		auto PShader = RS->LoadShader("BilateralBlurVertical_PS",	"ps_6_0", "assets\\shaders\\BilateralBlur.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	                = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	                = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = *RS->Library.RSDefault;
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

		SETDEBUGNAME(PSO, "VerticalBlur");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateEnvironmentPassPSO(RenderSystem* RS, iAllocator& allocator)
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
			PSO_Desc.pRootSignature        = *RS->Library.RSDefault;
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

		SETDEBUGNAME(PSO, "EnvironmentalLightingPass");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateForwardDrawInstancedPSO(RenderSystem* RS, iAllocator& allocator)
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
			PSO_Desc.pRootSignature        = *RS->Library.RS6CBVs4SRVs;
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
		if (FAILED(HR))
			return {};

		SETDEBUGNAME(PSO, "DrawFlatWhite");

		return { PSO, RS->Library.RS6CBVs4SRVs };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateBuildZLayer(RenderSystem* RS, iAllocator& allocator)
	{
		Shader computeShader = RS->LoadShader("GenerateZLevel", "cs_6_0", R"(assets\shaders\HZB.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			*RS->Library.RSDefault,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateDepthBufferCopy(RenderSystem* RS, iAllocator& allocator)
	{
		Shader computeShader = RS->LoadShader("GenerateZLevel", "cs_6_0", R"(assets\shaders\HZB.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			*RS->Library.RSDefault,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateOcclusionDrawPSO(RenderSystem* RS, iAllocator& allocator)
	{
		FK_ASSERT(0);

		return { nullptr, nullptr };

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
			PSO_Desc.pRootSignature        = *RS->Library.RS6CBVs4SRVs;
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

		SETDEBUGNAME(PSO, "OcclusionCulling");

		return { PSO, RS->Library.RS6CBVs4SRVs };
	}


	/************************************************************************************************/


	LoadPipelineStateRes CreateTexture2CubeMapIrradiancePSO(RenderSystem* RS, iAllocator& allocator)
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
			PSO_Desc.pRootSignature        = *RS->Library.RSDefault;
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

		SETDEBUGNAME(PSO, "Texture2CubeMapIrradiance");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/

	
	LoadPipelineStateRes CreateTexture2CubeMapGGXPSO(RenderSystem* RS, iAllocator& allocator)
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
			PSO_Desc.pRootSignature        = *RS->Library.RSDefault;
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

		SETDEBUGNAME(PSO, "Texture2CubeMapGGX");

		return { PSO, RS->Library.RSDefault };
	}


	/************************************************************************************************/


	CBPushBuffer& BrushConstants::GetConstantBuffer(size_t IN_reservationSize)
	{
		if (reservationSize == -1)
			reservationSize = IN_reservationSize;
		else
			FK_ASSERT(reservationSize == IN_reservationSize);

		const auto bufferSize = IN_reservationSize * AlignedSize<Brush::VConstantsLayout>();
		return getConstantBuffer(bufferSize);
	}


	CBPushBuffer& BrushConstants::GetConstantBuffer()
	{
		FK_ASSERT(reservationSize != -1);

		const auto bufferSize = reservationSize * AlignedSize<Brush::VConstantsLayout>();
		return getConstantBuffer(bufferSize);
	}


	/************************************************************************************************/


	size_t GetRTPoolSize(const AvailableFeatures& features, const uint2 WH)
	{
		if (features.resourceHeapTier == ResourceHeapTier::HeapTier1)
			return 512 * 3 * MEGABYTE;
		else
			return 512 * 2 * MEGABYTE;
	}


	/************************************************************************************************/


	WorldRender::WorldRender(RenderSystem& IN_renderSystem, TextureStreamingEngine& IN_streamingEngine, iAllocator* persistent, const WorldRenderOptions& options, const PoolSizes& poolSizes) :
			renderSystem				{ IN_renderSystem },

			UAVPool						{ renderSystem, poolSizes.UAVPoolByteSize, DefaultBlockSize, DeviceHeapFlags::UAVBuffer, persistent },
			RTPool						{ renderSystem, poolSizes.RTPoolByteSize, DefaultBlockSize,
											renderSystem.features.resourceHeapTier == ResourceHeapTier::HeapTier2 ?
												DeviceHeapFlags::UAVTextures | DeviceHeapFlags::RenderTarget : DeviceHeapFlags::RenderTarget, persistent },

			UAVTexturePool				{ renderSystem, poolSizes.UAVTexturePoolByteSize, DefaultBlockSize, DeviceHeapFlags::UAVTextures, persistent },

			timeStats					{ renderSystem.CreateTimeStampQuery(256) },
			timingReadBack				{ renderSystem.CreateReadBackBuffer(512) }, 

			streamingEngine				{ IN_streamingEngine },

			lightingEngine				{ renderSystem, *persistent, options.GI },
			shadowMapping				{ renderSystem, *persistent },
			clusteredRender				{ renderSystem, *persistent },
			transparency				{ renderSystem, *persistent },
			passHistories				{ *persistent }
	{
		FlexKit::DesciptorHeapLayout layout{};
		layout.SetParameterAsSRV(0, 0, 2, 0);
		layout.SetParameterAsShaderUAV(1, 1, 1, 0);

		RootSignatureBuilder builder{ persistent };
		builder.AllowIA = true;
		builder.SetParameterAsDescriptorTable(0, layout);
		builder.SetParameterAsUAV(1, 0, 0, PIPELINE_DEST_ALL);
		builder.SetParameterAsUINT(2, 16, 0, 0);
		rootSignatureToneMapping = builder.Build(renderSystem, *persistent);

		renderSystem.RegisterPSOLoader(FORWARDDRAW,						CreateForwardDrawPSO);
		renderSystem.RegisterPSOLoader(FORWARDDRAWINSTANCED,			CreateForwardDrawInstancedPSO);

		renderSystem.RegisterPSOLoader(LIGHTPREPASS,					CreateLightPassPSO);
		renderSystem.RegisterPSOLoader(DEPTHPREPASS,					CreateDepthPrePassPSO);

		renderSystem.RegisterPSOLoader(ENVIRONMENTPASS,					CreateEnvironmentPassPSO);

		renderSystem.RegisterPSOLoader(BILATERALBLURPASSHORIZONTAL,		CreateBilaterialBlurHorizontalPSO);
		renderSystem.RegisterPSOLoader(BILATERALBLURPASSVERTICAL,		CreateBilaterialBlurVerticalPSO);

		renderSystem.RegisterPSOLoader(ZPYRAMIDBUILDLEVEL,				CreateBuildZLayer);
		renderSystem.RegisterPSOLoader(DEPTHCOPY,						CreateDepthBufferCopy);

		renderSystem.RegisterPSOLoader(AVERAGELUMINANCE_BLOCK,			{ this, &WorldRender::CreateAverageLumanceLocal });
		renderSystem.RegisterPSOLoader(AVERAGELUMANANCE_GLOBAL,			{ this, &WorldRender::CreateAverageLumanceGlobal });
		renderSystem.RegisterPSOLoader(TONEMAP,							{ this, &WorldRender::CreateToneMapping });

		renderSystem.QueuePSOLoad(GBUFFERPASS);
		renderSystem.QueuePSOLoad(GBUFFERPASS_SKINNED);
		renderSystem.QueuePSOLoad(DEPTHPREPASS);
		renderSystem.QueuePSOLoad(LIGHTPREPASS);
		renderSystem.QueuePSOLoad(FORWARDDRAW);
		renderSystem.QueuePSOLoad(FORWARDDRAWINSTANCED);
		renderSystem.QueuePSOLoad(SHADINGPASS);
		renderSystem.QueuePSOLoad(COMPUTETILEDSHADINGPASS);
		renderSystem.QueuePSOLoad(BILATERALBLURPASSHORIZONTAL);
		renderSystem.QueuePSOLoad(BILATERALBLURPASSVERTICAL);
		renderSystem.QueuePSOLoad(SHADOWMAPPASS);
		renderSystem.QueuePSOLoad(CLEARCOUNTERSPSO);
		renderSystem.QueuePSOLoad(RESOLUTIONMATCHSHADOWMAPS);
		renderSystem.QueuePSOLoad(CLEARSHADOWRESOLUTIONBUFFER);
		renderSystem.QueuePSOLoad(ZPYRAMIDBUILDLEVEL);

		renderSystem.QueuePSOLoad(DEBUG_DrawBVH);

		renderSystem.QueuePSOLoad(CREATELIGHTBVH_PHASE1);
		renderSystem.QueuePSOLoad(CREATELIGHTBVH_PHASE2);

		renderSystem.SetReadBackEvent(
			timingReadBack,
			[&](ReadBackResourceHandle resource)
			{
				auto [buffer, bufferSize] = renderSystem.OpenReadBackBuffer(resource);
				EXITSCOPE(renderSystem.CloseReadBackBuffer(resource));

				if(buffer){
					size_t timePoints[64];
					memcpy(timePoints, (char*)buffer, sizeof(timePoints));

					UINT64 timeStampFreq;
					renderSystem.GraphicsQueue->GetTimestampFrequency(&timeStampFreq);

					float durations[32];

					for (size_t I = 0; I < 4; I++)
						durations[I] = float(timePoints[2 * I + 1] - timePoints[2 * I + 0]) / timeStampFreq * 1000;

					timingValues.gBufferPass		= durations[0];
					timingValues.ClusterCreation	= durations[1];
					timingValues.shadingPass		= durations[2];
					timingValues.BVHConstruction	= durations[3];
				}
			});

		pendingGPUTasks.emplace_back(
			[&](FrameGraph& frameGraph, auto& resources)
			{
				lightingEngine.Init(frameGraph);
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
		if(clusterBuffer != InvalidHandle)
			renderSystem.ReleaseResource(clusterBuffer);
	}


	DrawOutputs WorldRender::DrawScene(UpdateDispatcher& dispatcher, FrameGraph& frameGraph, DrawSceneDescription& drawSceneDesc, WorldRender_Targets targets, iAllocator* persistent, ThreadSafeAllocator& temporary)
	{
		ProfileFunction();

				auto& scene			= drawSceneDesc.scene;
		const	auto  camera		= drawSceneDesc.camera;
				auto& gbuffer		= drawSceneDesc.gbuffer;
		const	auto  t				= drawSceneDesc.t;

		auto&		depthTarget		= targets.DepthTarget;
		auto		renderTarget	= targets.RenderTarget;

		auto& passes = GatherScene(dispatcher, &scene, camera, temporary);

		passes.AddInput(drawSceneDesc.transformDependency);
		passes.AddInput(drawSceneDesc.cameraDependency);

		auto& lightGather		= scene.GetLights(dispatcher, temporary);
		auto& sceneBVH			= scene.UpdateSceneBVH(dispatcher, drawSceneDesc.transformDependency, temporary);
		auto& visableLights		= scene.GetVisableLights(dispatcher, camera, sceneBVH, temporary);
		auto& lightUpdate		= scene.UpdateLights(dispatcher, sceneBVH, visableLights, temporary, persistent);

		LoadLodLevels(dispatcher, passes, drawSceneDesc.camera, renderSystem, *persistent);

		lightGather.AddInput(drawSceneDesc.transformDependency);
		lightGather.AddInput(drawSceneDesc.cameraDependency);


		auto& IKUpdate			= UpdateIKControllers(dispatcher, drawSceneDesc.dt);
		auto& animationUpdate	= UpdateAnimations(dispatcher, drawSceneDesc.dt);
		auto& skinnedObjects	= GatherSkinned(dispatcher, scene, camera, temporary);
		auto& updatedPoses		= UpdatePoses(dispatcher, skinnedObjects);
		auto& loadMorphs		= LoadNeededMorphTargets(dispatcher, passes);

		// [skinned Objects] -> [update Poses]
		IKUpdate.AddInput(drawSceneDesc.transformDependency);
		updatedPoses.AddInput(IKUpdate);
		updatedPoses.AddInput(drawSceneDesc.transformDependency);
		skinnedObjects.AddInput(drawSceneDesc.cameraDependency);
		updatedPoses.AddInput(skinnedObjects);
		updatedPoses.AddInput(animationUpdate);

		// Add Resources
		frameGraph.AddMemoryPool(&UAVPool);
		frameGraph.AddMemoryPool(&RTPool);
		frameGraph.AddMemoryPool(&UAVTexturePool);
		frameGraph.AddTaskDependency(IKUpdate);

		PassData data = {
			.passes		= passes,
		};

		for (auto& task : pendingGPUTasks)
			task(frameGraph, data);

		pendingGPUTasks.clear();

		ClearGBuffer(frameGraph, gbuffer);

		auto& staticConstants =
			BuildBrushConstantsBuffer(
				frameGraph,
				dispatcher,
				passes,
				temporary);

		auto& animationResources =
			AcquirePoseResources(
				frameGraph,
				dispatcher,
				passes,
				UAVPool,
				temporary);

		auto& morphTargets = BuildMorphTargets(
				frameGraph,
				dispatcher,
				passes,
				loadMorphs,
				UAVPool,
				temporary);

		auto& gbufferPass =
			clusteredRender.FillGBuffer1(
				dispatcher,
				frameGraph,
				passes,
				camera,
				gbuffer,
				depthTarget.Get(),
				staticConstants,
				occlusionCulling ? passHistories.GetHistory(renderSystem, drawSceneDesc.camera) : nullptr,
				animationResources,
				temporary);

		if(occlusionCulling)
		{
			auto& occlutionResults =
				clusteredRender.OcclusionCulling(
					dispatcher,
					frameGraph,
					staticConstants,
					passes,
					camera,
					passHistories,
					depthTarget.Get(),
					temporary);

			clusteredRender.FillGBuffer2(
					dispatcher,
					frameGraph,
					passes,
					camera,
					gbufferPass,
					depthTarget.Get(),
					staticConstants,
					*passHistories.GetHistory(renderSystem, drawSceneDesc.camera),
					animationResources,
					temporary);
		}

		ExtraGBufferPassInputs extraGPassInputs{
			.frameGraph		= frameGraph,
			.dispatcher		= dispatcher,
			.passes			= passes,
			.gbuffer		= gbuffer,
			.depthTarget	= depthTarget.Get(),
			.activeCamera	= drawSceneDesc.camera
		};

		for (auto& pass : drawSceneDesc.additionalGbufferPasses)
			pass(extraGPassInputs);

		auto& shadowMapPass =
			shadowMapping.ShadowMapPass(
				frameGraph,
				visableLights,
				lightUpdate,
				drawSceneDesc.cameraDependency,
				passes,
				drawSceneDesc.additionalShadowPasses,
				t,
				RTPool,
				temporary);

		auto& lightPass =
			clusteredRender.UpdateLightBuffers(
				dispatcher,
				frameGraph,
				camera,
				scene,
				visableLights,
				depthTarget.Get(),
				temporary,
				drawSceneDesc.debugDisplay != DebugVisMode::ClusterVIS);

		auto updateVolumes =
			lightingEngine.BuildScene(
				frameGraph,
				scene,
				passes,
				temporary);

		auto& shadingPass =
			clusteredRender.ClusteredShading(
				dispatcher,
				frameGraph,
				shadowMapPass,
				lightGather,
				gbufferPass,
				depthTarget.Get(),
				renderTarget,
				lightPass,
				(float)t,
				temporary);

		lightingEngine.RayTrace(
				dispatcher,
				frameGraph,
				camera,
				passes,
				updateVolumes,
				depthTarget.Get(),
				shadingPass.renderTargetObject,
				gbuffer,
				lightPass,
				temporary);

		auto& OIT_pass =
			transparency.OIT_WB_Pass(
				dispatcher,
				frameGraph,
				passes,
				camera,
				depthTarget.Get(),
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
				(float)drawSceneDesc.dt,
				temporary);
		/*
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
		*/

		ExtraForwardPassInputs extraFPassInputs{
			.frameGraph		= frameGraph,
			.dispatcher		= dispatcher,
			.passes			= passes,
			.renderTarget	= renderTarget,
			.depthTarget	= depthTarget.Get(),
			.activeCamera	= drawSceneDesc.camera
		};

		for (auto& forwardPass : drawSceneDesc.additionalForwardPasses)
			forwardPass(extraFPassInputs);

		passHistories.GetHistory(renderSystem, drawSceneDesc.camera)->EndFrame();

		return DrawOutputs{
					.passes				= passes,
					.entityConstants	= staticConstants,
					.animationResources	= animationResources,
					.pointLights		= visableLights,
					//occlutionConstants.ZPyramid
		};
	}


	/************************************************************************************************/


	BrushConstants& WorldRender::BuildBrushConstantsBuffer(
		FrameGraph&						frameGraph,
		UpdateDispatcher&				dispatcher,
		GatherPassesTask&				passes,
		iAllocator&						allocator)
	{
		return frameGraph.BuildSharedConstants<BrushConstants>(
			[&](FrameGraphNodeBuilder& builder) -> BrushConstants
			{
				builder.AddDataDependency(passes);

				return BrushConstants
					{
						.node				= builder.GetNodeHandle(),
						.constants			= builder.CreateConstantBuffer(),
						.getConstantBuffer	= CreateOnceReserveBuffer2(builder.GetResources(), allocator),
						.passes				= passes,
						.entityTable		= Vector<uint32_t>{ allocator },
					};
			},
			[](BrushConstants& data, FrameResources& resources, iAllocator& localAllocator) // before submission task sections begin
			{
				const auto& materials	= MaterialComponent::GetComponent();
				const auto& brushes		= data.passes.GetData().solid;
				
				data.entityTable.reserve(brushes.size());

				size_t offset = 0;
				for (const auto& brush : brushes)
				{
					data.entityTable.push_back((uint32_t)offset);
					offset += Max(materials[brush.brush->material].subMaterials.size(), 1);
				}

				resources.objects[data.constants].constantBuffer = &data.GetConstantBuffer(offset);
			},
			[](BrushConstants& data, iAllocator& localAllocator) // in parallel with all other tasks
			{
				const	auto& brushes			= data.passes.GetData().solid;
				const	auto& materials			= MaterialComponent::GetComponent();
						auto& constantBuffer	= data.GetConstantBuffer();

				for (auto& brush : brushes)
				{
					const	auto&	mainMaterial	= materials[brush.brush->material];
					const	auto&	subMaterials	= mainMaterial.subMaterials;
							auto	constants		= brush->GetConstants();

					if (subMaterials.size())
					{
						for (const auto& sm : subMaterials)
						{
							const auto& subMaterial = materials[sm];

							constants.MP.textureChannels =
								subMaterial.HasTexture(GetTypeGUID(ALBEDO)) << 0 |
								subMaterial.HasTexture(GetTypeGUID(NORMAL)) << 1 |
								subMaterial.HasTexture(GetTypeGUID(METALLICROUGHNESS)) << 2;

							constants.MP.textureCount = subMaterial.textures.size();

							for (auto&& [idx, texture] : enumerate(subMaterial.textures))
								constants.textureHandles[idx] = uint4{ 256, 256, texture.to_uint() };

							constantBuffer.Push(constants);
						}
					}
					else
					{
						constants.MP.textureChannels =
							mainMaterial.HasTexture(GetTypeGUID(ALBEDO)) << 0 |
							mainMaterial.HasTexture(GetTypeGUID(NORMAL)) << 1 |
							mainMaterial.HasTexture(GetTypeGUID(METALLICROUGHNESS)) << 2;

						constants.MP.textureCount = mainMaterial.textures.size();

						constantBuffer.Push(constants);
					}
				}
			});
	}


	/************************************************************************************************/


	DepthPass& WorldRender::DepthPrePass(
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		const CameraHandle				camera,
		GatherPassesTask&				passes,
		const ResourceHandle			depthBufferTarget,
		iAllocator*						allocator)
	{
		const size_t MaxEntityDrawCount = 1000;

		auto& pass = frameGraph.AddNode<DepthPass>(
			passes.GetData().solid,
			[&, camera](FrameGraphNodeBuilder& builder, DepthPass& data)
			{
				const size_t localBufferSize = Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

				data.entityConstantsBuffer  = builder.GetResources().ReserveCB(sizeof(ForwardDrawConstants) * MaxEntityDrawCount);
				data.passConstantsBuffer    = builder.GetResources().ReserveCB(2048);
				data.depthBufferObject      = builder.DepthTarget(depthBufferTarget);
				data.depthPassTarget        = depthBufferTarget;

				builder.AddDataDependency(passes);
			},
			[=](DepthPass& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				const auto cameraConstants = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };

				DescriptorHeap heap{
					ctx,
					resources.renderSystem().Library.RS6CBVs4SRVs->GetDescHeap(0),
					&allocator };

				heap.NullFill(ctx);

				ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState(resources.GetPipelineState(DEPTHPREPASS, allocator));

				ctx.SetScissorAndViewports({ data.depthPassTarget });
				ctx.SetRenderTargets(
					{},
					true,
					resources.GetResource(data.depthBufferObject));

				ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);
				ctx.SetGraphicsDescriptorTable(0, heap);
				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(3, cameraConstants);
				ctx.NullGraphicsConstantBufferView(6);

				ctx.BeginEvent_DEBUG("Z-PrePass");

				TriMesh* prevMesh = nullptr;
				for (const auto& draw : data.draws)
				{
					auto& meshes = draw->meshes;

					for(size_t I = 0; I < meshes.size(); I++)
					{
						const uint8_t lodIdx	= draw.LODlevel[I];
						auto* const triMesh		= GetMeshResource(meshes[I]);
						const auto& lod			= triMesh->lods[lodIdx];

						if (triMesh != prevMesh)
						{
							prevMesh = triMesh;

							ctx.AddIndexBuffer(triMesh, lodIdx);
							ctx.AddVertexBuffers(triMesh,
								triMesh->GetHighestLoadedLodIdx(),
								{ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION });
						}

						auto constants = ConstantBufferDataSet{ draw->GetConstants(), data.entityConstantsBuffer };
						ctx.SetGraphicsConstantBufferView(2, constants);
						ctx.DrawIndexedInstanced(lod.GetIndexCount());
					}
				}

			ctx.EndEvent_DEBUG();
			});

		return pass;
	}


	/************************************************************************************************/


	BackgroundEnvironmentPass& WorldRender::BackgroundPass(
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		const CameraHandle				camera,
		const ResourceHandle			renderTarget,
		const ResourceHandle			hdrMap,
		iAllocator*						allocator)
	{
		FK_ASSERT(0);

		auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
			BackgroundEnvironmentPass{},
			[&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
			{
				const size_t localBufferSize		= Max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
				auto& renderSystem					= frameGraph.GetRenderSystem();

				data.renderTargetObject				= builder.RenderTarget(renderTarget);
				data.passConstants					= builder.ReserveCB(6 * KILOBYTE);
				data.passVertices					= builder.ReserveVB(sizeof(float4) * 6);
				//data.diffuseMap                     = hdrMap;
			},
			[=](BackgroundEnvironmentPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& tempAllocator)
			{
				DescriptorHeap descHeap;
				descHeap.Init2(ctx, renderSystem.Library.RSDefault->GetDescHeap(0), 20, &tempAllocator);
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
				}passConstants =
					{ float2(
						(float)WH[0],
						(float)WH[1]) };

				ctx.SetRootSignature(frameResources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS, tempAllocator));
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

				data.passConstants				= builder.ReserveCB(6 * KILOBYTE);
				data.passVertices				= builder.ReserveVB(sizeof(float4) * 6);
			},
			[=](BackgroundEnvironmentPass& data, const ResourceHandler& frameResources, Context& ctx, iAllocator& allocator)
			{
				auto& renderSystem			= frameResources.renderSystem();
				const float2 WH				= renderSystem.GetTextureWH(renderTarget);
				const auto cameraConstants	= GetCameraConstants(camera);

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
				descHeap.Init2(ctx, renderSystem.Library.RSDefault->GetDescHeap(0), 20, &allocator);

				descHeap.SetSRV(ctx, 0, frameResources.GetResource(data.AlbedoTargetObject));
				descHeap.SetSRV(ctx, 1, frameResources.GetResource(data.MRIATargetObject));
				descHeap.SetSRV(ctx, 2, frameResources.GetResource(data.NormalTargetObject));
				descHeap.SetSRV(ctx, 4, frameResources.GetResource(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
				descHeap.NullFill(ctx, 20);

				ctx.SetRootSignature(renderSystem.Library.RSDefault);
				ctx.SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS, allocator));
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
		iAllocator*                     tempMemory)
	{
		auto& pass = frameGraph.AddNode<BilateralBlurPass>(
			BilateralBlurPass{},
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
			[=](BilateralBlurPass& data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				auto& renderSystem	= resources.renderSystem();
				const float2 WH		= resources.renderSystem().GetTextureWH(destination);

				auto constantBuffer = resources.ReserveCB(2048);
				auto vertexBuffer   = resources.ReserveVB(2048);

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
				descHeap.Init2(ctx, renderSystem.Library.RSDefault->GetDescHeap(0), 5, &allocator);

				descHeap.SetSRV(ctx, 0, resources.GetResource(data.Source));
				descHeap.SetSRV(ctx, 1, resources.GetResource(data.NormalSource));
				descHeap.SetSRV(ctx, 2, resources.GetResource(data.DepthSource), DeviceFormat::R32_FLOAT);
				descHeap.NullFill(ctx, 3);

				ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(resources.GetPipelineState(BILATERALBLURPASSHORIZONTAL, allocator));
				ctx.SetGraphicsDescriptorTable(5, descHeap);

				ctx.SetScissorAndViewports({ destination });
				ctx.SetRenderTargets({ resources.GetResource(data.TempObject1), resources.GetResource(data.TempObject2) }, false);
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, 6, vertexBuffer } });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, constantBuffer });

				ctx.Draw(6);

				DescriptorHeap descHeap2;
				descHeap2.Init2(ctx, renderSystem.Library.RSDefault->GetDescHeap(0), 5, &allocator);

				descHeap2.SetSRV(ctx, 0, resources.PixelShaderResource(data.TempObject1, ctx));
				descHeap2.SetSRV(ctx, 1, resources.GetResource(data.NormalSource));
				descHeap2.SetSRV(ctx, 2, resources.GetResource(data.DepthSource), DeviceFormat::R32_FLOAT);
				descHeap2.SetSRV(ctx, 3, resources.PixelShaderResource(data.TempObject2, ctx));

				ctx.SetPipelineState(resources.GetPipelineState(BILATERALBLURPASSVERTICAL, allocator));
				ctx.SetGraphicsDescriptorTable(5, descHeap2);
				ctx.SetRenderTargets({ resources.GetResource(data.DestinationObject) }, false);
				ctx.Draw(6);
			});

		return pass;
	}


	/************************************************************************************************/


	LoadPipelineStateRes WorldRender::CreateAverageLumanceLocal(RenderSystem* renderSystem, iAllocator&)
	{
		auto lightPassShader = renderSystem->LoadShader("LuminanceAverage", "cs_6_0", "assets\\shaders\\ToneMapping-CreatePyramid.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
		PSO_desc.CS             = lightPassShader;
		PSO_desc.pRootSignature = *rootSignatureToneMapping;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return { PSO, rootSignatureToneMapping };
	}

	LoadPipelineStateRes WorldRender::CreateAverageLumanceGlobal(RenderSystem* renderSystem, iAllocator&)
	{
		auto lightPassShader = renderSystem->LoadShader("AverageLuminance", "cs_6_0", "assets\\shaders\\ToneMapping-CreatePyramid.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
		PSO_desc.CS             = lightPassShader;
		PSO_desc.pRootSignature = *rootSignatureToneMapping;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "CalculateLuminance");

		return { PSO, rootSignatureToneMapping };
	}

	LoadPipelineStateRes WorldRender::CreateToneMapping(RenderSystem* renderSystem, iAllocator&)
	{
		auto VShader = renderSystem->LoadShader("FullScreen", "vs_6_0", "assets\\shaders\\ToneMapping-CreatePyramid.hlsl");
		auto PShader = renderSystem->LoadShader("ToneMap", "ps_6_0", "assets\\shaders\\ToneMapping-CreatePyramid.hlsl");


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = *rootSignatureToneMapping;
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
			PSO_Desc.InputLayout           = { nullptr, 0 };
			PSO_Desc.DepthStencilState     = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "ToneMapping");

		return { PSO, rootSignatureToneMapping };
	}


	/************************************************************************************************/

	// TODO(@RobertMay): Do actual tone mapping
	ToneMap& WorldRender::RenderPBR_ToneMapping(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				FrameResourceHandle             source,
				ResourceHandle                  target,
				float                           t,
				iAllocator*                     allocator)
	{
		return frameGraph.AddNode<ToneMap>(
			ToneMap{},
			[&](FrameGraphNodeBuilder& builder, ToneMap& data)
			{
				const auto WH = frameGraph.GetRenderSystem().GetTextureWH(target) / 2;

				data.outputTarget   = builder.RenderTarget(target);
				data.sourceTarget   = builder.NonPixelShaderResource(source);

				//data.sourceTarget   = builder.ReadTransition(source, DASNonPixelShaderResource);
				data.temp1Buffer    = builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(2048, DeviceFormat::R32_FLOAT), DASUAV);
				data.temp2Buffer    = builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture(WH, DeviceFormat::R32_FLOAT), DASUAV);
			},
			[&]
			(ToneMap& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

				ctx.BeginEvent_DEBUG("Tone Mapping");

				const uint2 WH = resources.GetTextureWH(data.sourceTarget);
				const uint2 XY = (float2{ (float)WH[0], (float)WH[1] } / 512.0f).ceil();

				DescriptorHeap heap1{};
				heap1.Init(ctx, rootSignatureToneMapping->GetDescHeap(0), &allocator);
				heap1.SetSRV(ctx, 0, resources.NonPixelShaderResource(data.sourceTarget, ctx));

#if 0
				ID3D12PipelineState* createInitialLevel = resources.GetPipelineState(AVERAGELUMINANCE_BLOCK);
				ID3D12PipelineState* averageLuminance = resources.GetPipelineState(AVERAGELUMANANCE_GLOBAL);

				ctx.SetComputeRootSignature(rootSignatureToneMapping);
				ctx.SetComputeDescriptorTable(0, heap1);
				ctx.SetComputeUnorderedAccessView(1, resources.UAV(data.temp1Buffer, ctx));
				ctx.SetComputeConstantValue(2, 2, &XY, 0);
				ctx.Dispatch(createInitialLevel, { XY, 1 });

				ctx.AddUAVBarrier(resources.GetResource(data.temp1Buffer));
				ctx.Dispatch(averageLuminance, { 1, 1, 1 });
				ctx.AddUAVBarrier(resources.GetResource(data.temp1Buffer));
#endif

				ID3D12PipelineState* toneMap = resources.GetPipelineState(TONEMAP, allocator);

				ctx.SetRootSignature(rootSignatureToneMapping);
				ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);
				ctx.SetPipelineState(toneMap);
				ctx.SetGraphicsDescriptorTable(0, heap1);
				//ctx.SetGraphicsUnorderedAccessView(1, resources.UAV(data.temp1Buffer, ctx));
				ctx.SetGraphicsConstantValue(2, 2, &XY, 0);
				ctx.SetScissorAndViewports({  resources.GetResource(data.outputTarget) });
				ctx.SetRenderTargets({ resources.RenderTarget(data.outputTarget, ctx) }, false);
				ctx.Draw(3, 0);

				ctx.EndEvent_DEBUG();
			});
	}


}	/************************************************************************************************/

/**********************************************************************

Copyright (c) 2019-2022 Robert May

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
