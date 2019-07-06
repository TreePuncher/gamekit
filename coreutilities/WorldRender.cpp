/**********************************************************************

Copyright (c) 2014-2019 Robert May

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


	void WorldRender::DefaultRender(
		UpdateDispatcher&		dispatcher, 
		PVS&					drawables, 
		CameraHandle			camera, 
		WorldRender_Targets&	targets, 
		FrameGraph&				graph, 
		SceneDescription&		scene, 
		iAllocator*				memory)
	{
		RenderDrawabledPBR_ForwardPLUS(dispatcher, drawables, camera, targets, graph, scene, memory);
	}


	/************************************************************************************************/
	

	void WorldRender::RenderDrawabledPBR_ForwardPLUS(
		UpdateDispatcher&		dispatcher,
		PVS&					drawables, 
		CameraHandle			Camera,
		WorldRender_Targets&	Targets,
		FrameGraph&				frameGraph, 
		SceneDescription&		desc,
		iAllocator*				Memory)
	{
		const size_t MaxEntityDrawCount = 10000;

		struct ForwardDraw
		{
			TriMeshHandle	Mesh;
			size_t			ConstantBufferOffset;
			size_t			OcclusionIdx;
		};


		struct ForwardDrawConstants
		{
			float LightCount = desc.pointLightCount;
		};


		typedef Vector<ForwardDraw> ForwardDrawableList;
		struct ForwardDrawPass
		{
			FrameResourceHandle		BackBuffer;
			FrameResourceHandle		DepthBuffer;
			FrameResourceHandle		OcclusionBuffer;
			FrameResourceHandle		lightMap;
			FrameResourceHandle		lightLists;
			VertexBufferHandle		VertexBuffer;

			CBPushBuffer			entityConstants;
			CBPushBuffer			localConstants;

			ConstantBufferHandle	pointLightBuffer;
			TextureHandle			lightListBuffer;
			PVS const*				drawables;
			DescriptorHeap			Heap; // Null Filled
		};


		auto& Pass = frameGraph.AddNode<ForwardDrawPass>(
			ForwardDrawPass{},
			[&](FrameGraphNodeBuilder& builder, ForwardDrawPass& data)
			{
				builder.AddDataDependency(*desc.PVS);

				data.BackBuffer			= builder.WriteRenderTarget	(RS->GetTag(Targets.RenderTarget));
				data.DepthBuffer		= builder.WriteDepthBuffer	(RS->GetTag(Targets.DepthTarget));

				size_t localBufferSize  = std::max(sizeof(Camera::CameraConstantBuffer), sizeof(ForwardDrawConstants));
				data.entityConstants	= std::move(Reserve(ConstantBuffer, sizeof(ForwardDrawConstants), MaxEntityDrawCount, frameGraph.Resources));
				data.localConstants		= std::move(Reserve(ConstantBuffer, localBufferSize, 2, frameGraph.Resources));

				data.pointLightBuffer	= pointLightBuffer;
				data.lightListBuffer	= lightLists;
				data.lightMap			= builder.ReadShaderResource(lightMap);
				data.lightLists			= builder.ReadShaderResource(lightLists);

				data.drawables			= &drawables;

				data.Heap.Init(
					frameGraph.Resources.renderSystem,
					frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
					Memory);

				data.Heap.SetSRV(frameGraph.GetRenderSystem(), 0, lightMap);
				data.Heap.SetStructuredResource(frameGraph.GetRenderSystem(), 1, data.lightListBuffer, sizeof(uint32_t));

				data.Heap.NullFill(frameGraph.Resources.renderSystem);
			},
			[=](ForwardDrawPass& data, const FrameResources& Resources, Context* Ctx)
			{
				Ctx->SetRootSignature(Resources.renderSystem.Library.RS6CBVs4SRVs);
				Ctx->SetPipelineState(Resources.GetPipelineState(FORWARDDRAW));

				size_t localconstants	= data.localConstants.Push(ForwardDrawConstants{ float(desc.pointLightCount) });
				size_t cameraConstants	= data.localConstants.Push(GetCameraConstants(Camera));

				// Setup Initial Shading State
				Ctx->SetScissorAndViewports({Targets.RenderTarget});
				Ctx->SetRenderTargets(
					{ Resources.GetRenderTargetObject(data.BackBuffer) }, 
					true,
					Resources.GetRenderTargetObject(data.DepthBuffer));

				Ctx->SetPrimitiveTopology			(EInputTopology::EIT_TRIANGLE);
				Ctx->SetGraphicsDescriptorTable		(0, data.Heap);
				Ctx->SetGraphicsConstantBufferView	(1, data.localConstants,	cameraConstants);
				Ctx->SetGraphicsConstantBufferView	(3, data.localConstants,	localconstants);
				Ctx->SetGraphicsConstantBufferView	(4, data.pointLightBuffer);
				Ctx->NullGraphicsConstantBufferView	(6);

				TriMesh* prevMesh = nullptr;

				for (auto& drawable : *data.drawables)
				{
					TriMesh* triMesh		= GetMeshResource(drawable.D->MeshHandle);

					if (triMesh != prevMesh)
					{
						prevMesh = triMesh;

						Ctx->AddIndexBuffer(triMesh);
						Ctx->AddVertexBuffers(triMesh,
							{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
								VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
								//VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
							});
					}

					Ctx->SetGraphicsConstantBufferView(2, ConstantBufferDataSet{ drawable.D->GetConstants(), data.entityConstants });
					Ctx->DrawIndexedInstanced(triMesh->IndexCount, 0, 0, 1, 0);
				}
			});
	}


	/************************************************************************************************/


	LightBufferUpdate* WorldRender::updateLightBuffers(
		UpdateDispatcher&		dispatcher,
		CameraHandle			camera,
		GraphicScene&			scene,
		FrameGraph&				graph,
		SceneDescription&		sceneDescription,
		iAllocator*				tempMemory, 
		LighBufferDebugDraw*	drawDebug)
	{
		graph.Resources.AddShaderResource(lightMap);
		graph.Resources.AddShaderResource(lightLists);

		auto& lightBufferUpdate = dispatcher.Add<LighBufferCPUUpdate>(
		[&](auto& builder, LighBufferCPUUpdate& data)
		{
			builder.AddInput(*sceneDescription.transforms);
			builder.AddInput(*sceneDescription.cameras);

			const size_t tempBufferSize = KILOBYTE * 1024;
			const uint2 WH = lightMapWH + uint2( 64 )  - lightMapWH % 64; // add additional padding for upload alignment requirements

			data.tempMemory.Init((byte*)tempMemory->malloc(tempBufferSize), tempBufferSize);
			data.viewSplits			= lightMapWH;
			data.splitSpan			= { 1.0f / float(data.viewSplits[0]), 1.0f / float(data.viewSplits[1]) };
			data.sceneLightCount	= scene.GetPointLightCount();
			data.scene				= &scene;
			data.camera				= camera;

			data.lightMapBuffer		= TextureBuffer(WH, sizeof(uint16_t[2]), tempMemory);
			data.lightLists			= { data.tempMemory, 0		};
			data.pointLights		= { data.tempMemory, scene.GetPointLightCount() };
			data.samples			= { data.tempMemory, 8096	};
		},
		[](LighBufferCPUUpdate& data)
		{
			const auto frustum		= GetFrustum(data.camera);
			const auto lights		= data.scene->FindPointLights(frustum, data.tempMemory);

			const auto cameraConstants	= CameraComponent::GetComponent().GetCameraConstants(data.camera);
			const auto PV				= XMMatrixToFloat4x4(&cameraConstants.PV);
			const auto view				= XMMatrixToFloat4x4(&cameraConstants.View);
			const auto projection		= XMMatrixToFloat4x4(&cameraConstants.Proj);

			for (auto pointLight : data.scene->PLights.Lights)
				data.pointLights.push_back(
					{
						GetPositionW(pointLight.Position),
						{	pointLight.K, pointLight.I / 10.0f	}
					});


			auto lightMapView	= FlexKit::TextureBufferView<RG16LightMap>(data.lightMapBuffer);

			const uint2		viewSplits	= data.viewSplits;
			const float2	stepSize	= { 1.0f / viewSplits[0], 1.0f / viewSplits[1] };

			for (auto light : lights) 
			{
				auto pos		= data.scene->GetPointLightPosition(light);
				auto temp1		= view * float4(pos, 1);
				auto screenCord = projection * temp1;
				const float w	= screenCord.w;
				screenCord		= screenCord / w;

				const float r				 = data.scene->GetPointLightRadius(light);
				const float screenSpaceSizeX = abs(r / w) * (float(viewSplits[0]) / float(viewSplits[1]));
				const float screenSpaceSizeY = screenSpaceSizeX;

				const float x = (screenCord.x + 1);
				const float y = (1 - screenCord.y);

				const int32_t TileX = x * viewSplits[0] / 2;
				const int32_t TileY = y * viewSplits[1] / 2;

				const uint32_t XBegin = max(min(TileX - screenSpaceSizeX - 1, viewSplits[0]), 0);
				const uint32_t YBegin = max(min(TileY - screenSpaceSizeY - 1, viewSplits[1]), 0);

				const uint32_t XEnd = max(min(TileX + screenSpaceSizeX + 1, viewSplits[0]), 0);
				const uint32_t YEnd = max(min(TileY + screenSpaceSizeY + 1, viewSplits[1]), 0);

				for (uint32_t Yitr = YBegin; Yitr < YEnd; Yitr++)
					for (uint32_t Xitr = XBegin; Xitr < XEnd; Xitr++)
						data.samples.push_back({ { Xitr, Yitr }, light });
			}

			std::sort(data.samples.begin(), data.samples.end(), [&](auto& lhs, auto& rhs) { return lhs.ID(viewSplits) < rhs.ID(viewSplits); });

			size_t	offset		= 0;
			size_t	prevPixelID	= -1;

			for (auto sample : data.samples)
			{
				data.lightLists.push_back(sample.light);
				lightMapView[sample.pixel].count++;

				if(prevPixelID != sample.ID(viewSplits))
					lightMapView[sample.pixel].offset = offset;

				prevPixelID = sample.ID(viewSplits);
				offset++;
			}
		});


		struct Clear
		{
			TextureBuffer* lightMapBuffer = nullptr;
		};

		auto ClearTask = &dispatcher.Add<Clear>(
			[&](auto& builder, Clear& data)
			{
				builder.AddOutput(lightBufferUpdate);
				LighBufferCPUUpdate& lightBufferUpdate_ref = *(LighBufferCPUUpdate*)lightBufferUpdate.Data;
				data.lightMapBuffer = &lightBufferUpdate_ref.lightMapBuffer;
			},
			[](Clear& data)
			{
				memset(data.lightMapBuffer->Buffer, 0, data.lightMapBuffer->BufferSize());
			});
		

		auto lightBufferData = &graph.AddNode<LightBufferUpdate>(
		LightBufferUpdate{},
		[&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
		{
			builder.AddDataDependency(lightBufferUpdate);
			auto& lightBufferData	= *reinterpret_cast<LighBufferCPUUpdate*>(lightBufferUpdate.Data);

			data.lightMap			= lightMap;
			data.lightMapObject		= builder.WriteShaderResource(lightMap);
			data.lightListObject	= builder.WriteShaderResource(lightLists);
			data.lighBufferData		= &lightBufferData;
			data.pointLightBuffer	= pointLightBuffer;
			data.lightListBuffer	= lightLists;
			data.lightMapUpdate		= ReserveUploadBuffer(data.lighBufferData->lightMapBuffer.Size, graph.GetRenderSystem());
			data.lightListUpdate	= ReserveUploadBuffer(1024 * 4 * sizeof(uint32_t), graph.GetRenderSystem());

			BeginNewConstantBuffer(
				data.pointLightBuffer,
				graph.Resources);
		},
		[](LightBufferUpdate& data, FrameResources& resources, Context* ctx)
		{
			auto& lightBufferData	= *data.lighBufferData;
			auto& lightMapBuffer	= lightBufferData.lightMapBuffer;
			auto& lightListBuffer	= lightBufferData.lightLists;
			auto& pointLights		= lightBufferData.pointLights;

			PushConstantBufferData(
				reinterpret_cast<char*>(pointLights.begin()),
				pointLights.size() * sizeof(GPUPointLightLayout),
				data.pointLightBuffer,
				resources);

			MoveTexture2UploadBuffer(data.lightMapUpdate,	lightMapBuffer, lightMapBuffer.BufferSize());
			MoveTexture2UploadBuffer(data.lightListUpdate,	(byte*)lightListBuffer.begin(), lightListBuffer.size() * sizeof(uint32_t));

			ctx->CopyTexture2D	(data.lightMapUpdate,	data.lightMap,			lightMapBuffer.WH);
			ctx->CopyBuffer		(data.lightListUpdate,	data.lightListBuffer);
		});


#if 0
		if(drawDebug)
		{
			const size_t viewSplits = 16;
			Vector<FlexKit::Rectangle> rectangles{ tempMemory };
			rectangles.reserve(viewSplits * viewSplits);


			for (size_t columnIdx = 0; columnIdx < viewSplits; columnIdx++)
			{
				for (size_t rowIdx = 0; rowIdx < viewSplits; rowIdx++)
				{
					auto lightCount = LightBuckets[columnIdx][rowIdx].size();

					FlexKit::Rectangle rect;
					auto temp		= FlexKit::Grey(float(lightCount) / float(sceneLghtCount));
					rect.Color		= float4(temp, 0.25f);
					rect.Position	= float2{ float(columnIdx), float(rowIdx) } / viewSplits;
					rect.WH			= float2(1, 1) / viewSplits;

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

			return nullptr;
		}
#endif

		return lightBufferData;
	}


}	/************************************************************************************************/