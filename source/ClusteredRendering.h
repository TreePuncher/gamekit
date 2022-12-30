#pragma once

#include "graphics.h"
#include "FrameGraph.h"
#include "Scene.h"
#include "ShadowMapping.h"

namespace FlexKit
{   /************************************************************************************************/


	struct AnimationPoseUpload;


	/************************************************************************************************/


	class GBuffer
	{
	public:
		GBuffer(const uint2 WH, RenderSystem& RS_IN);

		~GBuffer();

		void Resize(const uint2 WH);

		ResourceHandle albedo;	 // rgba_UNORM, Albedo + Metal
		ResourceHandle MRIA;	 // rgba_UNORM, Metal + roughness + IOR + ANISO
		ResourceHandle normal;	 // float16_RGBA

		RenderSystem& RS;
	};


	void ClearGBuffer(FrameGraph& frameGraph, GBuffer& gbuffer);
	void AddGBufferResource(GBuffer& gbuffer, FrameGraph& frameGraph);


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


	struct GPUPointLight
	{
		float4 KI;
		float4 PositionR;
	};

	struct GPUCluster
	{
		float3 Min;
		float3 Max;
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
		Scene*	        scene;

		TextureBuffer				lightMapBuffer;
		Vector<uint32_t>			lightLists;
		Vector<GPUPointLightLayout>	pointLights;

		StackAllocator	tempMemory;
	};


	/************************************************************************************************/


	struct ComputeClusteredDeferredShadingDesc
	{
		PointLightGatherTask&   pointLightGather;
		GBuffer&                gbuffer;
		ResourceHandle          depthTarget;
		ResourceHandle          renderTarget;

		CameraHandle            activeCamera;
		iAllocator*             allocator;
	};


	/************************************************************************************************/


	struct ComputeTiledPass
	{
		ReserveConstantBufferFunction   constantBufferAllocator;
		PointLightGatherTask&           pointLights;

		uint3               dispatchDims;
		uint2               WH;
		CameraHandle        activeCamera;

		FrameResourceHandle albedoObject;
		FrameResourceHandle MRIAObject;
		FrameResourceHandle normalObject;
		FrameResourceHandle depthBufferObject;

		FrameResourceHandle lightBitBucketObject;
		FrameResourceHandle lightBuffer;

		FrameResourceHandle renderTargetObject;
		FrameResourceHandle tempBufferObject;

	};

	struct GBufferPass
	{
		GBuffer&							gbuffer;
		const GatherPassesTask&				passes;
		const CameraHandle					camera;

		ReserveConstantBufferFunction		reserveCB;

		FrameResourceHandle entityConstants;

		FrameResourceHandle AlbedoTargetObject;		// RGBA8
		FrameResourceHandle NormalTargetObject;		// RGBA16Float
		FrameResourceHandle MRIATargetObject;
		FrameResourceHandle IOR_ANISOTargetObject;	// RGBA8

		FrameResourceHandle depthBufferTargetObject;
	};


	struct LightBufferUpdate 
	{
		~LightBufferUpdate()
		{
			ProfileFunction();
		}

		const Vector<PointLightHandle>&	visableLights;

		CameraHandle			        camera;
		ReserveConstantBufferFunction   reserveCB;
		IndirectLayout&                 indirectLayout;

		const size_t counterOffset = 0;

		FrameResourceHandle	lightLists;

		FrameResourceHandle	lightListBuffer;
		FrameResourceHandle	lightBufferObject;
		FrameResourceHandle	lightBVH;
		FrameResourceHandle	lightLookupObject;
		FrameResourceHandle	lightCounterObject;
		FrameResourceHandle	lightResolutionObject;

		ReadBackResourceHandle  readBackHandle;

		FrameResourceHandle	clusterBufferObject;

		FrameResourceHandle	counterObject;
		FrameResourceHandle	depthBufferObject;
		FrameResourceHandle	indexBufferObject;
		FrameResourceHandle argumentBufferObject;
	};


	struct ClusteredDeferredShading
	{
		GBufferPass&                        gbuffer;
		const PointLightGatherTask&         lights;
		LightBufferUpdate&                  lightPass;
		const PointLightShadowGatherTask&   pointLightShadowMaps;
		//ShadowMapPassData&                  shadowMaps;

		const Vector<PointLightHandle>* pointLightHandles;

		CBPushBuffer            passConstants;
		VBPushBuffer            passVertices;

		FrameResourceHandle     AlbedoTargetObject;     // RGBA8
		FrameResourceHandle     NormalTargetObject;     // RGBA16Float
		FrameResourceHandle     MRIATargetObject;
		FrameResourceHandle     depthBufferTargetObject;
		FrameResourceHandle     lightMapObject;
		FrameResourceHandle     lightListBuffer;
		FrameResourceHandle     lightLists;

