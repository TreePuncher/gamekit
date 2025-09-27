#pragma once


#include "BuildSettings.hpp"

#include "AnimationComponents.hpp"
#include "Components.hpp"
#include "ClusteredRendering.hpp"
#include "DepthBuffer.hpp"
#include "FrameGraph.hpp"
//#include "GILightingUtilities.hpp"
#include "OcclusionCulling.hpp"
#include "RenderSystemInterface.hpp"
#include "Scene.hpp"
#include "ShadowMapping.hpp"
//#include "SVOGI.hpp"


namespace FlexKit
{	/************************************************************************************************/


	class TextureStreamingEngine;


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};


	/************************************************************************************************/


	struct ExtraGBufferPassInputs
	{
		FrameGraph&			frameGraph;
		UpdateDispatcher&	dispatcher;
		GatherPassesTask&	passes;
		GBuffer&			gbuffer;

		ResourceHandle	depthTarget		= InvalidHandle;
		CameraHandle	activeCamera	= InvalidHandle;
	};

	using AdditionalGBufferPass = TypeErasedCallable<void (ExtraGBufferPassInputs&), 256>;

	struct ExtraForwardPassInputs
	{
		FrameGraph&			frameGraph;
		UpdateDispatcher&	dispatcher;
		GatherPassesTask&	passes;

		ResourceHandle	renderTarget	= InvalidHandle;
		ResourceHandle	depthTarget		= InvalidHandle;
		CameraHandle	activeCamera	= InvalidHandle;
	};

	using AdditionalForwardPass = TypeErasedCallable<void (ExtraForwardPassInputs&), 256>;

	struct DrawSceneDescription
	{
		CameraHandle        camera;
		Scene&              scene;
		const double        dt;  // time since last frame
		const double        t;  // running time

		// Resources
		GBuffer&                        gbuffer;

		DebugVisMode            debugDisplay    = DebugVisMode::Disabled;
		BVHVisMode              BVHVisMode      = BVHVisMode::Both;
		ClusterDebugDrawMode    debugDrawMode   = ClusterDebugDrawMode::BVH;

		// Inputs
		UpdateTask&         transformDependency;
		UpdateTask&         cameraDependency;
		
		Vector<UpdateTask*>                 sceneDependencies;
		Vector<AdditionalGBufferPass>       additionalGbufferPasses;
		Vector<AdditionalShadowMapPass>		additionalShadowPasses;
		Vector<AdditionalForwardPass>		additionalForwardPasses;
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


	LoadPipelineStateRes CreateForwardDrawPSO			    (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateForwardDrawInstancedPSO	    (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateOcclusionDrawPSO			    (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateDepthPrePassPSO              (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateEnvironmentPassPSO           (IRenderSystem& RS, iAllocator& allocator);

	LoadPipelineStateRes CreateTexture2CubeMapIrradiancePSO (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateTexture2CubeMapGGXPSO        (IRenderSystem& RS, iAllocator& allocator);

	LoadPipelineStateRes CreateBilaterialBlurHorizontalPSO  (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateBilaterialBlurVerticalPSO    (IRenderSystem& RS, iAllocator& allocator);

	LoadPipelineStateRes CreateBuildZLayer                  (IRenderSystem& RS, iAllocator& allocator);
	LoadPipelineStateRes CreateDepthBufferCopy              (IRenderSystem& RS, iAllocator& allocator);


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
		uint8_t*		constantBuffers[4];
	};


	struct DepthPass
	{
		DepthPass(const BrushDrawList& IN_draws) :
			draws{ IN_draws } {}

		const BrushDrawList&     draws;
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
			const BrushDrawList&             IN_PVS,
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
		const BrushDrawList&				brushes;
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

		FrameGraphNodeHandle				node;
		FrameResourceHandle					constants;
		CreateOnceReserveBufferFunction2	getConstantBuffer;
		GatherPassesTask&					passes;
		Vector<uint32_t>					entityTable;
		size_t								reservationSize = (size_t)-1;


		CBPushBuffer&					GetConstantBuffer(size_t IN_reservationSize);
		CBPushBuffer&					GetConstantBuffer();
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

	struct WorldRenderOptions
	{
		//EGITECHNIQUE GI = EGITECHNIQUE::DISABLE;
	};


	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(IRenderSystem&, TextureStreamingEngine&, iAllocator* persistent, const WorldRenderOptions& options = {}, const PoolSizes& poolSizes = PoolSizes{});
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
				iAllocator&						allocator);

		DepthPass&					DepthPrePass(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				const CameraHandle				camera,
				GatherPassesTask&				passes,
				const ResourceHandle			depthBufferTarget,
				iAllocator*						tempAllocator);

		BackgroundEnvironmentPass& BackgroundPass(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				const CameraHandle				camera,
				const ResourceHandle			renderTarget,
				const ResourceHandle			hdrMap,
				iAllocator*						tempMemory);

		BackgroundEnvironmentPass& RenderPBR_IBL_Deferred(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				const SceneDescription&			sceneDescription,
				const CameraHandle				camera,
				const ResourceHandle			renderTarget,
				const ResourceHandle			depthTarget,
				GBuffer&						gbuffer,
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
				iAllocator*						tempMemory);

		ToneMap& RenderPBR_ToneMapping(
				UpdateDispatcher&				dispatcher,
				FrameGraph&						frameGraph,
				FrameResourceHandle				source,
				ResourceHandle					target,
				float							t,
				iAllocator*						allocator);


		DEBUG_WorldRenderTimingValues GetTimingValues() const { return timingValues; }


		struct PassData
		{
			GatherPassesTask&				passes;
		};


		using RenderTask = FlexKit::TypeErasedCallable<void (FrameGraph&, PassData&), 48>;


		void AddMemoryPools(FrameGraph& frameGraph)
		{
			frameGraph.AddMemoryPool(UAVPool);
			frameGraph.AddMemoryPool(RTPool);
			frameGraph.AddMemoryPool(UAVTexturePool);
		}


		void AddTask(RenderTask&& task)
		{
			pendingGPUTasks.emplace_back(task);
		}


		void BuildSceneGI(
			FrameGraph&						frameGraph,
			Scene&							scene,
			GatherPassesTask&				passes,
			iAllocator&						allocator)
		{
			//lightingEngine.BuildScene(frameGraph, scene, passes, allocator);
		}

		LoadPipelineStateRes CreateAverageLumanceLocal	(IRenderSystem& rs, iAllocator&);
		LoadPipelineStateRes CreateAverageLumanceGlobal	(IRenderSystem& rs, iAllocator&);
		LoadPipelineStateRes CreateToneMapping			(IRenderSystem& rs, iAllocator&);

		bool occlusionCulling = false;

		IRenderSystem&			renderSystem;

		PoolAllocatorInterface*	UAVPool			= nullptr;
		PoolAllocatorInterface*	RTPool			= nullptr;
		PoolAllocatorInterface*	UAVTexturePool	= nullptr;
		PoolAllocatorInterface*	activePools[3]	= { nullptr, nullptr, nullptr };

		QueryHandle				timeStats;
		ReadBackResourceHandle	timingReadBack;

		ResourceHandle			clusterBuffer = InvalidHandle;

		ClusteredRender				clusteredRender;
		ShadowMapper				shadowMapping;
		//Transparency				transparency;
		//GlobalIlluminationEngine	lightingEngine;

		PassHistoryTable			passHistories;

		static_vector<RenderTask>	pendingGPUTasks; // Tasks must be completed prior to rendering

		CircularBuffer<ReadBackResourceHandle, 6> readBackBuffers;

		TextureStreamingEngine&		streamingEngine;

		const IRootSignature*		rootSignatureToneMapping;

		DEBUG_WorldRenderTimingValues timingValues;
	};

	
}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2016-2024 Robert May

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

