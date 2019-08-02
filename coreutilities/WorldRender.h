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


#ifdef _WIN32
#pragma once
#endif


#ifndef WORLDRENDER_H_INCLUDED
#define WORLDRENDER_H_INCLUDED


#include "../buildsettings.h"
#include "../coreutilities/Components.h"
#include "../graphicsutilities/FrameGraph.h"
#include "../graphicsutilities/graphics.h"
#include "../graphicsutilities/CoreSceneObjects.h"


namespace FlexKit
{	/************************************************************************************************/


	class FLEXKITAPI ReadBack
	{
	public:
	};


	/************************************************************************************************/


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};

	class FLEXKITAPI StreamingTexture
	{
	public:
		uint16_t GetHighestResidentMip()
		{
			return -1;
		}


		bool isMipLoadinProgress(uint16_t mip)
		{
			return false;
		}


		void QueueMipLoad(ThreadManager& threads)
		{
		}


	private:
		bool							evicted		= true;
		ID3D12Resource*					tiledResource;
		static_vector<char*>			buffers;	// Nulls for unmapped buffers
		uint16_t						highestMappedMip = -1;
		static_vector<ResourceHandle>	backingResources;
	};


	/************************************************************************************************/


	class FLEXKITAPI TiledResource
	{
	public:
		TiledResource(iAllocator* IN_allocator) {}

	};


	/************************************************************************************************/


	constexpr size_t GetMinBlockSize()
	{
		return ct_sqrt(64 * KILOBYTE / sizeof(uint8_t[4])); // assuming RGBA pixel, and a 64 KB texture alignment
	}

	struct TextureCacheDesc
	{
		const size_t textureCacheSize	= MEGABYTE * 16; // Very small for debugging purposes forces resource eviction and prioritisation, should be changed to a reasonable value
		const size_t blockSize			= GetMinBlockSize();
	};


	/************************************************************************************************/


	class TextureStreamingEngine;

	class TextureBlockAllocator
	{
	public:
		TextureBlockAllocator(iAllocator* IN_allocator) : 
			allocator{ IN_allocator } {}

		iAllocator* allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI TextureStreamingEngine
	{
	public:
		TextureStreamingEngine(RenderSystem& IN_renderSystem, iAllocator* IN_allocator, const TextureCacheDesc& desc = {}) : 
			allocator		{ IN_allocator		},
			mappedTextures	{ IN_allocator		},
			renderSystem	{ IN_renderSystem	},
			settings		{ desc				} 
		{
			FK_ASSERT(desc.textureCacheSize % desc.blockSize == 0, "INVALID POOL SIZE! MUST BE MULTIPLE OF BLOCK SIZE!");

			renderSystem.pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(desc.textureCacheSize),
				D3D12_RESOURCE_STATE_COMMON,
				nullptr, IID_PPV_ARGS(&resourcePool));
		}


		UpdateTask& update(UpdateDispatcher& dispatcher)
		{
			struct _update
			{

			};

			return dispatcher.Add<_update>(
				[&](auto& threadBuilder, _update& data)
				{
				},
				[this](auto&) 
				{
				}
			);
		}


		StreamingTexture2DHandle CreateStreamingTexture()
		{
			return InvalidHandle_t;
		}

	private:
		RenderSystem&				renderSystem;
		ID3D12Resource*				resourcePool	= nullptr;
		Vector<StreamingTexture>	mappedTextures;
		Vector<StreamingTexture*>	loadList; // updated every frame
		TextureBlockAllocator		allocator;
		const TextureCacheDesc		settings;
	};


	/************************************************************************************************/

	const PSOHandle FORWARDDRAW				= PSOHandle(GetTypeGUID(FORWARDDRAW));
	const PSOHandle LIGHTPREPASS			= PSOHandle(GetTypeGUID(LIGHTPREPASS));
	const PSOHandle FORWARDDRAWINSTANCED	= PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	const PSOHandle FORWARDDRAW_OCCLUDE		= PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));

	ID3D12PipelineState* CreateForwardDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateForwardDrawInstancedPSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateOcclusionDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateLightPassPSO				(RenderSystem* RS);

	struct WorldRender_Targets
	{
		TextureHandle RenderTarget;
		TextureHandle DepthTarget;
	};


	struct ObjectDrawState
	{
		bool transparent	: 1;
		bool posed			: 1;
	};


	struct ObjectDraw
	{
		TriMeshHandle	mesh;
		TriMeshHandle	occluder;
		ObjectDrawState states;
		byte*			constantBuffers[4];
	};


	/************************************************************************************************/

	struct GPUPointLightLayout
	{
		float4 KI;	// Color + intensity in W
		float4 P;	//
	};

	struct RG16LightMap
	{
		uint16_t offset = 0;
		uint16_t count = 0;
	};


	struct LighBufferCPUUpdate
	{
		struct visableLightEntry
		{
			uint2				pixel;
			PointLightHandle	light;
			uint32_t			ID(uint2 wh) { return pixel[0] + pixel[1] * wh[0]; };
		};

		Vector<visableLightEntry> samples;

		uint2			viewSplits;
		float2			splitSpan;
		size_t			sceneLightCount;
		CameraHandle	camera;
		GraphicScene*	scene;

		TextureBuffer				lightMapBuffer;
		Vector<uint32_t>			lightLists;
		Vector<GPUPointLightLayout>	pointLights;

		StackAllocator	tempMemory;
	};

	struct GPUPointLight
	{
		float4 KI;
		float4 PositionR;
	};


	struct LightBufferUpdate 
	{
		Vector<GPUPointLight>		pointLights;
		Vector<PointLightHandle>*	pointLightHandles;

		CameraHandle				camera;

		TextureHandle			lightMap;
		TextureHandle			lightListBuffer;
		CBPushBuffer			constants;

		FrameResourceHandle		lightMapObject;
		FrameResourceHandle		lightListObject;
		FrameResourceHandle		lightBufferObject;

		UploadSegment			lightBuffer;	// immediate update

		FlexKit::DescriptorHeap	descHeap;
	};


	struct LighBufferDebugDraw
	{
		VertexBufferHandle		vertexBuffer;
		ConstantBufferHandle	constantBuffer;
		TextureHandle			renderTarget;
	};


	struct SceneDescription
	{
		UpdateTaskTyped<PointLightGather>*	lights;
		UpdateTask*							transforms;
		UpdateTask*							cameras;
		UpdateTaskTyped<GetPVSTaskData>*	PVS;
	};


	/************************************************************************************************/


	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(iAllocator* Memory, RenderSystem* RS_IN, TextureStreamingEngine& IN_streamingEngine, const uint2 IN_lightMapWH = { 64, 64 }) :

			RS(RS_IN),
			ConstantBuffer		{ RS->CreateConstantBuffer(64 * MEGABYTE, false)							},
			OcclusionCulling	{ false																		},
			lightLists			{ RS->CreateUAVBufferResource(sizeof(uint32_t) * IN_lightMapWH.Product() * 1024)	},
			pointLightBuffer	{ RS->CreateUAVBufferResource(sizeof(GPUPointLight) * 1024)					},
			lightMap			{ RS->CreateUAVTextureResource(IN_lightMapWH, FORMAT_2D::R32G32_UINT)		},
			streamingEngine		{ IN_streamingEngine														},
			lightMapWH			{ IN_lightMapWH																}
		{
			RS_IN->RegisterPSOLoader(FORWARDDRAW,			{ &RS_IN->Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,			});
			RS_IN->RegisterPSOLoader(FORWARDDRAWINSTANCED,	{ &RS_IN->Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO	});
			RS_IN->RegisterPSOLoader(FORWARDDRAW_OCCLUDE,	{ &RS_IN->Library.RS6CBVs4SRVs,		CreateOcclusionDrawPSO			});
			RS_IN->RegisterPSOLoader(LIGHTPREPASS,			{ &RS_IN->Library.ComputeSignature, CreateLightPassPSO				});

			RS_IN->QueuePSOLoad(FORWARDDRAW);
			RS_IN->QueuePSOLoad(FORWARDDRAWINSTANCED);
		}


		~WorldRender()
		{
			RS->ReleaseCB(ConstantBuffer);
			RS->ReleaseUAV(lightMap);
			RS->ReleaseUAV(lightLists);
			RS->ReleaseUAV(pointLightBuffer);
		}

		
		void DefaultRender						(UpdateDispatcher& dispatcher, PVS& Objects, CameraHandle Camera, WorldRender_Targets& Target, FrameGraph& Graph, SceneDescription& desc, iAllocator* Memory);
		void RenderDrawabledPBR_ForwardPLUS		(UpdateDispatcher& dispatcher, const PVS& Objects, const CameraHandle Camera, const WorldRender_Targets& Target, FrameGraph& Graph, SceneDescription& desc, iAllocator* Memory);


		LightBufferUpdate* updateLightBuffers	(UpdateDispatcher& dispatcher, CameraHandle Camera, GraphicScene& scene, FrameGraph& graph, SceneDescription& desc, iAllocator* tempMemory, LighBufferDebugDraw* drawDebug = nullptr);


		void ClearGBuffer				(FrameGraph& Graph);
		void RenderDrawabledPBR_Main	(PVS& Objects, FrameGraph& Graph);
		void RenderDrawabledPBR_Shadow	(PVS& Objects, FrameGraph& Graph);
		void ShadePBRPass				(FrameGraph& Graph);

	private:
		RenderSystem*			RS;
		ConstantBufferHandle	ConstantBuffer;
		//QueryHandle				OcclusionQueries;
		//TextureHandle			OcclusionBuffer;

		UAVTextureHandle		lightMap;			// GPU
		UAVResourceHandle		lightLists;			// GPU
		UAVResourceHandle		pointLightBuffer;	// GPU

		uint2					WH;					// Output Size
		uint2					lightMapWH;			// Output Size

		TextureStreamingEngine&	streamingEngine;
		bool OcclusionCulling;

		// GBuffer
		//RenderTargetHandle		Albedo;
		//RenderTargetHandle		Color;
		//RenderTargetHandle		Normal;
		//RenderTargetHandle		Depth;
	};


}	/************************************************************************************************/

#endif