		FrameResourceHandle		pointLightBufferObject;
		FrameResourceHandle     renderTargetObject;
	};


	struct DEBUGVIS_DrawBVH
	{
		ReserveConstantBufferFunction   reserveCB;
		LightBufferUpdate&              lightBufferUpdateData;

		FrameResourceHandle	            lightBVH;
		FrameResourceHandle	            clusters;
		FrameResourceHandle             renderTarget;
		FrameResourceHandle	            pointLights;

		FrameResourceHandle             indirectArgs;
		FrameResourceHandle             counterBuffer;

		CameraHandle			        camera;
	};


	/************************************************************************************************/


	constexpr PSOHandle LIGHTPREPASS                    = PSOHandle(GetTypeGUID(LIGHTPREPASS));
	constexpr PSOHandle CREATECLUSTERS                  = PSOHandle(GetTypeGUID(CREATECLUSTERS));
	constexpr PSOHandle CREATECLUSTERBUFFER             = PSOHandle(GetTypeGUID(CREATECLUSTERBUFFER));
	constexpr PSOHandle CLEARCOUNTERSPSO                = PSOHandle(GetTypeGUID(CLEARCOUNTERSPSO));
	constexpr PSOHandle CREATECLUSTERLIGHTLISTS         = PSOHandle(GetTypeGUID(CREATECLUSTERLIGHTLISTS));

	constexpr PSOHandle CREATELIGHTBVH_PHASE1           = PSOHandle(GetTypeGUID(CREATELIGHTPVH1123));
	constexpr PSOHandle CREATELIGHTBVH_PHASE2           = PSOHandle(GetTypeGUID(CREATELIGHTPVH2));
	constexpr PSOHandle LIGHTBVH_DEBUGVIS_PSO           = PSOHandle(GetTypeGUID(LIGHTBVH_DEBUGVIS_PSO));
	constexpr PSOHandle CLUSTER_DEBUGVIS_PSO            = PSOHandle(GetTypeGUID(CLUSTER_DEBUGVIS_PSO));
	constexpr PSOHandle CLUSTER_DEBUGARGSVIS_PSO        = PSOHandle(GetTypeGUID(CLUSTER_DEBUGARGSVIS_PSO));
	constexpr PSOHandle CREATELIGHTLISTARGS_PSO         = PSOHandle(GetTypeGUID(CREATELIGHTLISTARGS_POS));
	constexpr PSOHandle CREATELIGHTDEBUGVIS_PSO         = PSOHandle(GetTypeGUID(CREATELIGHTDEBUGVIS_PSO));
	constexpr PSOHandle RESOLUTIONMATCHSHADOWMAPS       = PSOHandle(GetTypeGUID(RESOLUTIONMATCHSHADOWMAPS));
	constexpr PSOHandle CLEARSHADOWRESOLUTIONBUFFER     = PSOHandle(GetTypeGUID(CLEARSHADOWRESOLUTIONBUFFER));
	constexpr PSOHandle COMPUTETILEDSHADINGPASS         = PSOHandle(GetTypeGUID(COMPUTETILEDSHADINGPASS));

	constexpr PSOHandle GBUFFERPASS                     = PSOHandle(GetTypeGUID(GBUFFERPASS));
	constexpr PSOHandle GBUFFERPASS_SKINNED             = PSOHandle(GetTypeGUID(GBUFFERPASS_SKINNED));
	constexpr PSOHandle SHADINGPASS                     = PSOHandle(GetTypeGUID(SHADINGPASS));
	constexpr PSOHandle SHADINGPASSCOMPUTE              = PSOHandle(GetTypeGUID(SHADINGPASSCOMPUTE));

	constexpr PSOHandle DEBUG_DrawBVH                   = PSOHandle(GetTypeGUID(DEBUG_DrawBVH1));


	/************************************************************************************************/


	enum class ClusterDebugDrawMode
	{
		BVH,
		Lights,
		Clusters
	};

	enum class BVHVisMode
	{
		Both,
		BVH,
		BoundingVolumes
	};

	enum class DebugVisMode
	{
		Disabled,
		ClusterVIS,
		BVHVIS
	};


