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


	ID3D12PipelineState* CreateLightPassPSO(RenderSystem* RS)
	{
		auto lightPassShader = LoadShader("tiledLightCulling", "tiledLightCulling", "cs_5_0", "assets\\lightPass.hlsl");

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
		PSO_desc.CS				= lightPassShader;
		PSO_desc.pRootSignature = RS->Library.ComputeSignature;

		ID3D12PipelineState* PSO = nullptr;
		auto HR = RS->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
		FK_ASSERT(SUCCEEDED(HR));

		return PSO;
	}


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
		auto PShader = LoadShader("FlatWhite",	"FlatWhite",			"ps_5_0", "assets\\forwardRender.hlsl");

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
		UpdateDispatcher&		    dispatcher, 
		FrameGraph&				    graph, 
		const PVS&					drawables, 
		const CameraHandle			camera, 
		const WorldRender_Targets&	targets, 
		const SceneDescription&		scene, 
		iAllocator*				    memory
    )
	{
		RenderDrawabledPBR_ForwardPLUS(dispatcher, graph, drawables, camera, targets, scene, memory);
	}


	/************************************************************************************************/
	

	void WorldRender::RenderDrawabledPBR_ForwardPLUS(
		UpdateDispatcher&			dispatcher,
		FrameGraph&					frameGraph, 
		const PVS&					drawables, 
		const CameraHandle			Camera,
		const WorldRender_Targets&	Targets,
		const SceneDescription&	    desc,
		iAllocator*					allocator)
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
			float LightCount;
		};

		typedef Vector<ForwardDraw> ForwardDrawableList;
		struct ForwardDrawPass
		{
			FrameResourceHandle			BackBuffer;
			FrameResourceHandle			DepthBuffer;
			FrameResourceHandle			OcclusionBuffer;
			FrameResourceHandle			lightMap;
			FrameResourceHandle			lightLists;
			FrameResourceHandle			pointLightBuffer;
			VertexBufferHandle			VertexBuffer;

			CBPushBuffer				entityConstants;
			CBPushBuffer				localConstants;

			const Vector<PointLightHandle>*	pointLights;

			PVS const*					drawables;
			DescriptorHeap				Heap; // Null Filled
		};

		auto& ShadingPass = frameGraph.AddNode<ForwardDrawPass>(
			ForwardDrawPass{},
			[&](FrameGraphNodeBuilder& builder, ForwardDrawPass& data)
			{
				builder.AddDataDependency(desc.PVS);

				data.pointLights		= &desc.lights.GetData().pointLights;
				data.BackBuffer			= builder.WriteRenderTarget	(Targets.RenderTarget);
				data.DepthBuffer		= builder.WriteDepthBuffer	(Targets.DepthTarget);
				size_t localBufferSize  = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
				data.entityConstants	= std::move(Reserve(ConstantBuffer, sizeof(ForwardDrawConstants), MaxEntityDrawCount, frameGraph.Resources));
				data.localConstants		= std::move(Reserve(ConstantBuffer, localBufferSize, 2, frameGraph.Resources));

				data.lightMap			= builder.ReadWriteUAV(lightMap,			DRS_ShaderResource);
				data.lightLists			= builder.ReadWriteUAV(lightLists,			DRS_ShaderResource);
				data.pointLightBuffer	= builder.ReadWriteUAV(pointLightBuffer,	DRS_ShaderResource);

				data.drawables			= &drawables;

				data.Heap.Init(
					frameGraph.Resources.renderSystem,
					frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    allocator);

				data.Heap.NullFill(frameGraph.Resources.renderSystem);
			},
			[=](ForwardDrawPass& data, const FrameResources& resources, Context* Ctx)
			{
				Ctx->SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
				Ctx->SetPipelineState(resources.GetPipelineState(FORWARDDRAW));
				
				size_t localconstants	= data.localConstants.Push(ForwardDrawConstants{ float(data.pointLights->size()) });
				size_t cameraConstants	= data.localConstants.Push(GetCameraConstants(Camera));

				// Setup Initial Shading State
				Ctx->SetScissorAndViewports({Targets.RenderTarget});
				Ctx->SetRenderTargets(
					{ resources.GetRenderTargetObject(data.BackBuffer) },
					true,
					resources.GetRenderTargetObject(data.DepthBuffer));


				data.Heap.SetSRV(resources.renderSystem, 0, lightMap);
				data.Heap.SetSRV(resources.renderSystem, 1, lightLists);
				data.Heap.SetSRV(resources.renderSystem, 2, pointLightBuffer);

				Ctx->SetPrimitiveTopology			(EInputTopology::EIT_TRIANGLE);
				Ctx->SetGraphicsDescriptorTable		(0, data.Heap);
				Ctx->SetGraphicsConstantBufferView	(1, data.localConstants, cameraConstants);
				Ctx->SetGraphicsConstantBufferView	(3, data.localConstants, localconstants);
				Ctx->NullGraphicsConstantBufferView	(6);

				TriMesh* prevMesh = nullptr;

				for (auto& drawable : *data.drawables)
				{
					TriMesh* triMesh = GetMeshResource(drawable.D->MeshHandle);

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
		FrameGraph&				graph,
		const CameraHandle	    camera,
		const GraphicScene&	    scene,
		const SceneDescription& sceneDescription,
		iAllocator*				tempMemory, 
		LighBufferDebugDraw*	drawDebug)
	{
		graph.Resources.AddUAVResource(lightMap,			0, graph.GetRenderSystem().GetObjectState(lightMap));
		graph.Resources.AddUAVResource(lightLists,			0, graph.GetRenderSystem().GetObjectState(lightLists));
		graph.Resources.AddUAVResource(pointLightBuffer,	0, graph.GetRenderSystem().GetObjectState(pointLightBuffer));

		auto lightBufferData = &graph.AddNode<LightBufferUpdate>(
		    LightBufferUpdate{
                    Vector<GPUPointLight>(tempMemory, 1024),
                    &sceneDescription.lights.GetData().pointLights,
                    ReserveUploadBuffer(1024 * sizeof(GPUPointLight), graph.GetRenderSystem()),// max point light count of 1024
                    camera,
                    CBPushBuffer(ConstantBuffer, 2 * KILOBYTE, graph.GetRenderSystem()),
            },
		    [&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
		    {
			    auto& renderSystem      = graph.GetRenderSystem();
			    data.lightMapObject		= builder.ReadWriteUAV(lightMap,		 DRS_UAV);
			    data.lightListObject	= builder.ReadWriteUAV(lightLists,		 DRS_UAV);
			    data.lightBufferObject	= builder.ReadWriteUAV(pointLightBuffer, DRS_Write);
			    data.camera				= camera;

			    data.descHeap.Init(renderSystem, renderSystem.Library.ComputeSignature.GetDescHeap(0), tempMemory);
			    data.descHeap.NullFill(renderSystem);

			    builder.AddDataDependency(sceneDescription.lights);
			    builder.AddDataDependency(sceneDescription.cameras);
		    },
		    [XY = lightMapWH](LightBufferUpdate& data, FrameResources& resources, Context* ctx)
		    {
			    const auto cameraConstants = GetCameraConstants(data.camera);

			    struct ConstantsLayout
			    {
				    float4x4 iproj;
				    float4x4 view;
				    uint2    LightMapWidthHeight[2];
				    uint32_t lightCount;
			    }constantsValues = {
			        XMMatrixToFloat4x4(DirectX::XMMatrixInverse(nullptr, cameraConstants.Proj)),
			        XMMatrixToFloat4x4(cameraConstants.View),
                    XY,
			        (uint32_t)data.pointLightHandles->size()
                };

			    ConstantBufferDataSet constants{ constantsValues, data.constants };
			    PointLightComponent& pointLights = PointLightComponent::GetComponent();

			    for (const auto light : *data.pointLightHandles)
			    {
				    PointLight pointLight	= pointLights[light];
				    float3 position			= GetPositionW(pointLight.Position);

				    data.pointLights.push_back(
					    {	{ pointLight.K, pointLight.I	},
						    { position, pointLight.R		} });
			    }

			    const size_t uploadSize = data.pointLights.size() * sizeof(GPUPointLight);
			    MoveBuffer2UploadBuffer(data.lightBuffer, (byte*)data.pointLights.begin(), uploadSize);
			    ctx->CopyBuffer(data.lightBuffer, uploadSize, resources.WriteUAV(data.lightBufferObject, ctx));

			    ctx->SetPipelineState(resources.GetPipelineState(LIGHTPREPASS));
			    ctx->SetComputeRootSignature(resources.renderSystem.Library.ComputeSignature);
			    ctx->SetComputeDescriptorTable(0, data.descHeap);

			    data.descHeap.SetUAV(resources.renderSystem, 0, resources.GetUAVTextureResource	(data.lightMapObject));
			    data.descHeap.SetUAV(resources.renderSystem, 1, resources.GetUAVBufferResource	(data.lightListObject));
			    data.descHeap.SetSRV(resources.renderSystem, 2, resources.ReadUAVBuffer			(data.lightBufferObject, DRS_ShaderResource, ctx));
			    data.descHeap.SetCBV(resources.renderSystem, 6, constants, constants.offset(), sizeof(ConstantsLayout));

			    ctx->Dispatch({ XY[0], XY[1], 1 });
		    });

		return lightBufferData;
	}


}	/************************************************************************************************/
