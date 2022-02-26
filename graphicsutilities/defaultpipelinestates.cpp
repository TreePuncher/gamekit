/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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

#include "defaultpipelinestates.h"
#include "CoreSceneObjects.h"


namespace FlexKit
{
	/************************************************************************************************/


	ID3D12PipelineState* CreateDrawTriStatePSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("DrawRect_VS",	"vs_6_0", "assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawRect",		"ps_6_0", "assets\\shaders\\pshader.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		Depth_Desc.DepthEnable = false;

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
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/

	ID3D12PipelineState* CreateTexturedTriStatePSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("DrawRect_VS",		"vs_6_0", "assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawRectTextured",	"ps_6_0", "assets\\shaders\\pshader.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		Depth_Desc.DepthEnable	= false;

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
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateTexturedTriStateDEBUGPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("VS",	"vs_6_0", "assets\\shaders\\temp.hlsl");
		auto DrawRectPShader = RS->LoadShader("PS",	"ps_6_0", "assets\\shaders\\temp.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC	(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		Depth_Desc.DepthEnable	= false;

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
		}

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


	/************************************************************************************************/


	ID3D12PipelineState* CreateDrawLineStatePSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("DrawRect_VS",	"vs_6_0",	"assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawRect",		"ps_6_0",	"assets\\shaders\\pshader.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 0, 8,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode		= D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.PS                    = DrawRectPShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
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

	/*
	struct Vert
	{
		float4 POS
		float4 COLOR
		float2 UV
	};
	*/
	ID3D12PipelineState* CreateDraw2StatePSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("V10Main",	"vs_6_0",	"assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawRect",	"ps_6_0",	"assets\\shaders\\pshader.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, sizeof(float4),	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, sizeof(float4)*2,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode		= D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
		Rast_Desc.CullMode		= D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		Depth_Desc.DepthEnable	= false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
			PSO_Desc.VS                    = DrawRectVShader;
			PSO_Desc.PS                    = DrawRectPShader;
			PSO_Desc.RasterizerState       = Rast_Desc;
			PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PSO_Desc.SampleMask            = UINT_MAX;
			PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
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


    ID3D12PipelineState* CreateDrawTri3DStatePSO(RenderSystem* RS)
	{
		auto DrawRectVShader = RS->LoadShader("V11Main",	        "vs_6_0",	"assets\\shaders\\vshader.hlsl");
		auto DrawRectPShader = RS->LoadShader("DrawFlatTriangle",	"ps_6_0",	"assets\\shaders\\pshader.hlsl");

		D3D12_INPUT_ELEMENT_DESC InputElements[] = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 0,	                D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		0, sizeof(float4),	    D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,			0, sizeof(float4)*2,    D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Rast_Desc.FillMode		= D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		Rast_Desc.CullMode		= D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
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
			PSO_Desc.NumRenderTargets      = 1;
			PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
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


	ID3D12PipelineState* LoadOcclusionState(RenderSystem* RS)
	{
		Shader VShader = RS->LoadShader("VMain", "vs_6_0", "assets\\shaders\\VShader.hlsl" );

		D3D12_INPUT_ELEMENT_DESC InputElements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
			Rast_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE::D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
		}

		D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); {
			Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			//Depth_Desc.DepthEnable	= false;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GDesc = {};
		GDesc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
		GDesc.VS                    = VShader;
		GDesc.PS                    = { nullptr, 0 };
		GDesc.RasterizerState       = Rast_Desc;
		GDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		GDesc.SampleMask            = UINT_MAX;
		GDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GDesc.NumRenderTargets      = 0;
		GDesc.SampleDesc.Count      = 1;
		GDesc.SampleDesc.Quality    = 0;
		GDesc.DSVFormat             = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		GDesc.InputLayout           = { InputElements, 1 };
		GDesc.DepthStencilState     = Depth_Desc;
		
		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateGraphicsPipelineState(&GDesc, IID_PPV_ARGS(&PSO));
		CheckHR(HR, ASSERTONFAIL("Failed to Create PSO for Occlusion Culling!"));

		return PSO;
	}


	/************************************************************************************************/

    /*

	OcclusionCuller CreateOcclusionCuller(RenderSystem* RS, uint32_t count, uint2 OcclusionBufferSize, bool Float)
	{
		FK_ASSERT(0);
		//RegisterPSOLoader(EPIPELINESTATES::OCCLUSION_CULLING, LoadOcclusionState);
		//QueuePSOLoad(RS, EPIPELINESTATES::OCCLUSION_CULLING);

		//auto NewRes = CreateGPUResource(RS, count * 8, "OCCLUSIONCULLERRESULTS");

		D3D12_QUERY_HEAP_DESC Desc;
		Desc.Count		= count;
		Desc.Type		= D3D12_QUERY_HEAP_TYPE_OCCLUSION;
		Desc.NodeMask	= 0;

		ID3D12QueryHeap* Heap[3] = { nullptr, nullptr, nullptr };
		
		HRESULT HR;
		HR = RS->pDevice->CreateQueryHeap(&Desc, IID_PPV_ARGS(Heap + 0)); FK_ASSERT(HR, "FAILED TO CREATE QUERY HEAP!");
		HR = RS->pDevice->CreateQueryHeap(&Desc, IID_PPV_ARGS(Heap + 1)); FK_ASSERT(HR, "FAILED TO CREATE QUERY HEAP!");
		HR = RS->pDevice->CreateQueryHeap(&Desc, IID_PPV_ARGS(Heap + 2)); FK_ASSERT(HR, "FAILED TO CREATE QUERY HEAP!");

		/*
		FlexKit::Texture2D_Desc Tex2D_Desc;
		Tex2D_Desc.CV				= true;
		Tex2D_Desc.FLAGS			= FlexKit::SPECIALFLAGS::DEPTHSTENCIL;
		Tex2D_Desc.Format			= DeviceFormat::D32_FLOAT;
		Tex2D_Desc.Width			= OcclusionBufferSize[0];
		Tex2D_Desc.Height			= OcclusionBufferSize[1];
		Tex2D_Desc.Read				= false;
		Tex2D_Desc.MipLevels		= 1;
		Tex2D_Desc.initialData		= nullptr;
		Tex2D_Desc.RenderTarget		= true;
		Tex2D_Desc.UAV				= false;
		Tex2D_Desc.Write			= false;
		*/

        /*
		DepthBuffer DB;
		FlexKit::DepthBuffer_Desc Depth_Desc;
		Depth_Desc.BufferCount		= 3;
		Depth_Desc.Float32			= true;
		Depth_Desc.InverseDepth		= true;

		//auto Res = CreateDepthBuffer(RS, OcclusionBufferSize, Depth_Desc, &DB);
		//FK_ASSERT(Res, "FAILED TO CREATE OCCLUSION BUFFER!");
		
		for (size_t I = 0; I < 3; ++I)
			SETDEBUGNAME(DB.Buffer[I], "OCCLUSION CULLER DEPTH BUFFER");

		OcclusionCuller Out;
		Out.Head		    = 0;
		Out.Max			    = count;
		Out.Heap[0]		    = Heap[0];
		Out.Heap[1]		    = Heap[1];
		Out.Heap[2]		    = Heap[2];
		//Out.Predicates	    = NewRes;
		Out.OcclusionBuffer = DB;
		Out.HW              = OcclusionBufferSize;

        return {};
	}
    */

	/************************************************************************************************/


        /*
	void OcclusionPass(RenderSystem* RS, PVS* Set, OcclusionCuller* OC, ID3D12GraphicsCommandList* CL, Camera* C)
	{
		FK_ASSERT(0);
		auto OCHeap = OC->Heap[OC->Idx];
		auto DSV	= RS->_ReserveDSVHeap(1);


		D3D12_VIEWPORT VPs[1];
		VPs[0].Width		= OC->HW[0];
		VPs[0].Height		= OC->HW[1];
		VPs[0].MaxDepth		= 1.0f;
		VPs[0].MinDepth		= 0.0f;
		VPs[0].TopLeftX		= 0;
		VPs[0].TopLeftY		= 0;

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format				= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		DSVDesc.Texture2D.MipSlice	= 0;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		RS->pDevice->CreateDepthStencilView(GetCurrent(&OC->OcclusionBuffer), &DSVDesc, (D3D12_CPU_DESCRIPTOR_HANDLE)DSV);

		CL->RSSetViewports(1, VPs);
		CL->OMSetRenderTargets(0, nullptr, false, &(D3D12_CPU_DESCRIPTOR_HANDLE)DSV);
        CL->SetPipelineState(RS->GetPSO(OCCLUSION_CULLING));
		CL->SetGraphicsRootSignature(RS->Library.RS6CBVs4SRVs);

		for (uint32_t I = 0; I < Set->size(); ++I)
        {
            uint32_t QueryID = OC->GetNext();

			auto& P			= Set->at(I);
			P.OcclusionID	= QueryID;

			CL->BeginQuery(OCHeap, D3D12_QUERY_TYPE::D3D12_QUERY_TYPE_BINARY_OCCLUSION, QueryID);

			auto MeshHande		= (P.D->Occluder == InvalidHandle_t) ? P.D->MeshHandle : P.D->Occluder;
			auto DHeapPosition	= RS->_GetDescTableCurrentPosition_GPU();
			auto DHeap			= RS->_ReserveDescHeap(8);
			auto DTable			= DHeap;

			{	// Fill Descriptor Table With Nulls
                FK_ASSERT(0);
				//auto Next = Push2DSRVToDescHeap(RS, RS->NullSRV, DHeap);
				//Next = Push2DSRVToDescHeap(RS, RS->NullSRV, Next);
				//Next = Push2DSRVToDescHeap(RS, RS->NullSRV, Next);
				//Next = Push2DSRVToDescHeap(RS, RS->NullSRV, Next);
				//Next = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), Next, 1024);
				//Next = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), Next, 1024);
				//Next = PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), Next, 1024);
				//PushCBToDescHeap(RS, RS->NullConstantBuffer.Get(), Next, 1024);
			}

			CL->SetGraphicsRootDescriptorTable(0, DHeapPosition);
			//CL->SetGraphicsRootConstantBufferView(1, C->Buffer.Get()->GetGPUVirtualAddress());
			//CL->SetGraphicsRootConstantBufferView(2, P.D->VConstants.Get()->GetGPUVirtualAddress());
			CL->SetGraphicsRootConstantBufferView(3, RS->NullConstantBuffer.Get()->GetGPUVirtualAddress());
			CL->SetGraphicsRootConstantBufferView(4, RS->NullConstantBuffer.Get()->GetGPUVirtualAddress());

			auto CurrentMesh	= GetMeshResource(MeshHande);
			size_t IBIndex		= CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
			size_t IndexCount	= GetMeshResource(P.D->MeshHandle)->IndexCount;

			D3D12_INDEX_BUFFER_VIEW		IndexView;
			IndexView.BufferLocation	= GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress();
			IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			IndexView.SizeInBytes		= (uint32_t)IndexCount * 32;

			static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
			FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, CurrentMesh, VBViews));
			
			CL->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CL->IASetIndexBuffer(&IndexView);
			CL->IASetVertexBuffers(0, (uint32_t)VBViews.size(), VBViews.begin());
			CL->DrawIndexedInstanced((uint32_t)IndexCount, 1, 0, 0, 0);
			CL->EndQuery(OCHeap, D3D12_QUERY_TYPE::D3D12_QUERY_TYPE_BINARY_OCCLUSION, QueryID);
		}

		CL->ResolveQueryData(OCHeap, D3D12_QUERY_TYPE_BINARY_OCCLUSION, 0, (uint32_t)Set->size(), OC->Predicates.Get(), 0);

		CL->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(OC->Predicates.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
	}
        */


    /************************************************************************************************/


    ID3D12PipelineState* LoadClearRenderTarget_RG32(RenderSystem* renderSystem)
    {
        auto VShader = renderSystem->LoadShader("FullscreenQuad", "vs_6_0", "assets\\shaders\\FullscreenQuad.hlsl");
        auto PShader = renderSystem->LoadShader("ClearRenderTargetUINT2", "ps_6_0", "assets\\shaders\\ClearRenderTarget.hlsl");

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC{ D3D12_DEFAULT };
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC{ D3D12_DEFAULT };
        Depth_Desc.DepthEnable = false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature = renderSystem->Library.RSDefault;
            PSO_Desc.VS = VShader;
            PSO_Desc.PS = PShader;
            PSO_Desc.RasterizerState = Rast_Desc;
            PSO_Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets = 1;
            PSO_Desc.RTVFormats[0] = DXGI_FORMAT_R32G32_UINT;
            PSO_Desc.SampleDesc.Count = 1;
            PSO_Desc.SampleDesc.Quality = 0;
            PSO_Desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
            PSO_Desc.InputLayout = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState = Depth_Desc;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
        }


        ID3D12PipelineState* PSO = nullptr;
        const auto HR = renderSystem->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }



	/************************************************************************************************/
}
