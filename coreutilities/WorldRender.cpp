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


#include "WorldRender.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>

namespace FlexKit
{	/************************************************************************************************/


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


	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardDrawInstancedPSO(RenderSystem* RS)
	{
		auto VShader = LoadShader("VMain",		"InstanceForward_VS",	"vs_5_0", "assets\\DrawInstancedVShader.hlsl");
		auto PShader = LoadShader("Forward_PS", "Forward_PS",			"ps_5_0", "assets\\forwardRender.hlsl");

		EXITSCOPE(
			Release(&VShader);
			Release(&PShader);
		);

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
				{ "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 2, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

				{ "INSTANCEWT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		3, 0,   D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWT",	1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		3, 16,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWT",	2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		3, 32,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
				{ "INSTANCEWT",	3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,		3, 48,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};


		D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		Depth_Desc.DepthEnable	= true;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
			PSO_Desc.pRootSignature        = RS->Library.RS4CBVs4SRVs;
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


	/************************************************************************************************/


	void WorldRender::DefaultRender(PVS& Drawables, CameraHandle Camera, WorldRender_Targets& Targets, FrameGraph& Graph, SceneDescription& scene, iAllocator* Memory)
	{
		RenderDrawabledPBR_ForwardPLUS(Drawables, Camera, Targets, Graph, scene, Memory);
	}


	/************************************************************************************************/
	

	void WorldRender::RenderDrawabledPBR_ForwardPLUS(
		PVS&					Drawables, 
		CameraHandle			Camera,
		WorldRender_Targets&	Targets,
		FrameGraph&				Graph, 
		SceneDescription&		desc,
		iAllocator*				Memory)
	{
		struct ForwardDraw
		{
			TriMeshHandle	Mesh;
			size_t			ConstantBufferOffset;
			size_t			OcclusionIdx;
		};

		struct ForwardDrawConstants
		{
			float LightCount = 10;
		};

		typedef Vector<ForwardDraw> ForwardDrawableList;
		struct ForwardDrawPass
		{
			FrameResourceHandle		BackBuffer;
			FrameResourceHandle		DepthBuffer;
			FrameResourceHandle		OcclusionBuffer;
			FrameResourceHandle		lightMap;
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	ConstantBuffer;
			ConstantBufferHandle	pointLightBuffer;
			ForwardDrawableList		Draws;
			size_t					ConstantsOffset;
			size_t					CameraConstantsOffset;
			FlexKit::DescriptorHeap	Heap; // Null Filled
		};

		auto& Pass = Graph.AddNode<ForwardDrawPass>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, ForwardDrawPass& Data)
			{
				Data.BackBuffer			= Builder.WriteRenderTarget(RS->GetTag(Targets.RenderTarget));
				Data.DepthBuffer		= Builder.WriteDepthBuffer	(RS->GetTag(Targets.DepthTarget));
				Data.ConstantBuffer		= ConstantBuffer;
				Data.pointLightBuffer	= pointLightBuffer;
				//Data.lightMap		 = Builder.ReadShaderResource(lightMap);

				Data.Heap.Init(
					Graph.Resources.renderSystem,
					Graph.Resources.renderSystem->Library.RS4CBVs4SRVs.GetDescHeap(0),
					Memory);
				Data.Heap.NullFill(Graph.Resources.renderSystem);

				//if(OcclusionCulling)
				//	Data.OcclusionBuffer = Builder.WriteDepthBuffer	(RS->GetTag(OcclusionBuffer));

				ForwardDrawConstants constants	= { desc.pointLightCount };
				Data.ConstantsOffset			= LoadConstants(constants, ConstantBuffer, Graph.Resources);

				Data.Draws = ForwardDrawableList{ Memory };
				Camera::CameraConstantBuffer CameraConstants = GetCameraConstantBuffer(Camera);
				Data.CameraConstantsOffset = LoadConstants(CameraConstants, ConstantBuffer, Graph.Resources);

				for (auto Viewable : Drawables)
				{
					auto constants = Viewable.D->GetConstants();
					auto CBOffset = LoadConstants(constants, ConstantBuffer, Graph.Resources);
					Data.Draws.push_back({ Viewable.D->MeshHandle, CBOffset });
				}
			},
			[=](ForwardDrawPass& Data, const FrameResources& Resources, Context* Ctx)
			{
				Ctx->SetRootSignature(Resources.renderSystem->Library.RS4CBVs4SRVs);
				Ctx->SetPipelineState(Resources.GetPipelineState(FORWARDDRAW));

				if (false)
				{
					Ctx->SetScissorAndViewports({ Targets.RenderTarget });
					Ctx->SetRenderTargets(
						{	Resources.GetRenderTargetObject(Data.BackBuffer) }, 
						true,
						(DescHeapPOS)Resources.GetRenderTargetObject(Data.OcclusionBuffer));
				}
				else
					Ctx->SetPredicate(false);



				// Setup Initial Shading State
				Ctx->SetScissorAndViewports({Targets.RenderTarget});
				Ctx->SetRenderTargets(
					{	Resources.GetRenderTargetObject(Data.BackBuffer) }, 
					true,
					 (DescHeapPOS)Resources.GetRenderTargetObject(Data.DepthBuffer));

				Ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

				Ctx->SetGraphicsDescriptorTable		(0, Data.Heap);
				Ctx->SetGraphicsConstantBufferView	(1, Data.ConstantBuffer,	Data.CameraConstantsOffset);
				Ctx->SetGraphicsConstantBufferView	(3, Data.ConstantBuffer,	Data.ConstantsOffset);
				Ctx->SetGraphicsConstantBufferView	(4, Data.pointLightBuffer);
				//Ctx->SetGraphicsConstantBufferView	(4, Data.ConstantBuffer, Data.ConstantsOffset);

				for (auto D : Data.Draws)
				{
					auto* TriMesh = GetMeshResource(D.Mesh);

					//if(OcclusionCulling)
					//	Ctx->SetPredicate(true, OcclusionQueries, D.OcclusionIdx * 8);

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


	/************************************************************************************************/


	LightBufferUpdate* WorldRender::updateLightBuffers(
		CameraHandle			camera, 
		GraphicScene&			scene, 
		FrameGraph&				graph, 
		iAllocator*				tempMemory, 
		LighBufferDebugDraw*	drawDebug)
	{
		graph.Resources.AddShaderResource(lightMap);

		const size_t viewSplits		= 30;
		const float  splitSpan		= 1.0f / float(viewSplits);
		const size_t sceneLghtCount = scene.GetPointLightCount();

		Vector<Vector<Vector<LightHandle>>> LightBuckets{ tempMemory };
		LightBuckets.reserve(viewSplits);

		for (size_t itr = 0; itr < viewSplits; itr++) 
		{
			LightBuckets.emplace_back(tempMemory);
			LightBuckets.reserve(viewSplits);
		}

		for (auto& column : LightBuckets) 
		{
			column.reserve(viewSplits);

			for (size_t itr = 0; itr < viewSplits; itr++)
				column.emplace_back(tempMemory);
		}

		auto cameraConstants = GetCameraConstantBuffer(camera);

		auto f		= GetFrustum(camera);
		auto lights = scene.FindPointLights(f, tempMemory);

		auto view			= XMMatrixToFloat4x4(&cameraConstants.View);
		auto perspective	= XMMatrixToFloat4x4(&cameraConstants.Proj);

		for (auto light : lights) 
		{
			auto pos		= scene.GetPointLightPosition(light);
			auto temp1		= view * float4(pos, 1);
			auto screenCord = perspective * temp1;
			float w			= screenCord.w;
			screenCord		= screenCord / w;

			const float r				= 10;//scene.GetPointLightRadius(light);
			const float screenSpaceSize = r / w;

			if (screenCord.x + screenSpaceSize > -1 && screenCord.x - screenSpaceSize < 1 &&
				screenCord.y + screenSpaceSize > -1 && screenCord.y - screenSpaceSize < 1 &&
				screenCord.z > 0 )
			{
				const float x = (screenCord.x + 1) / 2;
				const float y = (1 - screenCord.y) / 2;

				const float TileX = min(x * viewSplits, viewSplits - 1);
				const float TileY = min(y * viewSplits, viewSplits - 1);

				const float tileSpan = 2.0f * min(screenSpaceSize * viewSplits, viewSplits);

				for (int itr_x = 0; itr_x <= tileSpan + 1; ++itr_x) {
					const float offsetX = itr_x - tileSpan / 2.0f;

					for (int itr_y = 0; itr_y <= tileSpan + 1; ++itr_y)
					{
						const float offsetY = itr_y - tileSpan / 2.0f;
						const float idx_x	= TileX + offsetX;
						const float idx_y	= TileY + offsetY;

						if (idx_x < viewSplits && idx_x >= 0 &&
							idx_y < viewSplits && idx_y >= 0)
							LightBuckets[idx_x][idx_y].push_back(light);
					}
				}
			}
		}


		if(drawDebug)
		{
			Vector<FlexKit::Rectangle> rectangles{ tempMemory };
			rectangles.reserve(viewSplits * viewSplits);


			for (size_t columnIdx = 0; columnIdx < viewSplits; columnIdx++)
			{
				for (size_t rowIdx = 0; rowIdx < viewSplits; rowIdx++)
				{
					auto lightCount = LightBuckets[columnIdx][rowIdx].size();

					FlexKit::Rectangle rect;
					auto temp = FlexKit::Grey(float(lightCount) / float(sceneLghtCount));
					rect.Color = float4(temp, 0);
					rect.Position = float2{ float(columnIdx), float(rowIdx) } / viewSplits;
					rect.WH = float2(1, 1) / viewSplits;

					if (lightCount)
						rectangles.push_back(rect);

				}
			}

			if (rectangles.size())
			{
				DrawShapes(
					DRAW_PSO,
					graph,
					drawDebug->vertexBuffer,
					drawDebug->constantBuffer,
					drawDebug->renderTarget,
					tempMemory,
					SolidRectangleListShape{ std::move(rectangles) });
			}
		}

		return &graph.AddNode<LightBufferUpdate>(0,
			[&](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
			{
				data.lightMapObject = builder.WriteShaderResource(lightMap);

				// Build Tile Buffer
				size_t a = 0;
				for (size_t columnIdx = 0; columnIdx < viewSplits; columnIdx++)
					for (size_t rowIdx = 0; rowIdx < viewSplits; rowIdx++)
						a += LightBuckets[columnIdx][rowIdx].size();

				struct pointLightID
				{
					uint16_t idx;
				};

				struct TileMapEntry
				{
					uint16_t lightCount;
					uint16_t offset;
				};


				Vector<pointLightID>	lightIdsBuffer	{ tempMemory };
				Vector<TileMapEntry>	tileMap			{ tempMemory };
				lightIdsBuffer.reserve(a);
				tileMap.reserve(viewSplits * viewSplits);

				uint16_t offset = 0;

				for (size_t columnIdx = 0; columnIdx < viewSplits; columnIdx++) 
				{
					for (size_t rowIdx = 0; rowIdx < viewSplits; rowIdx++)
					{
						auto lightCount = LightBuckets[columnIdx][rowIdx].size();

						for (auto& light : LightBuckets[columnIdx][rowIdx])
							lightIdsBuffer.push_back({ static_cast<uint16_t>(light.INDEX) });

						tileMap.push_back(
							{ 
								static_cast<uint16_t>(lightCount), 
								offset
							});

						offset += lightCount;
					}
				}

				struct pointLight
				{
					float4 KI;	// Color + intensity in W
					float4 P;	//
				};

				Vector<pointLight> pointLights{ tempMemory };
				pointLights.reserve(scene.PLights.size());

				for (auto pointLight : scene.PLights.Lights) {
					pointLights.push_back(
						{
							GetPositionW(pointLight.Position),
							{pointLight.K, pointLight.I}
						});
				}


				BeginNewConstantBuffer(
					lightIDBuffer,
					graph.Resources);


				PushConstantBufferData(
					reinterpret_cast<char*>(lightIdsBuffer.begin()), 
					lightIdsBuffer.size() * sizeof(pointLightID), 
					lightIDBuffer,
					graph.Resources);


				BeginNewConstantBuffer(
					pointLightBuffer,
					graph.Resources);


				PushConstantBufferData(
					reinterpret_cast<char*>(pointLights.begin()),
					pointLights.size() * sizeof(pointLight),
					pointLightBuffer,
					graph.Resources);


				/*
				data.lightMapUpdate = MoveTexture2UploadBuffer(
					reinterpret_cast<char*>(tileMap.begin()),
					tileMap.size() * sizeof(TileMapEntry),
					lightMap,
					graph.Resources);
					*/
			},
			[](LightBufferUpdate& data, const FrameResources& resources, Context* ctx)
			{
				//UploadTexture(data.lightMapUpdate, ctx);
			});


	}


}	/************************************************************************************************/