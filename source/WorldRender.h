#pragma once


#include "buildsettings.h"

#include "AnimationComponents.h"
#include "Components.h"
#include "CoreSceneObjects.h"
#include "ClusteredRendering.h"
#include "DepthBuffer.h"
#include "FrameGraph.h"
#include "graphics.h"
#include "Scene.h"
#include "ShadowMapping.h"
#include "SVOGI.h"
#include "GILightingUtilities.h"

#include <DXProgrammableCapture.h>

namespace FlexKit
{	/************************************************************************************************/


	class TextureStreamingEngine;


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};


	/************************************************************************************************/


	using AdditionalGbufferPass = TypeErasedCallable<void (), 256>;

	struct DrawSceneDescription
	{
		CameraHandle        camera;
		Scene&              scene;
		const double        dt;  // time since last frame
		const double        t;  // running time

		// Resources
		GBuffer&                        gbuffer;

		ReserveVertexBufferFunction     reserveVB;
		ReserveConstantBufferFunction   reserveCB;

		DebugVisMode            debugDisplay    = DebugVisMode::Disabled;
		BVHVisMode              BVHVisMode      = BVHVisMode::Both;
		ClusterDebugDrawMode    debugDrawMode   = ClusterDebugDrawMode::BVH;

		// Inputs
		UpdateTask&         transformDependency;
		UpdateTask&         cameraDependency;
		
		Vector<UpdateTask*>                         sceneDependencies;
		static_vector<AdditionalGbufferPass>        additionalGbufferPasses;
		static_vector<AdditionalShadowMapPass>      additionalShadowPasses;
	};


	struct SceneDescription
	{
		CameraHandle							camera;
		LightGatherTask&						lights;
		UpdateTask&								transforms;
		UpdateTask&								cameras;
		GatherPassesTask&						passes;
		UpdateTaskTyped<GatherSkinnedTaskData>&	skinned;
	};


	/************************************************************************************************/


	constexpr PSOHandle FORWARDDRAW				        = PSOHandle(GetTypeGUID(FORWARDDRAW));

	constexpr PSOHandle ENVIRONMENTPASS                 = PSOHandle(GetTypeGUID(ENVIRONMENTPASS));
	constexpr PSOHandle BILATERALBLURPASSHORIZONTAL     = PSOHandle(GetTypeGUID(BILATERALBLURPASSHORIZONTAL));
	constexpr PSOHandle BILATERALBLURPASSVERTICAL       = PSOHandle(GetTypeGUID(BILATERALBLURPASSVERTICAL));

	constexpr PSOHandle AVERAGELUMINANCE_BLOCK          = PSOHandle(GetTypeGUID(AVERAGELUMINANCE_BLOCK));
	constexpr PSOHandle AVERAGELUMANANCE_GLOBAL         = PSOHandle(GetTypeGUID(AVERAGELUMANANCE_GLOBAL));
	constexpr PSOHandle TONEMAP                         = PSOHandle(GetTypeGUID(TONEMAP));

	constexpr PSOHandle DEPTHPREPASS                    = PSOHandle(GetTypeGUID(DEPTHPREPASS));
	constexpr PSOHandle FORWARDDRAWINSTANCED	        = PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	constexpr PSOHandle FORWARDDRAW_OCCLUDE		        = PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));
	constexpr PSOHandle TEXTURE2CUBEMAP_IRRADIANCE      = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_IRRADIANCE));
	constexpr PSOHandle TEXTURE2CUBEMAP_GGX             = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_GGX));

	constexpr PSOHandle ZPYRAMIDBUILDLEVEL              = PSOHandle(GetTypeGUID(ZPYRAMIDBUILDLEVEL));
	constexpr PSOHandle DEPTHCOPY                       = PSOHandle(GetTypeGUID(DEPTHCOPY));

	
	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardDrawPSO			    (RenderSystem* RS);
	ID3D12PipelineState* CreateForwardDrawInstancedPSO	    (RenderSystem* RS);
	ID3D12PipelineState* CreateOcclusionDrawPSO			    (RenderSystem* RS);
	ID3D12PipelineState* CreateDepthPrePassPSO              (RenderSystem* RS);
	ID3D12PipelineState* CreateEnvironmentPassPSO           (RenderSystem* RS);

	ID3D12PipelineState* CreateTexture2CubeMapIrradiancePSO (RenderSystem* RS);
	ID3D12PipelineState* CreateTexture2CubeMapGGXPSO        (RenderSystem* RS);

	ID3D12PipelineState* CreateBilaterialBlurHorizontalPSO  (RenderSystem* RS);
	ID3D12PipelineState* CreateBilaterialBlurVerticalPSO    (RenderSystem* RS);


	ID3D12PipelineState* CreateBuildZLayer                  (RenderSystem* RS);
	ID3D12PipelineState* CreateDepthBufferCopy              (RenderSystem* RS);


	/************************************************************************************************/


	struct WorldRender_Targets
	{
		ResourceHandle  RenderTarget;
		DepthBuffer&    DepthTarget;
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


	struct DepthPass
	{
		DepthPass(const PVS& IN_brushes) :
			brushes{ IN_brushes } {}

		const PVS&          brushes;
		ResourceHandle      depthPassTarget;
		FrameResourceHandle depthBufferObject;

		CBPushBuffer passConstantsBuffer;
		CBPushBuffer entityConstantsBuffer;
	};

	using PointLightHandleList = Vector<LightHandle>;


	struct ForwardDrawConstants
	{
		float LightCount;
		float t;
		uint2 WH;
	};

	struct ForwardPlusPass
	{
		ForwardPlusPass(
			const PointLightHandleList& IN_lights,
			const PVS&                  IN_PVS,
			const CBPushBuffer&         IN_entityConstants) :
				pointLights     { IN_lights },
				brushes         { IN_PVS    },
				entityConstants { IN_entityConstants } {}


		FrameResourceHandle	BackBuffer;
		FrameResourceHandle	DepthBuffer;
		FrameResourceHandle	OcclusionBuffer;
		FrameResourceHandle	lightMap;
		FrameResourceHandle	lightLists;
		FrameResourceHandle	pointLightBuffer;
		VertexBufferHandle	VertexBuffer;

		CBPushBuffer passConstantsBuffer;
		CBPushBuffer entityConstantsBuffer;

		const CBPushBuffer&         entityConstants;

		const PointLightHandleList& pointLights;
		const PVS&                  brushes;
	};


	/************************************************************************************************/


	struct BackgroundEnvironmentPass
	{
		FrameResourceHandle AlbedoTargetObject;     // RGBA8
		FrameResourceHandle NormalTargetObject;     // RGBA16Float
		FrameResourceHandle MRIATargetObject;
		FrameResourceHandle IOR_ANISOTargetObject;  // RGBA8
		FrameResourceHandle depthBufferTargetObject;

		FrameResourceHandle renderTargetObject;

		VBPushBuffer        passVertices;
		CBPushBuffer        passConstants;

		CameraHandle        camera;
	};


	struct BilateralBlurPass
	{
		ReserveConstantBufferFunction   reserveCB;
		ReserveVertexBufferFunction     reserveVB;

		FrameResourceHandle             DestinationObject;
		FrameResourceHandle             TempObject1;
		FrameResourceHandle             TempObject2;
		FrameResourceHandle             TempObject3;

		FrameResourceHandle             Source;
		FrameResourceHandle             DepthSource;
		FrameResourceHandle             NormalSource;
	};


	/************************************************************************************************/


	struct ToneMap
	{
		FrameResourceHandle     outputTarget;
		FrameResourceHandle     sourceTarget;
		FrameResourceHandle     temp1Buffer;
		FrameResourceHandle     temp2Buffer;
	};


	/************************************************************************************************/

	struct LightCluster
	{
		float4 Min;
		float4 Max;
	};

	struct DEBUG_WorldRenderTimingValues
	{
		float gBufferPass		= 0;
		float shadingPass		= 0;
		float ClusterCreation	= 0;
		float BVHConstruction	= 0;
	};

	struct BrushConstants
	{
		struct EntityConstantOffsets
		{
			static_vector<uint32_t, 16> subMaterial;
		};

		FrameResourceHandle				constants;
		CreateOnceReserveBufferFunction	getConstantBuffer;
		GatherPassesTask&				passes;
		Vector<uint32_t>				entityTable;
		size_t							reservationSize = (size_t)-1;


		CBPushBuffer&					GetConstantBuffer(size_t IN_reservationSize);
		CBPushBuffer&					GetConstantBuffer();
	};

	struct OcclusionCullingResults
	{
		GatherPassesTask&				passes;
		BrushConstants&					entityConstants;
		ReserveConstantBufferFunction	reserveCB;

		FrameResourceHandle				depthBuffer;
		FrameResourceHandle				ZPyramid;
	};

	struct DrawOutputs
	{
		GatherPassesTask&			passes;
		BrushConstants&				entityConstants;
		const ResourceAllocation&	animationResources;
		GatherVisibleLightsTask&	pointLights;
		FrameResourceHandle			visibilityBuffer;
	};


	class ParticleSystemInterface;


	size_t GetRTPoolSize(const AvailableFeatures& features, const uint2 WH = uint2{ 1920, 1080 });

	struct PoolSizes
	{
		size_t UAVPoolByteSize			= 512	* MEGABYTE;
		size_t RTPoolByteSize			= 1024	* MEGABYTE;
		size_t UAVTexturePoolByteSize	= 512	* MEGABYTE;
	};

	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(RenderSystem&, TextureStreamingEngine&, iAllocator* persistent, const PoolSizes& poolSizes = PoolSizes{});
		~WorldRender();

		void HandleTextures();
		void Release();


		DrawOutputs					DrawScene(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				DrawSceneDescription&			drawSceneDesc,
				WorldRender_Targets				targets,
				iAllocator*						persistent,
				ThreadSafeAllocator&			temporary);

		BrushConstants&				BuildBrushConstantsBuffer(
				FrameGraph&						frameGraph,
				UpdateDispatcher&				dispatcher,
				GatherPassesTask&				passes,
				ReserveConstantBufferFunction&	reserveConstants,
				iAllocator&						allocator);

		OcclusionCullingResults&	OcclusionCulling(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				BrushConstants&					entityConstants,
				GatherPassesTask&				passes,
				CameraHandle					camera,
				ReserveConstantBufferFunction&	reserveConstants,
				DepthBuffer&					depthBuffer,
				ThreadSafeAllocator&			temporary);

		DepthPass&					DepthPrePass(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				const CameraHandle				camera,
				GatherPassesTask&				passes,
				const ResourceHandle			depthBufferTarget,
				ReserveConstantBufferFunction	reserveCB,
				iAllocator*						tempAllocator);


		BackgroundEnvironmentPass& BackgroundPass(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				const CameraHandle				camera,
				const ResourceHandle			renderTarget,
				const ResourceHandle			hdrMap,
				ReserveConstantBufferFunction	reserveCB,
				ReserveVertexBufferFunction		reserveVB,
				iAllocator*						tempMemory);


		BackgroundEnvironmentPass& RenderPBR_IBL_Deferred(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				const SceneDescription&			sceneDescription,
				const CameraHandle				camera,
				const ResourceHandle			renderTarget,
				const ResourceHandle			depthTarget,
				GBuffer&						gbuffer,
				ReserveConstantBufferFunction	reserveCB,
				ReserveVertexBufferFunction		reserveVB,
				const float						t,
				iAllocator*						tempMemory);


		BilateralBlurPass&  BilateralBlur(
				FrameGraph&						frameGraph,
				const ResourceHandle			source,
				const ResourceHandle			temp1,
				const ResourceHandle			temp2,
				const ResourceHandle			temp3,
				const ResourceHandle			destination,
				GBuffer&						gbuffer,
				const ResourceHandle			depthBuffer,
				ReserveConstantBufferFunction	reserveCB,
				ReserveVertexBufferFunction		reserveVB,
				iAllocator*						tempMemory);


		ToneMap& RenderPBR_ToneMapping(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				FrameResourceHandle				source,
				ResourceHandle					target,
				ReserveConstantBufferFunction	reserveCB,
				ReserveVertexBufferFunction		reserveVB,
				float							t,
				iAllocator*						allocator);


		DEBUG_WorldRenderTimingValues GetTimingValues() const { return timingValues; }


		struct PassData
		{
			GatherPassesTask&				passes;
			ReserveConstantBufferFunction&	reserveCB;
			ReserveVertexBufferFunction&	reserveVB;
		};


		using RenderTask = FlexKit::TypeErasedCallable<void (FrameGraph&, PassData&), 48>;


		void AddMemoryPools(FrameGraph& frameGraph)
		{
			frameGraph.AddMemoryPool(&UAVPool);
			frameGraph.AddMemoryPool(&RTPool);
			frameGraph.AddMemoryPool(&UAVTexturePool);
		}


		void AddTask(RenderTask&& task)
		{
			pendingGPUTasks.emplace_back(task);
		}


		void BuildSceneGI(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			ReserveConstantBufferFunction&	reserveCB,
			iAllocator&						allocator)
		{
			lightingEngine.BuildScene(frameGraph, scene, passes, reserveCB, allocator);
		}

	//private:
		ID3D12PipelineState* CreateAverageLumanceLocal(RenderSystem* rs);
		ID3D12PipelineState* CreateAverageLumanceGlobal(RenderSystem* rs);
		ID3D12PipelineState* CreateToneMapping(RenderSystem* rs);


		RenderSystem&			renderSystem;

		MemoryPoolAllocator		UAVPool;
		MemoryPoolAllocator		RTPool;
		MemoryPoolAllocator		UAVTexturePool;

		MemoryPoolAllocator*	activePools[3] = { nullptr, nullptr, nullptr };

		QueryHandle				timeStats;
		ReadBackResourceHandle	timingReadBack;

		ResourceHandle			clusterBuffer = InvalidHandle;

		ClusteredRender				clusteredRender;
		ShadowMapper				shadowMapping;
		Transparency				transparency;
		GlobalIlluminationEngine	lightingEngine;

		static_vector<RenderTask>	pendingGPUTasks; // Tasks must be completed prior to rendering

		CircularBuffer<ReadBackResourceHandle, 6> readBackBuffers;

		TextureStreamingEngine&		streamingEngine;
		bool						enableOcclusionCulling;

		FlexKit::RootSignature		rootSignatureToneMapping;

		DEBUG_WorldRenderTimingValues timingValues;
	};

	
}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2016-2022 Robert May

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

