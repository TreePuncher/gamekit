/**********************************************************************

Copyright (c) 2014-2018 Robert May

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


#include "stdafx.h"
#include "WorldRender.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>

namespace FlexKit
{
	ID3D12PipelineState* CreateForwardDrawPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS", "vs_5_0",	"assets\\forwardRender.hlsl");
		auto DrawRectPShader = LoadShader("Forward_PS", "Forward_PS", "ps_5_0",	"assets\\forwardRender.hlsl");

		FINALLY
			
		Release(&DrawRectVShader);
		Release(&DrawRectPShader);

		FINALLYOVER

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
				//{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 2, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS4CBVs4SRVs;
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


	ID3D12PipelineState* CreateOcclusionDrawPSO(RenderSystem* RS)
	{
		auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS", "vs_5_0",	"assets\\forwardRender.hlsl");

		FINALLY
			
		Release(&DrawRectVShader);

		FINALLYOVER

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
			PSO_Desc.pRootSignature        = RS->Library.RS4CBVs4SRVs;
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


	void WorldRender::DefaultRender(PVS& Drawables, Camera& Camera, SceneNodes* Nodes, WorldRender_Targets& Targets, FrameGraph& Graph, iAllocator* Memory)
	{
		ClearDepthBuffer(Graph, Targets.DepthTarget,	1.0f);
		ClearDepthBuffer(Graph, OcclusionBuffer,		1.0f);

		RenderDrawabledPBR_Forward(Drawables, Camera, Nodes, Targets, Graph, Memory);
	}

	void WorldRender::RenderDrawabledPBR_Forward(
		PVS&					Drawables, 
		Camera&					Camera,
		SceneNodes*				Nodes,
		WorldRender_Targets&	Targets,
		FrameGraph&				Graph, 
		iAllocator*				Memory)
	{
		struct ForwardDraw
		{
			TriMeshHandle	Mesh;
			size_t			ConstantBufferOffset;
			size_t			OcclusionIdx;
		};

		typedef Vector<ForwardDraw> ForwardDrawableList;
		struct ForwardDrawPass
		{
			FrameResourceHandle		BackBuffer;
			FrameResourceHandle		DepthBuffer;
			FrameResourceHandle		OcclusionBuffer;
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	ConstantBuffer;
			ForwardDrawableList		Draws;
			size_t					CameraConsantsOffset;
		};

		auto& Pass = Graph.AddNode<ForwardDrawPass>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, ForwardDrawPass& Data)
		{
			Data.BackBuffer		 = Builder.WriteBackBuffer	(RS->GetTag(Targets.RenderTarget));
			Data.DepthBuffer	 = Builder.WriteDepthBuffer	(RS->GetTag(Targets.DepthTarget));

			if(OcclusionCulling)
				Data.OcclusionBuffer = Builder.WriteDepthBuffer	(RS->GetTag(OcclusionBuffer));

			Data.Draws = ForwardDrawableList{ Memory };
			Camera::BufferLayout CameraConstants = Camera.GetConstants(Nodes, 0.0f);

			auto CameraConsantsOffset = BeginNewConstantBuffer(ConstantBuffer, Graph.Resources);
			PushConstantBufferData(CameraConstants, ConstantBuffer, Graph.Resources);

			for (auto Viewable : Drawables)
			{
				Drawable::VConsantsLayout	Constants = Viewable.D->GetConstants(Nodes);

				auto CBOffset = BeginNewConstantBuffer(ConstantBuffer, Graph.Resources);
				PushConstantBufferData(Constants, ConstantBuffer, Graph.Resources);
				
				Data.Draws.push_back({ Viewable.D->MeshHandle, CBOffset });
			}
		},
			[=](const ForwardDrawPass& Data, const FrameResources& Resources, Context* Ctx)
		{
			if (OcclusionCulling)
			{
				Ctx->SetScissorAndViewports({ Targets.RenderTarget });
				Ctx->SetRenderTargets(
					{	(DescHeapPOS)Resources.GetRenderTargetObject(Data.BackBuffer) }, true,
						(DescHeapPOS)Resources.GetRenderTargetObject(Data.OcclusionBuffer));

				Ctx->SetRootSignature(Resources.RenderSystem->Library.RS4CBVs4SRVs);
			}
			else
				Ctx->SetPredicate(false);

			// Setup Initial Shading State
			Ctx->SetScissorAndViewports({Targets.RenderTarget});
			Ctx->SetRenderTargets(
				{ (DescHeapPOS)Resources.GetRenderTargetObject(Data.BackBuffer) }, true, 
				  (DescHeapPOS)Resources.GetRenderTargetObject(Data.DepthBuffer));

			Ctx->SetRootSignature(Resources.RenderSystem->Library.RS4CBVs4SRVs);
			Ctx->SetPipelineState(Resources.GetPipelineState(FORWARDDRAW));
			Ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
			Ctx->SetGraphicsConstantBufferView(1, Data.ConstantBuffer, Data.CameraConsantsOffset);

			for (auto D : Data.Draws)
			{
				auto* TriMesh = &GT->Geometry[D.Mesh];

				if(OcclusionCulling)
					Ctx->SetPredicate(true, OcclusionQueries, D.OcclusionIdx * 8);

				Ctx->AddIndexBuffer(TriMesh);
				Ctx->AddVertexBuffers(TriMesh, 
					{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
						//VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
					});

				Ctx->SetGraphicsConstantBufferView(2, Data.ConstantBuffer, D.ConstantBufferOffset);
				Ctx->DrawIndexedInstanced(TriMesh->IndexCount, 0, 0, 1, 0);
			}
		});
	}

}