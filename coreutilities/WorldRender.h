#pragma once

/**********************************************************************

Copyright (c) 2016-2021 Robert May

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


#include "buildsettings.h"

#include "AnimationComponents.h"
#include "Components.h"
#include "CoreSceneObjects.h"
#include "ClusteredRendering.h"
#include "FrameGraph.h"
#include "graphics.h"
#include "Scene.h"
#include "ShadowMapping.h"
#include "SVOGI.h"

#include <DXProgrammableCapture.h>

namespace FlexKit
{	/************************************************************************************************/


	class TextureStreamingEngine;


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};


	/************************************************************************************************/


    struct DepthBuffer
    {
        DepthBuffer(RenderSystem& IN_renderSystem, uint2 IN_WH, bool useFloatFormat = true) :
            floatingPoint   { useFloatFormat    },
            renderSystem    { IN_renderSystem   },
            WH              { IN_WH             }
        {
            buffers.push_back(renderSystem.CreateDepthBuffer(WH, useFloatFormat, 1));
            buffers.push_back(renderSystem.CreateDepthBuffer(WH, useFloatFormat, 1));
            buffers.push_back(renderSystem.CreateDepthBuffer(WH, useFloatFormat, 1));

            renderSystem.SetDebugName(buffers[0], "DepthBuffer0");
            renderSystem.SetDebugName(buffers[1], "DepthBuffer1");
            renderSystem.SetDebugName(buffers[2], "DepthBuffer2");
        }

        ~DepthBuffer()
        {
            Release();
        }


        DepthBuffer(const DepthBuffer& rhs)     = delete;
        DepthBuffer(const DepthBuffer&& rhs)    = delete;


        DepthBuffer& operator = (const DepthBuffer& rhs)    = delete;
        DepthBuffer& operator = (DepthBuffer&& rhs)         = delete;


        ResourceHandle Get() const noexcept
        {
            return buffers[idx];
        }

        ResourceHandle GetPrevious() const noexcept
        {
            auto previousIdx = (idx + 2) % 3;
            return buffers[previousIdx];
        }

        void Resize(uint2 newWH)
        {
            Release();

            WH = newWH;

            buffers.push_back(renderSystem.CreateDepthBuffer(WH, floatingPoint, 1));
            buffers.push_back(renderSystem.CreateDepthBuffer(WH, floatingPoint, 1));
            buffers.push_back(renderSystem.CreateDepthBuffer(WH, floatingPoint, 1));
        }

        void Release()
        {
            renderSystem.ReleaseResource(buffers.pop_back());
            renderSystem.ReleaseResource(buffers.pop_back());
            renderSystem.ReleaseResource(buffers.pop_back());
        }

        void Increment() noexcept
        {
            idx = (idx + 1) % 3;
        }

        RenderSystem&                       renderSystem;
        uint                                idx = 0;
        bool                                floatingPoint;
        uint2                               WH;
        CircularBuffer<ResourceHandle, 3>   buffers;
    };

    using AdditionalGbufferPass = TypeErasedCallable<256>;

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
		CameraHandle                            camera;
		PointLightGatherTask&	                lights;
		PointLightShadowGatherTask&	            pointLightMaps;
        UpdateTask&	                            shadowMapAcquire;
		UpdateTask&							    transforms;
		UpdateTask&							    cameras;
        GatherPassesTask&	                    passes;
		UpdateTaskTyped<GatherSkinnedTaskData>&	skinned;
	};


	/************************************************************************************************/


	constexpr PSOHandle FORWARDDRAW				        = PSOHandle(GetTypeGUID(FORWARDDRAW));

	constexpr PSOHandle ENVIRONMENTPASS                 = PSOHandle(GetTypeGUID(ENVIRONMENTPASS));
	constexpr PSOHandle BILATERALBLURPASSHORIZONTAL     = PSOHandle(GetTypeGUID(BILATERALBLURPASSHORIZONTAL));
	constexpr PSOHandle BILATERALBLURPASSVERTICAL       = PSOHandle(GetTypeGUID(BILATERALBLURPASSVERTICAL));

    constexpr PSOHandle CREATEINITIALLUMANANCELEVEL     = PSOHandle(GetTypeGUID(CREATEINITIALLUMANANCELEVEL));
    constexpr PSOHandle CREATELUMANANCELEVEL            = PSOHandle(GetTypeGUID(CREATELUMANANCELEVEL));

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

    ID3D12PipelineState* CreateInitialLumananceLevel        (RenderSystem* rs);
    ID3D12PipelineState* CreateLumananceLevel               (RenderSystem* rs);

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

	using PointLightHandleList = Vector<PointLightHandle>;


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
        float gBufferPass       = 0;
        float shadingPass       = 0;
        float ClusterCreation   = 0;
        float BVHConstruction   = 0;
    };

    struct EntityConstants
    {
        CreateOnceReserveBufferFunction getConstantBuffer;
        GatherPassesTask&               passes;

        CBPushBuffer&                   GetConstants()
        {
            const auto reserveSize = passes.GetData().solid.size() * AlignedSize<Brush::VConstantsLayout>();
            return getConstantBuffer(reserveSize);
        }
    };

    struct OcclusionCullingResults
    {
        GatherPassesTask&               passes;
        EntityConstants&                entityConstants;
        ReserveConstantBufferFunction   reserveCB;

        FrameResourceHandle             depthBuffer;
        FrameResourceHandle             ZPyramid;
    };

    struct DrawOutputs
    {
        GatherPassesTask&           passes;
        GatherSkinnedTask&          skinnedDraws;
        EntityConstants&            entityConstants;
        PointLightShadowGatherTask& pointLights;
        FrameResourceHandle         visibilityBuffer;
    };


    class ParticleSystemInterface;


	class FLEXKITAPI WorldRender
	{
	public:
        WorldRender(RenderSystem&, TextureStreamingEngine&, iAllocator* persistent);
		~WorldRender();

        void HandleTextures();
        void Release();


        DrawOutputs DrawScene(
                UpdateDispatcher&               dispatcher,
                FrameGraph&                     frameGraph,
                DrawSceneDescription&           drawSceneDesc,
                WorldRender_Targets             targets,
                iAllocator*                     persistent,
                ThreadSafeAllocator&            temporary);


        EntityConstants& BuildEntityConstantsBuffer(
                FrameGraph&                     frameGraph,
                UpdateDispatcher&               dispatcher,
                GatherPassesTask&               passes,
                ReserveConstantBufferFunction&  reserveConstants,
                iAllocator&                     allocator);


        OcclusionCullingResults& OcclusionCulling(
                UpdateDispatcher&               dispatcher,
                FrameGraph&                     frameGraph,
                EntityConstants&                entityConstants,
                GatherPassesTask&               passes,
                CameraHandle                    camera,
                ReserveConstantBufferFunction&  reserveConstants,
                DepthBuffer&                    depthBuffer,
                ThreadSafeAllocator&            temporary);


		DepthPass& DepthPrePass(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				GatherPassesTask&               passes,
				const ResourceHandle            depthBufferTarget,
				ReserveConstantBufferFunction   reserveCB,
				iAllocator*                     tempAllocator);


		BackgroundEnvironmentPass& BackgroundPass(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				const ResourceHandle            renderTarget,
				const ResourceHandle            hdrMap,
				ReserveConstantBufferFunction   reserveCB,
				ReserveVertexBufferFunction     reserveVB,
				iAllocator*                     tempMemory);

		BackgroundEnvironmentPass& RenderPBR_IBL_Deferred(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const SceneDescription&         sceneDescription,
			    const CameraHandle              camera,
			    const ResourceHandle            renderTarget,
			    const ResourceHandle            depthTarget,
			    GBuffer&                        gbuffer,
			    ReserveConstantBufferFunction   reserveCB,
			    ReserveVertexBufferFunction     reserveVB,
			    const float                     t,
			    iAllocator*                     tempMemory);

		BilateralBlurPass&  BilateralBlur(
			    FrameGraph&                     frameGraph,
			    const ResourceHandle            source,
			    const ResourceHandle            temp1,
			    const ResourceHandle            temp2,
			    const ResourceHandle            temp3,
			    const ResourceHandle            destination,
	            GBuffer&                        gbuffer,
			    const ResourceHandle            depthBuffer,
			    ReserveConstantBufferFunction   reserveCB,
			    ReserveVertexBufferFunction     reserveVB,
			    iAllocator*                     tempMemory);

        ToneMap& RenderPBR_ToneMapping(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
                FrameResourceHandle             source,
			    ResourceHandle                  target,
			    ReserveConstantBufferFunction   reserveCB,
			    ReserveVertexBufferFunction     reserveVB,
			    float                           t,
			    iAllocator*                     allocator);

        DEBUG_WorldRenderTimingValues GetTimingValues() const { return timingValues; }


	private:

		RenderSystem&			renderSystem;

        MemoryPoolAllocator     UAVPool;
        MemoryPoolAllocator     UAVTexturePool;
        MemoryPoolAllocator     RTPool;

        QueryHandle             timeStats;
        ReadBackResourceHandle  timingReadBack;

        ResourceHandle          clusterBuffer = InvalidHandle_t;

        ClusteredRender         clusteredRender;
        ShadowMapper            shadowMapping;
        GILightingEngine        lightingEngine;
        Transparency            transparency;

        using RenderTask = FlexKit::TypeErasedCallable<48, void, FrameGraph&>;

        static_vector<RenderTask>   pendingGPUTasks; // Tasks must be completed prior to rendering

        CircularBuffer<ReadBackResourceHandle, 6> readBackBuffers;

		TextureStreamingEngine&	    streamingEngine;
		bool                        enableOcclusionCulling;


        DEBUG_WorldRenderTimingValues timingValues;
	};

	
}	/************************************************************************************************/
