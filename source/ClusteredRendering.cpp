#pragma once

#include "AnimationComponents.h"
#include "AnimationRendering.h"
#include "ClusteredRendering.h"
#include "WorldRender.h"
#include <numeric>
#include <ranges>
#include <bit>

namespace FlexKit
{   /************************************************************************************************/
	using namespace std::views;


	ID3D12PipelineState* ClusteredRender::CreateGBufferPassPSO(RenderSystem* RS)
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
			PSO_Desc.pRootSignature			= RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS						= DrawRectVShader;
			PSO_Desc.PS						= DrawRectPShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 3;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
			PSO_Desc.RTVFormats[1]			= DXGI_FORMAT_R8G8B8A8_UNORM; // Specular
			PSO_Desc.RTVFormats[2]			= DXGI_FORMAT_R16G16_FLOAT; // Normal
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "GBufferPassPSO");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateGBufferSkinnedPassPSO(RenderSystem* RS)
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
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "BLENDWEIGHT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	4, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES",	0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT,	5, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "BLENDPOS",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	6, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDNORM",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	7, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BLENDTAN",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,	8, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS						= DrawRectVShader;
			PSO_Desc.PS						= DrawRectPShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 3;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
			PSO_Desc.RTVFormats[1]			= DXGI_FORMAT_R8G8B8A8_UNORM; // Specular
			PSO_Desc.RTVFormats[2]			= DXGI_FORMAT_R16G16_FLOAT; // Normal
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;
			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "GBufferSkinnedPassPSO");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateDeferredShadingPassPSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("ShadingPass_VS",     "vs_6_0",	"assets\\shaders\\ClusteredShading\\deferredRender.hlsl");
		auto PShader = RS->LoadShader("DeferredShade_PS",   "ps_6_0",	"assets\\shaders\\ClusteredShading\\deferredRender.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc					= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable					= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= rootSignature;
			PSO_Desc.VS						= VShader;
			PSO_Desc.PS						= PShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
			PSO_Desc.DepthStencilState		= Depth_Desc;

			PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
			PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

			PSO_Desc.BlendState.RenderTarget[0].DestBlend	= D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].SrcBlend	= D3D12_BLEND::D3D12_BLEND_ONE;

			PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha	= D3D12_BLEND::D3D12_BLEND_ONE;
			PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha	= D3D12_BLEND::D3D12_BLEND_ONE;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "DeferredShadingPSO");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateDeferredShadingPassComputePSO(RenderSystem* RS)
	{
		auto CShader = RS->LoadShader("ClusteredShading", "cs_6_6", "assets\\shaders\\ClusteredShading\\ClusteredShading.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_Desc = {};
		PSO_Desc.pRootSignature	= RS->Library.RSDefault;
		PSO_Desc.CS				= CShader;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "DeferredShadingComputePSO");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateClearClusterCountersPSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("ClearCounters", "cs_6_0", R"(assets\shaders\ClusteredShading\ClusteredRendering.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "ClearClusteredCounters");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateLight_DEBUGARGSVIS_PSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");
		auto PShader = RS->LoadShader("PMain", "ps_6_1", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");
		auto GShader = RS->LoadShader("GMain3", "gs_6_0", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS->Library.RSDefault;
			PSO_Desc.VS						= VShader;
			PSO_Desc.PS						= PShader;
			PSO_Desc.GS						= GShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { nullptr, 0 };
			PSO_Desc.DepthStencilState		= Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "CreateLightBuffers_DEBUGVIS");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateLightBVH_PHASE1_PSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateLightBVH_PHASE1", "cs_6_0", R"(assets\shaders\ClusteredShading\LightBVH.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateLightBVH_PHASE1");

		return PSO;
	}

	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateLightBVH_PHASE2_PSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateLightBVH_PHASE2", "cs_6_0", R"(assets\shaders\ClusteredShading\LightBVH.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateLightBVH_PHASE2");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateLightBVH_DEBUGVIS_PSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");
		auto PShader = RS->LoadShader("PMain", "ps_6_1", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");
		auto GShader = RS->LoadShader("GMain", "gs_6_0", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS->Library.RSDefault;
			PSO_Desc.VS						= VShader;
			PSO_Desc.PS						= PShader;
			PSO_Desc.GS						= GShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { nullptr, 0 };
			PSO_Desc.DepthStencilState		= Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "CreateLightBVH_DEBUGVIS");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateCluster_DEBUGARGSVIS_PSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateArguments", "cs_6_0", R"(assets\shaders\ClusteredShading\ClusterArgsDebugVis.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateClusterArguments");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateLightListArgs_PSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateLightListArguents", "cs_6_0", R"(assets\shaders\ClusteredShading\LightListArguementIndirect.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateLightListArgs");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateClusterLightListsPSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateClustersLightLists", "cs_6_0", R"(assets\shaders\ClusteredShading\lightListConstruction.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateLightLists");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateResolutionMatch_PSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("ResolutionMatch", "cs_6_0", R"(assets\shaders\ResolutionMatch.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "ResolutionMatch_UNUSED");

		return PSO;
	}

	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateClearResolutionMatch_PSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("Clear", "cs_6_0", R"(assets\shaders\ResolutionMatch.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "ResolutionMatchClear_UNUSED");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateClustersPSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateClusters", "cs_6_0", R"(assets\shaders\ClusteredShading\ClusteredRendering.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateClusters");

		return PSO;
	}

	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateCluster_DEBUGVIS_PSO(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");
		auto PShader = RS->LoadShader("PMain", "ps_6_1", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");
		auto GShader = RS->LoadShader("GMain2", "gs_6_0", "assets\\shaders\\ClusteredShading\\lightBVH_DEBUGVIS.hlsl");

		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature			= RS->Library.RSDefault;
			PSO_Desc.VS						= VShader;
			PSO_Desc.PS						= PShader;
			PSO_Desc.GS						= GShader;
			PSO_Desc.RasterizerState		= Rast_Desc;
			PSO_Desc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask				= UINT_MAX;
			PSO_Desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets		= 1;
			PSO_Desc.RTVFormats[0]			= DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
			PSO_Desc.SampleDesc.Count		= 1;
			PSO_Desc.SampleDesc.Quality		= 0;
			PSO_Desc.DSVFormat				= DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout			= { nullptr, 0 };
			PSO_Desc.DepthStencilState		= Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		SETDEBUGNAME(PSO, "CreateClusters");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateDEBUGBVHVIS(RenderSystem* RS)
	{
		auto VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\ClusteredShading\\DebugVISBVH.hlsl");
		auto GShader = RS->LoadShader("GMain", "gs_6_0", "assets\\shaders\\ClusteredShading\\DebugVISBVH.hlsl");
		auto PShader = RS->LoadShader("PMain", "ps_6_0", "assets\\shaders\\ClusteredShading\\DebugVISBVH.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
			{ "MIN", 0,     DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	 D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "MAX", 0,     DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0,   DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

		SETDEBUGNAME(PSO, "CreateBVH_DEBUGVIS");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateClusterBufferPSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("CreateClusterBuffer", "cs_6_0", R"(assets\shaders\ClusteredShading\ClusterBuffer.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.ComputeSignature,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateClusterBuffer");

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* ClusteredRender::CreateComputeTiledDeferredPSO(RenderSystem* RS)
	{
		Shader computeShader = RS->LoadShader("csmain", "cs_6_0", R"(assets\shaders\ClusteredShading\computedeferredtiledshading.hlsl)");

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			RS->Library.RSDefault,
			computeShader
		};

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PSO));

		FK_ASSERT(SUCCEEDED(HR), "Failed to create PSO");

		SETDEBUGNAME(PSO, "CreateComputeTiledDeferred");

		return PSO;
	}


	/************************************************************************************************/


	GBuffer::GBuffer(const uint2 WH, RenderSystem& RS_IN) :
		RS				{ RS_IN },
		albedo			{ RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM)) },
		MRIA			{ RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM)) },
		normal			{ RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16_FLOAT)) }
	{

		/*
		auto albedoDesc = GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM);
		auto mriaDesc = GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM);
		auto normalDesc = GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16_FLOAT);

		albedoDesc.bufferCount = 1;
		mriaDesc.bufferCount = 1;
		normalDesc.bufferCount = 1;

		albedo = RS.CreateGPUResource(albedoDesc);
		MRIA = RS.CreateGPUResource(mriaDesc);
		normal = RS.CreateGPUResource(normalDesc);
		*/
		RS.SetDebugName(albedo,		"Albedo");
		RS.SetDebugName(MRIA,		"MRIA");
		RS.SetDebugName(normal,		"Normal");
	}


	/************************************************************************************************/


	GBuffer::~GBuffer()
	{
		RS.ReleaseResource(albedo);
		RS.ReleaseResource(MRIA);
		RS.ReleaseResource(normal);
	}


	/************************************************************************************************/


	void GBuffer::Resize(const uint2 WH)
	{
		auto previousWH = RS.GetTextureWH(albedo);
		if (WH == previousWH)
			return;

		RS.ReleaseResource(albedo);
		RS.ReleaseResource(MRIA);
		RS.ReleaseResource(normal);

		albedo  = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM));
		MRIA    = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM));
		normal  = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16_FLOAT));

		RS.SetDebugName(albedo,	"Albedo");
		RS.SetDebugName(MRIA,	"MRIA");
		RS.SetDebugName(normal,	"Normal");
	}


	/************************************************************************************************/


	void ClearGBuffer(FrameGraph& frameGraph, GBuffer& gbuffer)
	{
		struct GBufferClear
		{
			FrameResourceHandle albedo;
			FrameResourceHandle MRIA;
			FrameResourceHandle normal;
		};

		auto& clear = frameGraph.AddNode<GBufferClear>(
			GBufferClear{},
			[&](FrameGraphNodeBuilder& builder, GBufferClear& data)
			{
				auto temp = builder.GetRenderSystem().GetDeviceResource(gbuffer.albedo);

				data.albedo  = builder.RenderTarget(gbuffer.albedo);
				data.MRIA    = builder.RenderTarget(gbuffer.MRIA);
				data.normal  = builder.RenderTarget(gbuffer.normal);
			},
			[&gbuffer](GBufferClear& data, ResourceHandler& resources, Context& ctx, iAllocator&)
			{
				auto temp = resources.renderSystem().GetDeviceResource(gbuffer.albedo);

				ctx.BeginEvent_DEBUG("Clear GBuffer");

				ctx.ClearRenderTarget(gbuffer.albedo);
				ctx.ClearRenderTarget(gbuffer.MRIA);
				ctx.ClearRenderTarget(gbuffer.normal);

				ctx.EndEvent_DEBUG();
			});
	}


	/************************************************************************************************/


	ClusteredRender::ClusteredRender(RenderSystem& renderSystem, iAllocator& persistent) :
		dispatch		{ renderSystem.CreateIndirectLayout({ { ILE_DispatchCall } },	&persistent) },
		draw			{ renderSystem.CreateIndirectLayout({ { ILE_DrawCall } },		&persistent) },
		rootSignature	{ persistent }
	{
		renderSystem.RegisterPSOLoader(COMPUTETILEDSHADINGPASS,		{ &renderSystem.Library.RSDefault, CreateComputeTiledDeferredPSO });

		renderSystem.RegisterPSOLoader(CREATECLUSTERS,				{ &renderSystem.Library.ComputeSignature, CreateClustersPSO				});
		renderSystem.RegisterPSOLoader(CREATECLUSTERBUFFER,			{ &renderSystem.Library.ComputeSignature, CreateClusterBufferPSO		});
		renderSystem.RegisterPSOLoader(CREATECLUSTERLIGHTLISTS,		{ &renderSystem.Library.ComputeSignature, CreateClusterLightListsPSO	});
		renderSystem.RegisterPSOLoader(CREATELIGHTBVH_PHASE1,		{ &renderSystem.Library.ComputeSignature, CreateLightBVH_PHASE1_PSO		});
		renderSystem.RegisterPSOLoader(CREATELIGHTBVH_PHASE2,		{ &renderSystem.Library.ComputeSignature, CreateLightBVH_PHASE2_PSO		});
		renderSystem.RegisterPSOLoader(CREATELIGHTLISTARGS_PSO,		{ &renderSystem.Library.ComputeSignature, CreateLightListArgs_PSO		});
		renderSystem.RegisterPSOLoader(CLEARCOUNTERSPSO,			{ &renderSystem.Library.ComputeSignature, CreateClearClusterCountersPSO	});

		renderSystem.RegisterPSOLoader(LIGHTBVH_DEBUGVIS_PSO,		{ &renderSystem.Library.RSDefault, CreateLightBVH_DEBUGVIS_PSO });
		renderSystem.RegisterPSOLoader(CLUSTER_DEBUGVIS_PSO,		{ &renderSystem.Library.RSDefault, CreateCluster_DEBUGVIS_PSO });
		renderSystem.RegisterPSOLoader(CLUSTER_DEBUGARGSVIS_PSO,	{ &renderSystem.Library.RSDefault, CreateCluster_DEBUGARGSVIS_PSO });
		renderSystem.RegisterPSOLoader(CREATELIGHTDEBUGVIS_PSO,		{ &renderSystem.Library.RSDefault, CreateLight_DEBUGARGSVIS_PSO });
		renderSystem.RegisterPSOLoader(RESOLUTIONMATCHSHADOWMAPS,	{ &renderSystem.Library.RSDefault, CreateResolutionMatch_PSO });
		renderSystem.RegisterPSOLoader(CLEARSHADOWRESOLUTIONBUFFER,	{ &renderSystem.Library.RSDefault, CreateClearResolutionMatch_PSO});

		renderSystem.RegisterPSOLoader(GBUFFERPASS,				{ &renderSystem.Library.RS6CBVs4SRVs, CreateGBufferPassPSO			});
		renderSystem.RegisterPSOLoader(GBUFFERPASS_SKINNED,		{ &renderSystem.Library.RS6CBVs4SRVs, CreateGBufferSkinnedPassPSO	});
		renderSystem.RegisterPSOLoader(SHADINGPASS,				{ &renderSystem.Library.RS6CBVs4SRVs, [this](auto rs){ return CreateDeferredShadingPassPSO(rs);	} });
		renderSystem.RegisterPSOLoader(SHADINGPASSCOMPUTE,		{ &renderSystem.Library.RS6CBVs4SRVs, CreateDeferredShadingPassComputePSO });

		renderSystem.RegisterPSOLoader(DEBUG_DrawBVH, { &renderSystem.Library.RSDefault, CreateDEBUGBVHVIS });


		rootSignature.AllowIA = true;

		DesciptorHeapLayout<16> DescriptorHeapSRV1;
		DescriptorHeapSRV1.SetParameterAsSRV(0, 0, -1, 0);
		DesciptorHeapLayout<16> DescriptorHeapSRV2;
		DescriptorHeapSRV2.SetParameterAsSRV(0, 0, -1, 1);

		rootSignature.SetParameterAsCBV(0, 0, 0, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
		rootSignature.SetParameterAsCBV(1, 1, 0, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
		rootSignature.SetParameterAsCBV(2, 2, 0, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
		rootSignature.SetParameterAsCBV(3, 3, 0, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
		rootSignature.SetParameterAsDescriptorTable(4, DescriptorHeapSRV1, -1, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
		rootSignature.SetParameterAsDescriptorTable(5, DescriptorHeapSRV2, -1, PIPELINE_DESTINATION::PIPELINE_DEST_PS);
		rootSignature.Build(renderSystem, persistent);

		SETDEBUGNAME(rootSignature.Get_ptr(), "Shading");
	}



	
	/************************************************************************************************/


	FlexKit::TypeErasedCallable<void (FrameGraph&), 64> ClusteredRender::CreateClusterBuffer(RenderSystem& renderSystem, uint2 WH, CameraHandle camera, MemoryPoolAllocator& UAVPool, ReserveConstantBufferFunction& reserveCB)
	{
		return
			[&, camera = camera, WH = WH](FrameGraph& frameGraph)
			{
				if (clusterBuffer != InvalidHandle)
					UAVPool.Release(clusterBuffer, false, false);

				struct GPUCluster { float4 min, max; };

				const uint2 XY		{ (Vect2{ WH } / 32.0f).Ceil() };
				const uint2 dim		{ (Vect2{ XY } / 32.0f).Ceil() };
				const uint bufferSize	= XY.Product() * sizeof(GPUCluster) * 24;

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
						ProfileFunction();

						ctx.BeginEvent_DEBUG("Create Cluster");

						auto  cameraValues    = GetCameraConstants(camera);

						struct
						{
							float4x4	iproj;
							float4x4	pad;
							uint2		WH;
							uint2		Dim;
							uint		rowPitch;
							uint		slicePitch;
						} constantValues = {
								Inverse(cameraValues.Proj),
								float4x4{},
								WH,
								XY,
								XY[0],
								XY.Product(),
						};

						auto constantBuffer		= data.reserveCB(AlignedSize(sizeof(cameraValues)) + AlignedSize(sizeof(constantValues)));
						auto passConstants		= FlexKit::ConstantBufferDataSet{ constantValues, constantBuffer };
						auto cameraConstants	= FlexKit::ConstantBufferDataSet{ cameraValues, constantBuffer };

						const auto PSO		= resources.GetPipelineState(CREATECLUSTERBUFFER);
						const auto& rootSig	= resources.renderSystem().Library.ComputeSignature;

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
			};
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

	constexpr size_t sizeofBVH_Node		= sizeof(BVH_Node);
	constexpr size_t BVH_ELEMENT_COUNT	= 32;

	LightBufferUpdate& ClusteredRender::UpdateLightBuffers(
		UpdateDispatcher&					dispatcher,
		FrameGraph&							frameGraph,
		const CameraHandle					camera,
		const Scene&						scene,
		const GatherVisibleLightsTask&		visibleLightsTask,
		ResourceHandle						depthBuffer,
		ReserveConstantBufferFunction		reserveCB,
		iAllocator*							tempMemory,
		bool								releaseTemporaries)
	{
		PassDrivenResourceAllocation allocationDesc
		{
			.getPass				= [&]() -> std::span<const LightHandle>
			{
				return visibleLightsTask.GetData().lights;
			},
			.initializeResources	= [](std::span<const LightHandle> visibleLights, std::span<const FrameResourceHandle> resourceHandles, auto& transferCtx, iAllocator& allocator)
			{
				Vector<float4x4> shadowMaps{ allocator };
				shadowMaps.resize(visibleLights.size());

				auto& lights = LightComponent::GetComponent();

				for (auto [idx, lightHandle] : zip(iota(0), visibleLights))
				{
					auto& light = lights[lightHandle];
					switch (light.type)
					{
					case LightType::Direction:
					case LightType::SpotLight:
					case LightType::SpotLightBasicShadows:
					{
						const float3		position	= GetPositionW(light.node);
						const Quaternion	Q			= GetOrientation(light.node);

						const auto [PV, view] = CalculateSpotLightMatrices(position, Q, light.R, light.outerAngle);

						shadowMaps[idx] = PV;
					}	break;
					case LightType::PointLight:
						break;
					}
				}

				transferCtx.CreateResource(resourceHandles[0], shadowMaps.ByteSize(),	shadowMaps.data());
			},

			.layout		= DeviceLayout::DeviceLayout_Common,
			.access		= DeviceAccessState::DASPixelShaderResource,
			.max		= 1,
			.pool		= nullptr,
			.dependency = visibleLightsTask
		};

		const ResourceAllocation& allocation = frameGraph.AllocateResourceSet(allocationDesc);

		auto WH = frameGraph.GetRenderSystem().GetTextureWH(depthBuffer);
		auto& lightBufferData = frameGraph.AddNode<LightBufferUpdate>(
			LightBufferUpdate{
					.visableLights	= visibleLightsTask.GetData().lights,
					.shadowMatrices	= allocation.handles[0],
					.camera			= camera,
					.reserveCB		= reserveCB,
					.indirectLayout	= dispatch
			},
			[&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
			{
				auto& renderSystem			= frameGraph.GetRenderSystem();

				// Output Objects
				data.clusterBufferObject	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1 * MEGABYTE), DASUAV);
				data.lightLists				= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(2 * MEGABYTE), DASUAV, VirtualResourceScope::Frame);
				data.indexBufferObject		= builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture({ WH }, DeviceFormat::R32_UINT), DASUAV, VirtualResourceScope::Frame);
				data.lightListBuffer		= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1024 * KILOBYTE), DASUAV, VirtualResourceScope::Frame);
				data.lightBufferObject		= builder.AcquireVirtualResource(GPUResourceDesc::StructuredResource(512 * KILOBYTE), DASNonPixelShaderResource, VirtualResourceScope::Frame);

				// Temporaries
				data.lightBVH				= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE),	DASUAV);
				data.lightLookupObject		= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512 * KILOBYTE),	DASUAV);
				data.lightCounterObject		= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512),				DASUAV);
				data.lightResolutionObject	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(64 * KILOBYTE),	DASUAV);
				data.counterObject			= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DASUAV);
				data.argumentBufferObject	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DASUAV);

				data.depthBufferObject		= builder.DepthTarget(depthBuffer);
				data.camera					= camera;

				/*
				if (readBackBuffers.size())
					data.readBackHandle = readBackBuffers.pop_front();
				else
					data.readBackHandle = InvalidHandle;
				*/

				builder.SetDebugName(data.clusterBufferObject,		"ClusterBufferObject");
				builder.SetDebugName(data.lightLists,				"lightLists");
				builder.SetDebugName(data.indexBufferObject,		"indexBufferObject");
				builder.SetDebugName(data.lightListBuffer,			"lightListBuffer");
				builder.SetDebugName(data.lightBufferObject,		"lightBufferObject");
				builder.SetDebugName(data.lightBVH,					"lightBVH");
				builder.SetDebugName(data.lightLookupObject,		"lightLookupObject");
				builder.SetDebugName(data.lightCounterObject,		"lightCounterObject");
				builder.SetDebugName(data.lightResolutionObject,	"lightResolutionObject");
				builder.SetDebugName(data.counterObject,			"counterObject");
				builder.SetDebugName(data.argumentBufferObject,		"argumentBufferObject");

				builder.AddNodeDependency(allocation.node);
				//builder.AddDataDependency(sceneDescription.lights);
				//builder.AddDataDependency(sceneDescription.cameras);
			},
			[this, XY = WH / 32](LightBufferUpdate& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

				ctx.BeginEvent_DEBUG("Update Light Buffers");

				auto		CreateClusters				= resources.GetPipelineState(CREATECLUSTERS);
				auto		ClearCounters				= resources.GetPipelineState(CLEARCOUNTERSPSO);
				auto		CreateBVH_Phase1			= resources.GetPipelineState(CREATELIGHTBVH_PHASE1);
				auto		CreateBVH_Phase2			= resources.GetPipelineState(CREATELIGHTBVH_PHASE2);
				auto		CreateLightLists			= resources.GetPipelineState(CREATECLUSTERLIGHTLISTS);
				auto		CreateLightListArguments	= resources.GetPipelineState(CREATELIGHTLISTARGS_PSO);
				auto		resolutionMatchShadowsMaps	= resources.GetPipelineState(RESOLUTIONMATCHSHADOWMAPS);
				auto		clearShadowMapBuffer		= resources.GetPipelineState(CLEARSHADOWRESOLUTIONBUFFER);

				const auto  cameraConstants		= GetCameraConstants(data.camera);
				const auto  lightCount			= data.visableLights.size();

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

				LightComponent& pointLights = LightComponent::GetComponent();

				const uint32_t nodeReservation = uint32_t(ceilf(std::logf(float(lightCount)) / std::logf(BVH_ELEMENT_COUNT)));

				CBPushBuffer    constantBuffer = data.reserveCB(
					AlignedSize( sizeof(FlexKit::GPULight) * data.visableLights.size() ) +
					AlignedSize<ConstantsLayout>() * (1 + nodeReservation) +
					AlignedSize<Camera::ConstantBuffer>());

				auto constantBuffer2ReserveSize = AlignedSize<ConstantsLayout>() * (1 + nodeReservation);
				CBPushBuffer    constantBuffer2 = data.reserveCB(constantBuffer2ReserveSize);

				const ConstantBufferDataSet constants				{ constantsValues, constantBuffer };
				const ConstantBufferDataSet cameraConstantsBuffer	{ cameraConstants, constantBuffer };

				Vector<FlexKit::GPULight> lightValues{ &allocator };

				for (const auto lightHandle : data.visableLights)
				{
					const Light& light				= pointLights[lightHandle];
					const float3 WS_position		= GetPositionW(light.node);
					const Quaternion WS_orientation = GetOrientation(light.node);

					lightValues.emplace_back(
						GPULight{
							.KI					= { light.K, light.I },
							.PositionR			= { WS_position, light.R },
							.directionSpread	= { WS_orientation * float3{ 0, 0, 1 }, light.outerAngle },
							.typeExtra			= { light.type, light.GetExtra() }
						});
				}

				const uint32_t uploadSize = (uint32_t)lightValues.size() * sizeof(GPULight);
				const ConstantBufferDataSet pointLights_GPU{ (char*)lightValues.data(), uploadSize, constantBuffer };

				resources.UAV(data.counterObject, ctx);
				resources.UAV(data.indexBufferObject, ctx);

				ctx.ClearUAVTextureUint(resources.GetResource(data.indexBufferObject), uint4{(uint)-1, (uint)-1, (uint)-1, (uint)-1 });

				ctx.CopyBufferRegion(
					resources.CopyDest(data.lightBufferObject, ctx),
					resources.GetDeviceResource(pointLights_GPU.Handle()),
					uploadSize,
					0,
					pointLights_GPU.Offset());

				ctx.SetComputeRootSignature(resources.renderSystem().Library.ComputeSignature);

				DescriptorHeap clearHeap;
				clearHeap.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
				clearHeap.SetUAVStructured(ctx, 2, resources.GetResource(data.counterObject), sizeof(UINT), 0);
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
				clusterCreationResources.SetSRV(ctx, 4, resources.NonPixelShaderResource(data.depthBufferObject, ctx), DeviceFormat::R32_FLOAT);

				// CBV's start at 8
				clusterCreationResources.SetCBV(ctx, 8, cameraConstantsBuffer);
				clusterCreationResources.SetCBV(ctx, 9, constants);
				clusterCreationResources.NullFill(ctx);

				//ctx.TimeStamp(timeStats, 2);

				const auto threadsWH = uint2{ (Vect2{ resources.GetTextureWH(data.depthBufferObject) } / 32.0f).Ceil() };
				ctx.SetComputeDescriptorTable(0, clusterCreationResources);
				ctx.Dispatch(CreateClusters, uint3{ threadsWH, 1 });
				ctx.AddUAVBarrier(resources[data.lightBVH]);

				//ctx.TimeStamp(timeStats, 3);

				// Create light BVH
				// TODO: allow for more than 1024 point lights
				//          move sorting of lights to a separate pass
				//          then pull sorted light list into the two pass BVH construct
				DescriptorHeap BVH_Phase1_Resources;
				BVH_Phase1_Resources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
				BVH_Phase1_Resources.SetUAVStructured		(ctx, 0, resources.UAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);
				BVH_Phase1_Resources.SetUAVStructured		(ctx, 1, resources.UAV(data.lightLookupObject, ctx), sizeof(uint), 0);
				BVH_Phase1_Resources.SetStructuredResource	(ctx, 4, resources.NonPixelShaderResource(data.lightBufferObject, ctx), sizeof(GPULight));
				BVH_Phase1_Resources.SetCBV(ctx, 8, constants);

				ctx.BeginEvent_DEBUG("Build BVH");

				//ctx.TimeStamp(timeStats, 6);
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
						BVH_Phase2_Resources.SetUAVStructured(ctx, 0,		resources.UAV(data.lightBVH, ctx), sizeof(BVH_Node), 0);
						BVH_Phase2_Resources.SetStructuredResource(ctx, 4,	resources.NonPixelShaderResource(data.lightBufferObject, ctx), sizeof(GPULight));
						BVH_Phase2_Resources.SetCBV(ctx, 8, constantSet);

						ctx.BeginEvent_DEBUG("BVH Phase2");

						ctx.SetComputeDescriptorTable(0, BVH_Phase2_Resources);
						ctx.Dispatch(CreateBVH_Phase2, { 1, 1, 1 });

						ctx.AddUAVBarrier();

						ctx.EndEvent_DEBUG();
					};

				uint32_t offset				= 0;
				uint32_t nodeCount			= uint32_t(std::ceilf(float(lightCount) / BVH_ELEMENT_COUNT));
				const uint32_t passCount	= uint32_t(std::floor(std::log(lightCount) / std::log(BVH_ELEMENT_COUNT)));

				for (uint32_t I = 0; I < passCount; I++)
				{
					Phase2_Pass(offset, nodeCount);
					offset		+= nodeCount;
					nodeCount	= uint32_t(std::ceilf(float(nodeCount) / BVH_ELEMENT_COUNT));
				}

				ctx.EndEvent_DEBUG();

				DescriptorHeap createArgumentResources;
				createArgumentResources.Init(ctx, resources.renderSystem().Library.ComputeSignature.GetDescHeap(0), &allocator);
				createArgumentResources.SetUAVStructured(ctx, 0, resources.UAV(data.argumentBufferObject, ctx), sizeof(uint4), 0);
				createArgumentResources.SetUAVStructured(ctx, 1, resources.UAV(data.lightCounterObject, ctx), sizeof(uint32_t), 0);
				createArgumentResources.SetStructuredResource(ctx, 4, resources.PixelShaderResource(data.counterObject, ctx), sizeof(uint32_t));
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
				createLightList_ShaderResources.SetStructuredResource(ctx, 0, resources.NonPixelShaderResource(data.lightBVH,				ctx), sizeofBVH_Node);
				createLightList_ShaderResources.SetStructuredResource(ctx, 1, resources.NonPixelShaderResource(data.lightLookupObject,		ctx), sizeof(uint32_t));
				createLightList_ShaderResources.SetStructuredResource(ctx, 2, resources.NonPixelShaderResource(data.lightBufferObject,		ctx), sizeof(GPULight));
				createLightList_ShaderResources.SetStructuredResource(ctx, 3, resources.NonPixelShaderResource(data.clusterBufferObject,	ctx), sizeof(GPUCluster), 0);

				DescriptorHeap createLightList_UAVResources;
				createLightList_UAVResources.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 3, &allocator);
				createLightList_UAVResources.SetUAVStructured(ctx, 0, resources.NonPixelShaderResource(data.lightLists,				ctx), sizeof(uint2), 0);
				createLightList_UAVResources.SetUAVStructured(ctx, 1, resources.NonPixelShaderResource(data.lightListBuffer,		ctx), sizeof(uint32_t), 0);
				createLightList_UAVResources.SetUAVStructured(ctx, 2, resources.NonPixelShaderResource(data.lightCounterObject,		ctx), sizeof(uint32_t), 0);

				ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(CreateLightLists);

				ctx.SetComputeConstantBufferView(0, lightListConstantSet);
				ctx.SetComputeConstantBufferView(1, resources.NonPixelShaderResource(data.counterObject, ctx), 0, 256);

				ctx.SetComputeDescriptorTable(4, createLightList_ShaderResources);
				ctx.SetComputeDescriptorTable(5, createLightList_UAVResources);

				ctx.ExecuteIndirect(resources.IndirectArgs(data.argumentBufferObject, ctx), data.indirectLayout);

				//ctx.TimeStamp(timeStats, 7);

				ctx.EndEvent_DEBUG();

				if(data.readBackHandle != InvalidHandle && false)
				{
					/*
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

					resolutionMatchShadowMaps.SetUAVStructured(ctx, 0,		resources.UAV(data.lightResolutionObject,	ctx), 4, 0);
					resolutionMatchShadowMaps.SetStructuredResource(ctx, 4,	resources.NonPixelShaderResource(data.clusterBufferObject,	ctx), sizeof(GPUCluster), 0);
					resolutionMatchShadowMaps.SetStructuredResource(ctx, 5,	resources.NonPixelShaderResource(data.lightListBuffer,		ctx), sizeof(uint32_t), 0);
					resolutionMatchShadowMaps.SetStructuredResource(ctx, 6,	resources.NonPixelShaderResource(data.lightBufferObject,	ctx), sizeof(GPUPointLight));
					resolutionMatchShadowMaps.SetStructuredResource(ctx, 7,	resources.NonPixelShaderResource(data.counterObject,		ctx), sizeof(uint32_t));

					resolutionMatchShadowMaps.SetCBV(ctx, 8, lightListConstantSet);

					resolutionMatchShadowMaps.NullFill(ctx);

					ctx.SetComputeDescriptorTable(0, resolutionMatchShadowMaps);
					ctx.Dispatch(clearShadowMapBuffer, { 1, 1, 1 });

					ctx.AddUAVBarrier(resources.GetResource(data.lightResolutionObject));

					ctx.SetPipelineState(resolutionMatchShadowsMaps);
					ctx.ExecuteIndirect(resources.IndirectArgs(data.argumentBufferObject, ctx), data.indirectLayout);

					// Write out
					ctx.CopyBufferRegion(
						{ resources.GetDeviceResource(resources.CopySrc(data.lightResolutionObject, ctx)) }, // Sources Offsets
						{ 0 },  // Source Offsets
						{ resources.GetDeviceResource(data.readBackHandle) }, // Destination
						{ 0 },  // Destination Offsets
						{ 64 * KILOBYTE },
						{ DASCopyDest },
						{ DASCopyDest });

					if(false)
					ctx.QueueReadBack(data.readBackHandle,
						[&, &readBackBuffers = readBackBuffers, &renderSystem = *ctx.renderSystem](ReadBackResourceHandle readBack)
						{
							auto [buffer, bufferSize] = renderSystem.OpenReadBackBuffer(readBack);

							auto temp = malloc(bufferSize);
							memcpy(temp, buffer, bufferSize);

							readBackBuffers.push_back(readBack);
						});
					*/

					ctx.EndEvent_DEBUG();
				}

				ctx.EndEvent_DEBUG();
			});

		return lightBufferData;
	}


	/************************************************************************************************/


	GBufferPass& ClusteredRender::FillGBuffer(
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		GatherPassesTask&				passes,
		const CameraHandle				camera,
		GBuffer&						gbuffer,
		ResourceHandle					depthTarget,
		BrushConstants&					entityConstants,
		const ResourceAllocation&		animationResources,
		ReserveConstantBufferFunction	reserveCB,
		iAllocator*						allocator)
	{
		using std::views::zip;
		using std::views::iota;

		auto& pass = frameGraph.AddNode<GBufferPass>(
			GBufferPass{
				gbuffer,
				passes,
				camera,
				reserveCB,
			},
			[&](FrameGraphNodeBuilder& builder, GBufferPass& data)
			{
				builder.AddDataDependency(passes);
				builder.AddNodeDependency(animationResources.node);

				data.entityConstants			= builder.ReadConstantBuffer(entityConstants.constants);
				data.AlbedoTargetObject			= builder.RenderTarget(gbuffer.albedo);
				data.MRIATargetObject			= builder.RenderTarget(gbuffer.MRIA);
				data.NormalTargetObject			= builder.RenderTarget(gbuffer.normal);
				data.depthBufferTargetObject	= builder.DepthTarget(depthTarget);
			},
			[&entityConstants, &animationResources, &gbuffer](GBufferPass& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

				auto& passes		= data.passes.GetData().passes;
				auto pass			= FindPass(passes.begin(), passes.end(), GBufferPassID);
				auto animatedPass	= FindPass(passes.begin(), passes.end(), GBufferAnimatedPassID);

				if ((!pass || !pass->pvs.size()) && (!animatedPass || !animatedPass->pvs.size()))
					return;


				struct ForwardDrawConstants
				{
					float LightCount;
					float t;
					uint2 WH;
				};

				const size_t entityBufferSize =
					AlignedSize<Brush::VConstantsLayout>();

				constexpr size_t passBufferSize =
					AlignedSize<Camera::ConstantBuffer>() +
					AlignedSize<ForwardDrawConstants>();

				auto passConstantBuffer		= data.reserveCB(passBufferSize);
				//auto entityConstantBuffer	= data.reserveCB(entityBufferSize);

				const auto cameraConstants	= ConstantBufferDataSet{ GetCameraConstants(data.camera), passConstantBuffer };
				const auto passConstants	= ConstantBufferDataSet{ ForwardDrawConstants{ 1, 1 }, passConstantBuffer };

				ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS));
				ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

				// Setup pipeline resources
				SetScissorAndViewports(
					ctx,
					std::tuple{
						data.gbuffer.albedo,
						data.gbuffer.MRIA,
						data.gbuffer.normal,
					});

				RenderTargetList renderTargets = {
					gbuffer.albedo,
					gbuffer.MRIA,
					gbuffer.normal,
				};

				ctx.SetRenderTargets(
					renderTargets,
					true,
					resources.GetResource(data.depthBufferTargetObject));

				// Setup Constants
				ctx.SetGraphicsConstantBufferView(1, cameraConstants);
				ctx.SetGraphicsConstantBufferView(3, passConstants);

				ctx.BeginEvent_DEBUG("G-Buffer Pass");
				ctx.BeginEvent_DEBUG("Static Objects");

				//ctx.TimeStamp(timeStats, 0);

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


				auto& materials			= MaterialComponent::GetComponent();
				auto& constantBuffer	= entityConstants.GetConstantBuffer();
				auto constants			= FlexKit::CreateCBIterator<Brush::VConstantsLayout>(constantBuffer);

				if(pass && pass->pvs.size())
				{
					TriMesh*					prevMesh	= nullptr;
					const TriMesh::LOD_Runtime* prevLOD		= nullptr;

					for (auto&& [I, brush] : zip(iota(0), pass->pvs))
					{
						auto& brush = pass->pvs[I];

						if (!brush->meshes.size())
							continue;

						const auto& material		= materials[brush->material];
						const auto beginConstants	= entityConstants.entityTable[I];

						const size_t meshCount	= brush->meshes.size();
						for (size_t J = 0; J < meshCount; J++)
						{
							auto mesh		= brush->meshes[J];
							auto* triMesh	= GetMeshResource(mesh);
							auto  lodIdx	= brush.LODlevel[J];
							auto  lod		= &triMesh->lods[lodIdx];

							if (triMesh != prevMesh || prevLOD != lod)
							{
								prevMesh	= triMesh;
								prevLOD		= lod;

								ctx.AddIndexBuffer(triMesh, lodIdx);
								ctx.AddVertexBuffers(
									triMesh,
									lodIdx,
									{
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
									});
							}

							auto&			submeshes		= lod->subMeshes;
							const size_t	subMeshesEnd	= submeshes.size();

							for (size_t K = 0; K < subMeshesEnd; K++)
							{
								const auto	subMesh		= submeshes[K];
								const auto& subMaterial	= (subMeshesEnd == 1) ? material : materials[material.subMaterials[K]];

								if (subMaterial.textureDescriptors.size != 0)
									ctx.SetGraphicsDescriptorTable(0, subMaterial.textureDescriptors);

								ctx.SetGraphicsConstantBufferView(2, constants[beginConstants + K]);

								ctx.DrawIndexed(
									subMesh.IndexCount,
									subMesh.BaseIndex);
							}
						}
					}
				}

				// skinned models
				ctx.EndEvent_DEBUG();

				if (!animatedPass || !animatedPass->pvs.size())
				{
					ctx.EndEvent_DEBUG();
					return;
				}

				ctx.BeginEvent_DEBUG("Skinned Objects");


				if(animatedPass && animatedPass->pvs.size())
				{
					auto& animatedBrushes = animatedPass->pvs;

					ctx.SetPipelineState(resources.GetPipelineState(GBUFFERPASS_SKINNED));

					ctx.SetGraphicsConstantBufferView(1, cameraConstants);
					ctx.SetGraphicsConstantBufferView(3, passConstants);

					TriMesh*					prevMesh	= nullptr;
					const TriMesh::LOD_Runtime* prevLOD		= nullptr;

					for (auto&& [idx, brush] : zip(iota(0), animatedBrushes))
					{
						if (!brush->meshes.size())
							continue;

						const auto& material		= materials[brush->material];
						const auto beginConstants	= entityConstants.entityTable[brush.submissionID];

						const size_t meshCount	= brush->meshes.size();
						for (size_t J = 0; J < meshCount; J++)
						{
							auto mesh		= brush->meshes[J];
							auto* triMesh	= GetMeshResource(mesh);
							auto  lodIdx	= brush.LODlevel[J];
							auto  lod		= &triMesh->lods[lodIdx];

							if (triMesh != prevMesh || prevLOD != lod)
							{
								prevMesh	= triMesh;
								prevLOD		= lod;

								ctx.AddIndexBuffer(triMesh, lodIdx);
								ctx.AddVertexBuffers(
									triMesh,
									lodIdx,
									{
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1,
										VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2,
									});
							}

							auto&			submeshes		= lod->subMeshes;
							const size_t	subMeshesEnd	= submeshes.size();

							for (size_t K = 0; K < subMeshesEnd; K++)
							{
								const auto	subMesh		= submeshes[K];
								const auto& subMaterial	= (subMeshesEnd == 1) ? material : materials[material.subMaterials[K]];

								if (subMaterial.textureDescriptors.size != 0)
									ctx.SetGraphicsDescriptorTable(0, subMaterial.textureDescriptors);

								auto poseBuffer = resources.GetResource(animationResources.handles[0]);
								ctx.SetGraphicsConstantBufferView(2, constants[beginConstants + K]);
								ctx.SetGraphicsShaderResourceView(7, poseBuffer);

								ctx.DrawIndexed(
									subMesh.IndexCount,
									subMesh.BaseIndex);
							}
						}
					}
				}

				ctx.EndEvent_DEBUG();
				ctx.EndEvent_DEBUG();
				//ctx.TimeStamp(timeStats, 1u);
			}
			);

		return pass;
	}


	/************************************************************************************************/


	ClusteredDeferredShading& ClusteredRender::ClusteredShading(
		UpdateDispatcher&				dispatcher,
		FrameGraph&						frameGraph,
		ShadowMapPassData&				lightShadowMaps,
		LightGatherTask&				lightGather,
		GBufferPass&					gbufferPass,
		ResourceHandle					depthTarget,
		ResourceHandle					renderTarget,
		LightBufferUpdate&				lightPass,
		ReserveConstantBufferFunction	reserveCB,
		ReserveVertexBufferFunction		reserveVB,
		float							t,
		iAllocator*						allocator)
	{
		auto& pass = frameGraph.AddNode<ClusteredDeferredShading>(
			ClusteredDeferredShading{
				.gbuffer	= gbufferPass,
				.lights		= lightGather,
				.lightPass	= lightPass,
				.shadowPass	= lightShadowMaps,
			},
			[&](FrameGraphNodeBuilder& builder, ClusteredDeferredShading& data)
			{
				builder.AddNodeDependency(lightShadowMaps.multiPass);
				builder.AddDataDependency(lightGather);

				auto& renderSystem				= frameGraph.GetRenderSystem();

				for (const auto shadowMap : lightShadowMaps.acquireMaps)
					builder.ReadTransition(shadowMap, DASPixelShaderResource, { Sync_All, Sync_PixelShader });

				data.pointLightHandles			= &lightGather.GetData().lights;
				data.AlbedoTargetObject			= builder.Common(gbufferPass.gbuffer.albedo);
				data.NormalTargetObject			= builder.Common(gbufferPass.gbuffer.normal);
				data.MRIATargetObject			= builder.Common(gbufferPass.gbuffer.MRIA);
				data.depthBufferTargetObject	= builder.PixelShaderResource(depthTarget);
				data.lightMapObject				= builder.ReadTransition(lightPass.indexBufferObject,		DASPixelShaderResource);
				data.lightListBuffer			= builder.ReadTransition(lightPass.lightListBuffer,			DASPixelShaderResource);
				data.lightLists					= builder.ReadTransition(lightPass.lightLists,				DASPixelShaderResource);

				data.renderTargetObject			= builder.AcquireVirtualResource(
													GPUResourceDesc::UAVTexture(
														builder.GetRenderSystem().GetTextureWH(renderTarget),
														DeviceFormat::R16G16B16A16_FLOAT, true), FlexKit::DASRenderTarget, VirtualResourceScope::Frame);

				data.pointLightBufferObject		= builder.ReadTransition(lightPass.lightBufferObject,		DASPixelShaderResource);

				data.passConstants				= reserveCB(128 * KILOBYTE);
				data.passVertices				= reserveVB(sizeof(float4) * 6);

				builder.SetDebugName(data.renderTargetObject, "renderTargetObject");
			}, 
			[camera = gbufferPass.camera, renderTarget, t, &rootSignature = this->rootSignature, &shadowMaps = lightShadowMaps.acquireMaps, &gbuffer = gbufferPass.gbuffer]
			(ClusteredDeferredShading& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

				ctx.BeginEvent_DEBUG("Clustered Shading");

				LightComponent&	lightComponent	= LightComponent::GetComponent();
				const auto&		visableLights	= *data.pointLightHandles;

				auto& renderSystem			= resources.renderSystem();
				const auto WH				= resources.renderSystem().GetTextureWH(renderTarget);
				const auto cameraConstants	= GetCameraConstants(camera);
				const auto lightCount		= (uint32_t)visableLights.size();

				if (!lightCount)
					return;

				struct ShadowMap
				{
					float4x4 PV;
					float4x4 View;
				};

				struct
				{
					float2		WH;
					float2		WH_I;
					float		time;
					uint32_t	lightCount;
					float4		ambientLight;
				}	passConstants =
				{
					{(float)WH[0], (float)WH[1]},
					{1.0f / (float)WH[0], 1.0f / (float)WH[1]},
					t,
					lightCount,
					{ 0.1f, 0.1f, 0.1f, 0 }
				};

				struct
				{
					float4 Position;
				}   vertices[] =
				{
					float4{ 3,  -1, 0, 1 },
					float4{-1,  -1, 0, 1 },
					float4{-1,   3, 0, 1 },
				};

				const size_t descriptorTableSize = 20 + lightCount;

				DescriptorHeap descHeap;
				descHeap.Init2(ctx, rootSignature.GetDescHeap(0), descriptorTableSize, &allocator);
				descHeap.SetSRV(ctx, 0, gbuffer.albedo);
				descHeap.SetSRV(ctx, 1, gbuffer.MRIA);
				descHeap.SetSRV(ctx, 2, gbuffer.normal);
				descHeap.SetSRV(ctx, 4, resources.GetResource(data.depthBufferTargetObject), DeviceFormat::R32_FLOAT);
				descHeap.SetSRV(ctx, 5, resources.GetResource(data.lightMapObject));
				descHeap.SetStructuredResource(ctx, 6, resources.GetResource(data.lightLists), sizeof(uint2));
				descHeap.SetStructuredResource(ctx, 7, resources.GetResource(data.lightListBuffer), sizeof(uint32_t));
				descHeap.SetStructuredResource(ctx, 8, resources.PixelShaderResource(data.pointLightBufferObject, ctx), sizeof(GPULight));
				descHeap.SetStructuredResource(ctx, 9, resources.PixelShaderResource(data.lightPass.shadowMatrices, ctx), sizeof(float4x4));

				for (size_t shadowMapIdx = 0; shadowMapIdx < lightCount; shadowMapIdx++)
				{
					auto& light = lightComponent[visableLights[shadowMapIdx]];

					auto shadowMap = resources.GetResource(shadowMaps[shadowMapIdx]);

					if (shadowMap != InvalidHandle)
					{
						switch (light.type)
						{
						case LightType::PointLight:
							descHeap.SetSRVCubemap(ctx, 10 + shadowMapIdx, shadowMap, DeviceFormat::R32_FLOAT);
							break;
						case LightType::SpotLight:
						case LightType::SpotLightBasicShadows:
						case LightType::Direction:
							descHeap.SetSRV(ctx, 10 + shadowMapIdx, shadowMap, DeviceFormat::R32_FLOAT);
							break;
						}
					}
				}

				descHeap.NullFill(ctx, descriptorTableSize);

#if 1
				ctx.ClearRenderTarget(resources.GetResource({ data.renderTargetObject }));

				ctx.SetRootSignature(rootSignature);
				ctx.SetPipelineState(resources.GetPipelineState(SHADINGPASS));
				ctx.SetPrimitiveTopology(EIT_TRIANGLELIST);

				ctx.SetScissorAndViewports({ renderTarget });
				ctx.SetRenderTargets({ resources.GetResource({ data.renderTargetObject })}, false, {});
				ctx.SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
				ctx.SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
				ctx.SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
				ctx.SetGraphicsDescriptorTable(4, descHeap);
				ctx.SetGraphicsDescriptorTable(5, descHeap);

				ctx.Draw(3);
#else
				DescriptorHeap uavHeap;
				uavHeap.Init2(ctx, resources.renderSystem().Library.RSDefault.GetDescHeap(1), 1, &allocator);
				uavHeap.SetUAVTexture(ctx, 0, resources.UAV(data.renderTargetObject, ctx));

				//ctx.ClearRenderTarget(resources.GetResource({ data.renderTargetObject }));

				ctx.SetComputeRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(resources.GetPipelineState(SHADINGPASSCOMPUTE));
				ctx.SetComputeConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
				ctx.SetComputeConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });
				ctx.SetComputeDescriptorTable(4, descHeap);
				ctx.SetComputeDescriptorTable(5, uavHeap);

				//ctx.TimeStamp(timeStats, 4);
				ctx.Dispatch({ WH[0] / 16, WH[0] / 16, 1 });
				ctx.AddUAVBarrier(resources.GetResource(data.renderTargetObject));
				//ctx.TimeStamp(timeStats, 5);

				//ctx.ResolveQuery(timeStats, 0, 8, resources.GetObjectResource(timingReadBack), 0);
				//ctx.QueueReadBack(timingReadBack);
				ctx.EndEvent_DEBUG();
				ctx.Dispatch({ WH[0] / 16, WH[0] / 16, 1 });
#endif

				//ctx.TimeStamp(timeStats, 4);
				//ctx.TimeStamp(timeStats, 5);

				//ctx.ResolveQuery(timeStats, 0, 8, resources.GetObjectResource(timingReadBack), 0);
				//ctx.QueueReadBack(timingReadBack);
				ctx.EndEvent_DEBUG();
			});

		return pass;
	}


	void ClusteredRender::ReleaseFrameResources(
		FrameGraph&					frameGraph,
		LightBufferUpdate&			lightPass,
		ClusteredDeferredShading&	clusteredDeferredShading)
	{
		struct _{};
		frameGraph.AddNode<_>(
			_{},
			[&](FrameGraphNodeBuilder& builder, _& data)
			{
				builder.ReleaseVirtualResource(clusteredDeferredShading.renderTargetObject);
				builder.ReleaseVirtualResource(lightPass.lightLists);
				builder.ReleaseVirtualResource(lightPass.indexBufferObject);
				builder.ReleaseVirtualResource(lightPass.lightListBuffer);
				builder.ReleaseVirtualResource(lightPass.lightBufferObject);
			},
			[](_& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

			});
	}

	/************************************************************************************************/


	DEBUGVIS_DrawBVH& ClusteredRender::DEBUGVIS_DrawLightBVH(
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
				data.lightBVH		= builder.ReadTransition(lightBufferUpdate.lightBVH,              DASNonPixelShaderResource);
				data.clusters		= builder.ReadTransition(lightBufferUpdate.clusterBufferObject,   DASNonPixelShaderResource);
				data.renderTarget	= builder.RenderTarget(renderTarget);
				data.indirectArgs	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(512), DASUAV);
				data.counterBuffer	= builder.ReadTransition(lightBufferUpdate.counterObject,     DASNonPixelShaderResource);
				data.pointLights	= builder.ReadTransition(lightBufferUpdate.lightBufferObject, DASNonPixelShaderResource);
				data.camera			= camera;

				builder.SetDebugName(data.indirectArgs, "indirectArgs");

				builder.ReleaseVirtualResource(lightBufferUpdate.counterObject);
				builder.ReleaseVirtualResource(lightBufferUpdate.lightBVH);
				builder.ReleaseVirtualResource(lightBufferUpdate.lightLookupObject);
				builder.ReleaseVirtualResource(lightBufferUpdate.lightCounterObject);
				builder.ReleaseVirtualResource(lightBufferUpdate.argumentBufferObject);
			},
			[&, mode = mode](DEBUGVIS_DrawBVH& data, ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				ProfileFunction();

				auto debugBVHVISPSO			= resources.GetPipelineState(LIGHTBVH_DEBUGVIS_PSO);
				auto debugClusterVISPSO		= resources.GetPipelineState(CLUSTER_DEBUGVIS_PSO);
				auto debugClusterArgsVISPSO	= resources.GetPipelineState(CLUSTER_DEBUGARGSVIS_PSO);
				auto debugLightVISPSO		= resources.GetPipelineState(CREATELIGHTDEBUGVIS_PSO);

				const auto cameraConstants = GetCameraConstants(data.camera);

				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) }, false);
				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });

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
				descHeap.SetStructuredResource(ctx, 0, resources.NonPixelShaderResource(data.lightBVH, ctx), 4, 0);
				descHeap.SetStructuredResource(ctx, 1, resources.NonPixelShaderResource(data.clusters, ctx), sizeof(GPUCluster), 0);
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

				size_t accumlator	= 0u;
				size_t nodeCount	= uint32_t(std::ceilf(float(lightCount) / BVH_ELEMENT_COUNT));

				const uint32_t passCount = uint32_t(std::floor(std::log(lightCount) / std::log(BVH_ELEMENT_COUNT)));

				for (uint32_t I = 0; I < passCount; I++)
				{
					nodeCount    = uint32_t(std::floor(float(nodeCount) / BVH_ELEMENT_COUNT));
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
					ctx.ExecuteIndirect(resources.IndirectArgs(data.indirectArgs, ctx), draw);
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


	void ClusteredRender::DEBUGVIS_BVH(
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
				auto renderTargets  = { resources.GetResource(desc.renderTargetObject) };

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

}   /************************************************************************************************/



/**********************************************************************

Copyright (c) 2014-2023 Robert May

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
