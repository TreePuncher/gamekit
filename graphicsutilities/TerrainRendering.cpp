/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#include "TerrainRendering.h"
#include "intersection.h"

#include <stdio.h>

#pragma warning( disable : 4267 )

namespace FlexKit
{
	/************************************************************************************************/

	ID3D12PipelineState* CreateCullTerrainComputePSO(RenderSystem* renderSystem)
	{
		auto cullTerrain_shader_VS = renderSystem->LoadShader("CP_PassThroughVS", "vs_6_0", "assets\\cullterrain.hlsl");
		auto cullTerrain_shader_GS = renderSystem->LoadShader("CullTerrain", "gs_6_0", "assets\\cullterrain.hlsl");


		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	48, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		/*
		typedef struct D3D12_SO_DECLARATION_ENTRY
		{
		UINT Stream;
		LPCSTR SemanticName;
		UINT SemanticIndex;
		BYTE StartComponent;
		BYTE ComponentCount;
		BYTE OutputSlot;
		} 	D3D12_SO_DECLARATION_ENTRY;
		*/

		D3D12_SO_DECLARATION_ENTRY SO_Entries[] = {
			{ 0, "REGION",		0, 0, 4, 0 },
			{ 0, "TEXCOORD",	0, 0, 4, 0 },
			{ 0, "TEXCOORD",	1, 0, 4, 0 },
			{ 0, "TEXCOORD",	2, 0, 4, 0 },
			{ 1, "REGION",		0, 0, 4, 1 },
			{ 1, "TEXCOORD",	0, 0, 4, 1 },
			{ 1, "TEXCOORD",	1, 0, 4, 1 },
			{ 1, "TEXCOORD",	2, 0, 4, 1 },
		};

		UINT Strides = sizeof(ViewableRegion);
		UINT SO_Strides[] = {
			Strides,
			Strides,
			Strides,
			Strides,
		};

		D3D12_STREAM_OUTPUT_DESC SO_Desc = {};{
			SO_Desc.NumEntries		= 8;
			SO_Desc.NumStrides		= 4;
			SO_Desc.pBufferStrides	= SO_Strides;
			SO_Desc.pSODeclaration	= SO_Entries;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
			PSO_Desc.VS                            = cullTerrain_shader_VS;
			PSO_Desc.GS                            = cullTerrain_shader_GS;
			PSO_Desc.SampleMask                    = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets              = 0;
			PSO_Desc.SampleDesc.Count              = 1;
			PSO_Desc.SampleDesc.Quality            = 0;
			PSO_Desc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements) / sizeof(InputElements[0]) };
			PSO_Desc.DepthStencilState.DepthEnable = false;
			PSO_Desc.StreamOutput				   = SO_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardRenderTerrainPSO(RenderSystem* renderSystem)
	{
		auto forwardRenderTerrain_shader_VS = renderSystem->LoadShader("CP_PassThroughVS", "vs_6_0", "assets\\forwardRenderTerrain.hlsl");
		auto forwardRenderTerrain_shader_GS = renderSystem->LoadShader("GS_RenderTerrain", "gs_6_0", "assets\\forwardRenderTerrain.hlsl");
		auto forwardRenderTerrain_shader_PS = renderSystem->LoadShader("PS_RenderTerrain", "ps_6_0", "assets\\forwardRenderTerrain.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	48, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); {
			Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
			PSO_Desc.VS                            = forwardRenderTerrain_shader_VS;
			PSO_Desc.GS                            = forwardRenderTerrain_shader_GS;
			PSO_Desc.PS							   = forwardRenderTerrain_shader_PS;
			PSO_Desc.RasterizerState			   = Rast_Desc;
			PSO_Desc.SampleMask                    = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets              = 1;
			PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count              = 1;
			PSO_Desc.SampleDesc.Quality            = 0;
			PSO_Desc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements) / sizeof(InputElements[0]) };
			PSO_Desc.DepthStencilState			   = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardRenderTerrainWireFramePSO(RenderSystem* renderSystem)
	{
		auto forwardRenderTerrain_shader_VS = renderSystem->LoadShader("CP_PassThroughVS",		"vs_6_0", "assets\\forwardRenderTerrain.hlsl");
		auto forwardRenderTerrain_shader_GS = renderSystem->LoadShader("GS_RenderTerrain",		"gs_6_0", "assets\\forwardRenderTerrain.hlsl");
		auto ShaderPaint_Wire				= renderSystem->LoadShader("PS_RenderTerrainDEBUG",	"ps_6_0", "assets\\forwardRenderTerrainDEBUG.hlsl");


		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "REGION",	  0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	32, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT,	0,	48, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
			Rast_Desc.FillMode	= D3D12_FILL_MODE_WIREFRAME;
			Rast_Desc.DepthBias = -10;
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); {
			Depth_Desc.DepthEnable	= true;
			Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature                = renderSystem->Library.RS4CBVs_SO;
			PSO_Desc.VS                            = forwardRenderTerrain_shader_VS;
			PSO_Desc.GS                            = forwardRenderTerrain_shader_GS;
			PSO_Desc.PS							   = ShaderPaint_Wire;
			PSO_Desc.RasterizerState			   = Rast_Desc;
			PSO_Desc.SampleMask                    = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			PSO_Desc.NumRenderTargets              = 1;
			PSO_Desc.RTVFormats[0]				   = DXGI_FORMAT_R16G16B16A16_FLOAT;
			PSO_Desc.SampleDesc.Count              = 1;
			PSO_Desc.SampleDesc.Quality            = 0;
			PSO_Desc.RasterizerState               = Rast_Desc;
			PSO_Desc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.DSVFormat                     = DXGI_FORMAT_D32_FLOAT;
			PSO_Desc.InputLayout                   = { InputElements, sizeof(InputElements) / sizeof(InputElements[0]) };
			PSO_Desc.DepthStencilState			   = Depth_Desc;
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));

		return PSO;
	}


}	/************************************************************************************************/