	constexpr PassHandle GBufferPassID          = PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) };
	constexpr PassHandle GBufferAnimatedPassID  = PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED_ANIMATED) };

	/************************************************************************************************/


	class ClusteredRender
	{
	public:

		ClusteredRender(RenderSystem&, iAllocator& persistent);


		FlexKit::TypeErasedCallable<void (FrameGraph&), 64>
			CreateClusterBuffer(
								RenderSystem&					renderSystem,
								uint2							WH,
								CameraHandle					camera,
								MemoryPoolAllocator&			UAVPool,
								ReserveConstantBufferFunction&	reserveCB);


		GBufferPass& FillGBuffer(
								UpdateDispatcher&				dispatcher,
								FrameGraph&						frameGraph,
								GatherPassesTask&				passes,
								const CameraHandle				camera,
								GBuffer&						gbuffer,
								ResourceHandle					depthTarget,
								FrameResourceHandle				constants,
								ReserveConstantBufferFunction	reserveCB,
								iAllocator*						allocator,
								AnimationPoseUpload*			poses = nullptr);


		LightBufferUpdate& UpdateLightBuffers(
								UpdateDispatcher&					dispatcher,
								FrameGraph&							graph,
								const CameraHandle					camera,
								const Scene&						scene,
								const PointLightShadowGatherTask&	pointLightsShadowMaps,
								ResourceHandle						depthBuffer,
								ReserveConstantBufferFunction		reserveCB,
								iAllocator*							tempMemory,
								bool								releaseTemporaries = true);


		ClusteredDeferredShading& ClusteredShading(
								UpdateDispatcher&				dispatcher,
								FrameGraph&						frameGraph,
								PointLightShadowGatherTask&		pointLightShadowMaps,
								PointLightGatherTask&			pointLightgather,
								GBufferPass&					gbufferPass,
								ResourceHandle					depthTarget,
								ResourceHandle					renderTarget,
								ShadowMapPassData&				shadowMaps,
								LightBufferUpdate&				lightPass,
								ReserveConstantBufferFunction	reserveCB,
								ReserveVertexBufferFunction		reserveVB,
								float							t,
								iAllocator*						allocator);


		void ReleaseFrameResources(
								FrameGraph&						rameGraph,
								LightBufferUpdate&				lightPass,
								ClusteredDeferredShading&		ClusteredDeferredShading);


		DEBUGVIS_DrawBVH& DEBUGVIS_DrawLightBVH(
								UpdateDispatcher&				dispatcher,
								FrameGraph&						frameGraph,
								const CameraHandle				camera,
								ResourceHandle					renderTarget,
								LightBufferUpdate&				lightBufferUpdate,
								ReserveConstantBufferFunction	reserveCB,
								ClusterDebugDrawMode			mode,
								iAllocator*						tempMemory);


		void DEBUGVIS_BVH(
								UpdateDispatcher&				dispatcher,
								FrameGraph&						frameGraph,
								SceneBVH&						bvh,
								CameraHandle					camera,
								ResourceHandle					renderTarget,
								ReserveConstantBufferFunction	reserveCB,
								ReserveVertexBufferFunction		reserveVB,
								BVHVisMode						mode,
								iAllocator*						allocator);


	private:
		ResourceHandle clusterBuffer = InvalidHandle;
		IndirectLayout dispatch;
		IndirectLayout gather;
		IndirectLayout draw;

		static ID3D12PipelineState* CreateLightPassPSO                  (RenderSystem* RS);
		static ID3D12PipelineState* CreateGBufferPassPSO                (RenderSystem* RS);
		static ID3D12PipelineState* CreateGBufferSkinnedPassPSO         (RenderSystem* RS);
		static ID3D12PipelineState* CreateDeferredShadingPassPSO        (RenderSystem* RS);
		static ID3D12PipelineState* CreateDeferredShadingPassComputePSO (RenderSystem* RS);
		static ID3D12PipelineState* CreateComputeTiledDeferredPSO       (RenderSystem* RS);

		static ID3D12PipelineState* CreateLight_DEBUGARGSVIS_PSO    (RenderSystem* RS);
		static ID3D12PipelineState* CreateLightBVH_PHASE1_PSO       (RenderSystem* RS);
		static ID3D12PipelineState* CreateLightBVH_PHASE2_PSO       (RenderSystem* RS);
		static ID3D12PipelineState* CreateLightBVH_DEBUGVIS_PSO     (RenderSystem* RS);
		static ID3D12PipelineState* CreateLightListArgs_PSO         (RenderSystem* RS);
		static ID3D12PipelineState* CreateCluster_DEBUGVIS_PSO      (RenderSystem* RS);
		static ID3D12PipelineState* CreateCluster_DEBUGARGSVIS_PSO  (RenderSystem* RS);
		static ID3D12PipelineState* CreateClusterLightListsPSO      (RenderSystem* RS);
		static ID3D12PipelineState* CreateResolutionMatch_PSO       (RenderSystem* RS);
		static ID3D12PipelineState* CreateClearResolutionMatch_PSO  (RenderSystem* RS);

		static ID3D12PipelineState* CreateClustersPSO               (RenderSystem* RS);
		static ID3D12PipelineState* CreateClusterBufferPSO          (RenderSystem* RS);
		static ID3D12PipelineState* CreateClearClusterCountersPSO   (RenderSystem* RS);
		static ID3D12PipelineState* CreateDEBUGBVHVIS               (RenderSystem* RS);

	};
}


/**********************************************************************

Copyright (c) 2014-2022 Robert May

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
