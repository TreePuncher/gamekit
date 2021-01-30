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


#ifdef _WIN32
#pragma once
#endif


#ifndef WORLDRENDER_H_INCLUDED
#define WORLDRENDER_H_INCLUDED


#include "buildsettings.h"
#include "Components.h"
#include "FrameGraph.h"
#include "graphics.h"
#include "CoreSceneObjects.h"
#include "AnimationComponents.h"
#include "GraphicScene.h"

#include <DXProgrammableCapture.h>

namespace FlexKit
{	/************************************************************************************************/


	class TextureStreamingEngine;


	/************************************************************************************************/


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};


	/************************************************************************************************/


    class GBuffer;

    struct DrawSceneDescription
    {
        CameraHandle        camera;
        GraphicScene&       scene;
        const double        dt;  // time since last frame
        const double        t;  // running time

        // Resources
        GBuffer&                        gbuffer;
        ReserveVertexBufferFunction     reserveVB;
        ReserveConstantBufferFunction   reserveCB;

        bool                debugDisplay = false;

        // Inputs
        UpdateTask&         transformDependency;
        UpdateTask&         cameraDependency;

        
        Vector<UpdateTask*> sceneDependencies;
    };


	struct SceneDescription
	{
		CameraHandle                            camera;
		PointLightGatherTask&	                lights;
		PointLightShadowGatherTask&	            pointLightMaps;
		UpdateTask&							    transforms;
		UpdateTask&							    cameras;
		UpdateTaskTyped<GetPVSTaskData>&	    PVS;
		UpdateTaskTyped<GatherSkinnedTaskData>&	skinned;
	};


	/************************************************************************************************/


	static const PSOHandle FORWARDDRAW				   = PSOHandle(GetTypeGUID(FORWARDDRAW));
	static const PSOHandle GBUFFERPASS                 = PSOHandle(GetTypeGUID(GBUFFERPASS));
	static const PSOHandle GBUFFERPASS_SKINNED         = PSOHandle(GetTypeGUID(GBUFFERPASS_SKINNED));
	static const PSOHandle SHADINGPASS                 = PSOHandle(GetTypeGUID(SHADINGPASS));
	static const PSOHandle COMPUTETILEDSHADINGPASS     = PSOHandle(GetTypeGUID(COMPUTETILEDSHADINGPASS));
	static const PSOHandle ENVIRONMENTPASS             = PSOHandle(GetTypeGUID(ENVIRONMENTPASS));
	static const PSOHandle BILATERALBLURPASSHORIZONTAL = PSOHandle(GetTypeGUID(BILATERALBLURPASSHORIZONTAL));
	static const PSOHandle BILATERALBLURPASSVERTICAL   = PSOHandle(GetTypeGUID(BILATERALBLURPASSVERTICAL));

    constexpr PSOHandle LIGHTPREPASS		    = PSOHandle(GetTypeGUID(LIGHTPREPASS));

    constexpr PSOHandle CREATECLUSTERS          = PSOHandle(GetTypeGUID(CREATECLUSTERS));
    constexpr PSOHandle CLEARCOUNTERSPSO        = PSOHandle(GetTypeGUID(CLEARCOUNTERSPSO));
    constexpr PSOHandle CREATECLUSTERLIGHTLISTS = PSOHandle(GetTypeGUID(CREATECLUSTERLIGHTLISTS));

    constexpr PSOHandle CREATELIGHTBVH_PHASE1       = PSOHandle(GetTypeGUID(CREATELIGHTPVH1123));
    constexpr PSOHandle CREATELIGHTBVH_PHASE2       = PSOHandle(GetTypeGUID(CREATELIGHTPVH2));
    constexpr PSOHandle LIGHTBVH_DEBUGVIS_PSO       = PSOHandle(GetTypeGUID(LIGHTBVH_DEBUGVIS_PSO));
    constexpr PSOHandle CLUSTER_DEBUGVIS_PSO        = PSOHandle(GetTypeGUID(CLUSTER_DEBUGVIS_PSO));
    constexpr PSOHandle CLUSTER_DEBUGARGSVIS_PSO    = PSOHandle(GetTypeGUID(CLUSTER_DEBUGARGSVIS_PSO));
    constexpr PSOHandle CREATELIGHTLISTARGS_PSO     = PSOHandle(GetTypeGUID(CREATELIGHTLISTARGS_POS));
    constexpr PSOHandle CREATELIGHTDEBUGVIS_PSO     = PSOHandle(GetTypeGUID(CREATELIGHTDEBUGVIS_PSO));


    static const PSOHandle DEPTHPREPASS                = PSOHandle(GetTypeGUID(DEPTHPREPASS));
	static const PSOHandle FORWARDDRAWINSTANCED	       = PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	static const PSOHandle FORWARDDRAW_OCCLUDE		   = PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));
	static const PSOHandle TEXTURE2CUBEMAP_IRRADIANCE  = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_IRRADIANCE));
	static const PSOHandle TEXTURE2CUBEMAP_GGX         = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_GGX));


	static const PSOHandle SHADOWMAPPASS                = PSOHandle(GetTypeGUID(SHADOWMAPPASS));


	/************************************************************************************************/


	ID3D12PipelineState* CreateForwardDrawPSO			    (RenderSystem* RS);
	ID3D12PipelineState* CreateForwardDrawInstancedPSO	    (RenderSystem* RS);
	ID3D12PipelineState* CreateOcclusionDrawPSO			    (RenderSystem* RS);
	ID3D12PipelineState* CreateDepthPrePassPSO              (RenderSystem* RS);
	ID3D12PipelineState* CreateEnvironmentPassPSO           (RenderSystem* RS);

	ID3D12PipelineState* CreateTexture2CubeMapIrradiancePSO (RenderSystem* RS);
	ID3D12PipelineState* CreateTexture2CubeMapGGXPSO        (RenderSystem* RS);

	ID3D12PipelineState* CreateLightPassPSO				    (RenderSystem* RS);
	ID3D12PipelineState* CreateGBufferPassPSO               (RenderSystem* RS);
	ID3D12PipelineState* CreateGBufferSkinnedPassPSO        (RenderSystem* RS);
	ID3D12PipelineState* CreateDeferredShadingPassPSO       (RenderSystem* RS);
	ID3D12PipelineState* CreateComputeTiledDeferredPSO      (RenderSystem* RS);

    ID3D12PipelineState* CreateLight_DEBUGARGSVIS_PSO       (RenderSystem* RS);
    ID3D12PipelineState* CreateLightBVH_PHASE1_PSO          (RenderSystem* RS);
    ID3D12PipelineState* CreateLightBVH_PHASE2_PSO          (RenderSystem* RS);
    ID3D12PipelineState* CreateLightBVH_DEBUGVIS_PSO        (RenderSystem* RS);
    ID3D12PipelineState* CreateLightListArgs_PSO            (RenderSystem* RS);
    ID3D12PipelineState* CreateCluster_DEBUGVIS_PSO         (RenderSystem* RS);
    ID3D12PipelineState* CreateCluster_DEBUGARGSVIS_PSO     (RenderSystem* RS);
    ID3D12PipelineState* CreateClusterLightListsPSO         (RenderSystem* RS);

    ID3D12PipelineState* CreateClustersPSO                  (RenderSystem* RS);
    ID3D12PipelineState* CreateClearClusterCountersPSO      (RenderSystem* RS);

	ID3D12PipelineState* CreateBilaterialBlurHorizontalPSO  (RenderSystem* RS);
	ID3D12PipelineState* CreateBilaterialBlurVerticalPSO    (RenderSystem* RS);

	ID3D12PipelineState* CreateShadowMapPass                (RenderSystem* RS);


    /************************************************************************************************/


	struct WorldRender_Targets
	{
		ResourceHandle RenderTarget;
		ResourceHandle DepthTarget;
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

    struct GPUCluster
    {
        float3 Min;
        float3 Max;
    };

	struct LightBufferUpdate 
	{
		const Vector<PointLightHandle>*	pointLightHandles;

		CameraHandle			        camera;
		ReserveConstantBufferFunction   reserveCB;
        IndirectLayout&                 indirectLayout;

        const size_t counterOffset = 0;

		ResourceHandle	    lightListBuffer;
		FrameResourceHandle	lightListObject;
		FrameResourceHandle	lightBufferObject;
        FrameResourceHandle	lightBVH;
        FrameResourceHandle	lightLookupObject;
        FrameResourceHandle	lightCounterObject;


        FrameResourceHandle	clusterBufferObject;
        FrameResourceHandle	counterObject;
        FrameResourceHandle	depthBufferObject;
        FrameResourceHandle	indexBufferObject;
        FrameResourceHandle argumentBufferObject;
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

	struct DepthPass
	{
		DepthPass(const PVS& IN_drawables) :
			drawables{ IN_drawables } {}

		const PVS&          drawables;
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
				drawables       { IN_PVS    },
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
		const PVS&                  drawables;
	};


	/************************************************************************************************/


	class GBuffer
	{
	public:
		GBuffer(const uint2 WH, RenderSystem& RS_IN) :
			RS              { RS_IN },
			albedo          { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM)) },
			MRIA            { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT)) },
			normal          { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT)) },
			tangent         { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT)) }
		{
			RS.SetDebugName(albedo,     "Albedo");
			RS.SetDebugName(MRIA,       "MRIA");
			RS.SetDebugName(normal,     "Normal");
			RS.SetDebugName(tangent,    "Tangent");
		}

		~GBuffer()
		{
			RS.ReleaseResource(albedo);
			RS.ReleaseResource(MRIA);
			RS.ReleaseResource(normal);
			RS.ReleaseResource(tangent);
		}

		void Resize(const uint2 WH)
		{
			RS.ReleaseResource(albedo);
			RS.ReleaseResource(MRIA);
			RS.ReleaseResource(normal);
			RS.ReleaseResource(tangent);

			albedo  = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM));
			MRIA    = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT));
			normal  = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT));
			tangent = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT));

			RS.SetDebugName(albedo,  "Albedo");
			RS.SetDebugName(MRIA,    "MRIA");
			RS.SetDebugName(normal,  "Normal");
			RS.SetDebugName(tangent, "Tangent");
		}

		ResourceHandle albedo;	 // rgba_UNORM, Albedo + Metal
		ResourceHandle MRIA;	 // rgba_UNORM, Metal + roughness + IOR + ANISO
		ResourceHandle normal;	 // float16_RGBA
		ResourceHandle tangent;	 // float16_RGBA

		RenderSystem& RS;
	};


	void ClearGBuffer(GBuffer& gbuffer, FrameGraph& frameGraph)
	{
		struct GBufferClear
		{
			FrameResourceHandle albedo;
			FrameResourceHandle MRIA;
			FrameResourceHandle normal;
			FrameResourceHandle tangent;
		};

		auto& clear = frameGraph.AddNode<GBufferClear>(
			GBufferClear{},
			[&](FrameGraphNodeBuilder& builder, GBufferClear& data)
			{
				data.albedo             = builder.WriteRenderTarget(gbuffer.albedo);
				data.MRIA               = builder.WriteRenderTarget(gbuffer.MRIA);
				data.normal             = builder.WriteRenderTarget(gbuffer.normal);
				data.tangent            = builder.WriteRenderTarget(gbuffer.tangent);
			},
			[](GBufferClear& data, ResourceHandler& resources, Context& ctx, iAllocator&)
			{
				ctx.ClearRenderTarget(resources.GetResource(data.albedo));
				ctx.ClearRenderTarget(resources.GetResource(data.MRIA));
				ctx.ClearRenderTarget(resources.GetResource(data.normal));
				ctx.ClearRenderTarget(resources.GetResource(data.tangent));
			});
	}


	void AddGBufferResource(GBuffer& gbuffer, FrameGraph& frameGraph)
	{
		frameGraph.Resources.AddResource(gbuffer.albedo, true);
		frameGraph.Resources.AddResource(gbuffer.MRIA, true);
		frameGraph.Resources.AddResource(gbuffer.normal, true);
		frameGraph.Resources.AddResource(gbuffer.tangent, true);
	}


	struct GBufferPass
	{
		GBuffer&                    gbuffer;
		const PVS&                  pvs;
		const PosedDrawableList&    skinned;

		ReserveConstantBufferFunction   reserveCB;

		FrameResourceHandle AlbedoTargetObject;     // RGBA8
		FrameResourceHandle NormalTargetObject;     // RGBA16Float
		FrameResourceHandle MRIATargetObject;
		FrameResourceHandle TangentTargetObject;
		FrameResourceHandle IOR_ANISOTargetObject;  // RGBA8

		FrameResourceHandle depthBufferTargetObject;
	};


	struct BackgroundEnvironmentPass
	{
		FrameResourceHandle AlbedoTargetObject;     // RGBA8
		FrameResourceHandle NormalTargetObject;     // RGBA16Float
		FrameResourceHandle MRIATargetObject;
		FrameResourceHandle TangentTargetObject;
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


	struct ShadowMapPassData
	{
		Vector<TemporaryFrameResourceHandle>    shadowMapTargets;
	};

	struct LocalShadowMapPassData
	{
		ShadowMapPassData&            sharedData;
		const GatherTask&             sceneSource;
		ReserveConstantBufferFunction reserveCB;
		TemporaryFrameResourceHandle  shadowMapTarget;
		PointLightHandle              pointLight;
	};

	struct TiledDeferredShade
	{
		GBuffer&                            gbuffer;
		const PointLightGatherTask&         lights;
		LightBufferUpdate&                  lightPass;
		const PointLightShadowGatherTask&   pointLightShadowMaps;
		ShadowMapPassData&                  shadowMaps;

		Vector<GPUPointLight>		    pointLights;
		const Vector<PointLightHandle>* pointLightHandles;

		CBPushBuffer            passConstants;
		VBPushBuffer            passVertices;

		FrameResourceHandle     AlbedoTargetObject;     // RGBA8
		FrameResourceHandle     NormalTargetObject;     // RGBA16Float
		FrameResourceHandle     MRIATargetObject;
		FrameResourceHandle     TangentTargetObject;
		FrameResourceHandle     depthBufferTargetObject;
        FrameResourceHandle     clusterIndexBufferObject;
        FrameResourceHandle     clusterBufferObject;
        FrameResourceHandle     lightListsObject;


		FrameResourceHandle		pointLightBufferObject;
		
		FrameResourceHandle     renderTargetObject;
	};


	/************************************************************************************************/


	struct ComputeTiledDeferredShadeDesc
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
		FrameResourceHandle tangentObject;
		FrameResourceHandle depthBufferObject;

		FrameResourceHandle lightBitBucketObject;
		FrameResourceHandle lightBuffer;

		FrameResourceHandle renderTargetObject;
		FrameResourceHandle tempBufferObject;

	};


	/************************************************************************************************/


	ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS);


	struct ShadowPassMatrices
	{
		float4x4 ViewI[6];
		float4x4 PV[6];
	};

	ShadowPassMatrices CalculateShadowMapMatrices(const float3 pos, const float r, const float T);


	/************************************************************************************************/


	inline ShadowMapPassData& ShadowMapPass(
		FrameGraph&                     frameGraph,
		const SceneDescription          sceneDesc,
		const uint2                     screenWH,
		ReserveConstantBufferFunction   reserveCB,
		const double                    t,
		iAllocator*                     allocator);


	/************************************************************************************************/


#define SHADOWMAPPING OFF

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


    struct DrawOutputs
    {
        UpdateTaskTyped<GetPVSTaskData>& PVS;
    };

	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(RenderSystem& RS_IN, TextureStreamingEngine& IN_streamingEngine, const uint2 WH, iAllocator* persistent) :
			renderSystem                { RS_IN },
			OcclusionCulling	        { false	},

            UAVPool                     { renderSystem, 64 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVBuffer, persistent },
            UAVTexturePool              { renderSystem, 64 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::UAVTextures, persistent },
            RTPool                      { renderSystem, 64 * MEGABYTE, DefaultBlockSize, DeviceHeapFlags::RenderTarget, persistent },

            timeStats                   { renderSystem.CreateTimeStampQuery(256) },
            timingReadBack              { renderSystem.CreateReadBackBuffer(512) },

			streamingEngine		        { IN_streamingEngine },
            createClusterLightListLayout{ renderSystem.CreateIndirectLayout({ ILE_DispatchCall }, persistent) },
            createDebugDrawLayout       { renderSystem.CreateIndirectLayout({ ILE_DrawCall }, persistent) }
		{
			RS_IN.RegisterPSOLoader(FORWARDDRAW,			    { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,		  });
			RS_IN.RegisterPSOLoader(FORWARDDRAWINSTANCED,	    { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO });

			RS_IN.RegisterPSOLoader(LIGHTPREPASS,			    { &RS_IN.Library.ComputeSignature,  CreateLightPassPSO			  });
			RS_IN.RegisterPSOLoader(DEPTHPREPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateDepthPrePassPSO         });
            RS_IN.RegisterPSOLoader(SHADOWMAPPASS,              { &RS_IN.Library.RS6CBVs4SRVs,      CreateShadowMapPass           });


			RS_IN.RegisterPSOLoader(GBUFFERPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferPassPSO          });
			RS_IN.RegisterPSOLoader(GBUFFERPASS_SKINNED,	    { &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferSkinnedPassPSO   });

			RS_IN.RegisterPSOLoader(SHADINGPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateDeferredShadingPassPSO  });
			RS_IN.RegisterPSOLoader(ENVIRONMENTPASS,            { &RS_IN.Library.RS6CBVs4SRVs,      CreateEnvironmentPassPSO      });
			RS_IN.RegisterPSOLoader(COMPUTETILEDSHADINGPASS,    { &RS_IN.Library.RSDefault,         CreateComputeTiledDeferredPSO });

            RS_IN.RegisterPSOLoader(CREATECLUSTERS,             { &RS_IN.Library.ComputeSignature,  CreateClustersPSO               });
            RS_IN.RegisterPSOLoader(CREATECLUSTERLIGHTLISTS,    { &RS_IN.Library.ComputeSignature,  CreateClusterLightListsPSO      });
            RS_IN.RegisterPSOLoader(CREATELIGHTBVH_PHASE1,      { &RS_IN.Library.ComputeSignature,  CreateLightBVH_PHASE1_PSO       });
            RS_IN.RegisterPSOLoader(CREATELIGHTBVH_PHASE2,      { &RS_IN.Library.ComputeSignature,  CreateLightBVH_PHASE2_PSO       });
            RS_IN.RegisterPSOLoader(CREATELIGHTLISTARGS_PSO,    { &RS_IN.Library.ComputeSignature,  CreateLightListArgs_PSO         });
            RS_IN.RegisterPSOLoader(CLEARCOUNTERSPSO,           { &RS_IN.Library.ComputeSignature,  CreateClearClusterCountersPSO   });
                
			RS_IN.RegisterPSOLoader(BILATERALBLURPASSHORIZONTAL, { &RS_IN.Library.RSDefault, CreateBilaterialBlurHorizontalPSO });
			RS_IN.RegisterPSOLoader(BILATERALBLURPASSVERTICAL,   { &RS_IN.Library.RSDefault, CreateBilaterialBlurVerticalPSO   });

			RS_IN.RegisterPSOLoader(LIGHTBVH_DEBUGVIS_PSO,      { &RS_IN.Library.RSDefault, CreateLightBVH_DEBUGVIS_PSO });
			RS_IN.RegisterPSOLoader(CLUSTER_DEBUGVIS_PSO,       { &RS_IN.Library.RSDefault, CreateCluster_DEBUGVIS_PSO });
			RS_IN.RegisterPSOLoader(CLUSTER_DEBUGARGSVIS_PSO,   { &RS_IN.Library.RSDefault, CreateCluster_DEBUGARGSVIS_PSO });
            RS_IN.RegisterPSOLoader(CREATELIGHTDEBUGVIS_PSO,    { &RS_IN.Library.RSDefault, CreateLight_DEBUGARGSVIS_PSO });

			RS_IN.QueuePSOLoad(GBUFFERPASS);
			RS_IN.QueuePSOLoad(GBUFFERPASS_SKINNED);
			RS_IN.QueuePSOLoad(DEPTHPREPASS);
			RS_IN.QueuePSOLoad(LIGHTPREPASS);
			RS_IN.QueuePSOLoad(FORWARDDRAW);
			RS_IN.QueuePSOLoad(FORWARDDRAWINSTANCED);
			RS_IN.QueuePSOLoad(SHADINGPASS);
			RS_IN.QueuePSOLoad(COMPUTETILEDSHADINGPASS);
			RS_IN.QueuePSOLoad(BILATERALBLURPASSHORIZONTAL);
			RS_IN.QueuePSOLoad(BILATERALBLURPASSVERTICAL);
            RS_IN.QueuePSOLoad(SHADOWMAPPASS);
            RS_IN.QueuePSOLoad(CLEARCOUNTERSPSO);

            RS_IN.QueuePSOLoad(CREATELIGHTBVH_PHASE1);
            RS_IN.QueuePSOLoad(CREATELIGHTBVH_PHASE2);

            RS_IN.SetReadBackEvent(
                timingReadBack,
                [&](ReadBackResourceHandle resource)
                {
                    auto [buffer, bufferSize] = RS_IN.OpenReadBackBuffer(resource);
                    EXITSCOPE(RS_IN.CloseReadBackBuffer(resource));

                    if(buffer){
                        size_t timePoints[64];
                        memcpy(timePoints, (char*)buffer, sizeof(timePoints));

                        UINT64 timeStampFreq;
                        RS_IN.GraphicsQueue->GetTimestampFrequency(&timeStampFreq);

                        float durations[32];

                        for (size_t I = 0; I < 4; I++)
                            durations[I] = float(timePoints[2 * I + 1] - timePoints[2 * I + 0]) / timeStampFreq * 1000;

                        timingValues.gBufferPass        = durations[0];
                        timingValues.ClusterCreation    = durations[1];
                        timingValues.shadingPass        = durations[2];
                        timingValues.BVHConstruction    = durations[3];
                    }
                });
		}


		~WorldRender()
		{
			Release();
		}


		void HandleTextures()
		{

		}

		void Release()
		{
		}


        DrawOutputs& DrawScene(UpdateDispatcher& dispatcher, FrameGraph& frameGraph, DrawSceneDescription& drawSceneDesc, WorldRender_Targets targets, iAllocator* temporary);


		DepthPass& DepthPrePass(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				GatherTask&                     pvs,
				const ResourceHandle            depthBufferTarget,
				ReserveConstantBufferFunction,
				iAllocator*);

		
		BackgroundEnvironmentPass& BackgroundPass(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				const ResourceHandle            renderTarget,
				const ResourceHandle            hdrMap,
				ReserveConstantBufferFunction   reserveCB,
				ReserveVertexBufferFunction     reserveVB,
				iAllocator*                     tempMemory);


		ForwardPlusPass& RenderPBR_ForwardPlus(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const DepthPass&                depthPass,
				const CameraHandle              camera,
				const WorldRender_Targets&      Target,
				const SceneDescription&         desc,
				const float                     t,
				ReserveConstantBufferFunction   reserveCB,
				ResourceHandle                  environmentMap,
				iAllocator*                     allocator);


		LightBufferUpdate& UpdateLightBuffers(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				const GraphicScene&             scene,
				const SceneDescription&         desc,
                ResourceHandle                  depthBuffer,
				ReserveConstantBufferFunction   reserveCB,
				iAllocator*                     tempMemory);

        DEBUGVIS_DrawBVH& DEBUGVIS_DrawLightBVH(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
                ResourceHandle                  renderTarget,
                LightBufferUpdate&              lightBufferUpdate,
				ReserveConstantBufferFunction   reserveCB,
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


		BilateralBlurPass& BilateralBlur(
			FrameGraph&,
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


		GBufferPass&        RenderPBR_GBufferPass(
			UpdateDispatcher&               dispatcher,
			FrameGraph&                     frameGraph,
			const SceneDescription&         sceneDescription,
			const CameraHandle              camera,
			GBuffer&                        gbuffer,
			ResourceHandle                  depthTarget,
			ReserveConstantBufferFunction   reserveCB,
			iAllocator*                     allocator);


		TiledDeferredShade& RenderPBR_DeferredShade(
			UpdateDispatcher&               dispatcher,
			FrameGraph&                     frameGraph,
			const SceneDescription&         sceneDescription,
			PointLightGatherTask&           gather,
			GBuffer&                        gbuffer,
			ResourceHandle                  depthTarget,
			ResourceHandle                  renderTarget,
			ShadowMapPassData&              shadowMaps,
			LightBufferUpdate&              lightPass,
			ReserveConstantBufferFunction   reserveCB,
			ReserveVertexBufferFunction     reserveVB,
			float                           t,
			iAllocator*                     allocator);


		ComputeTiledPass& RenderPBR_ComputeDeferredTiledShade(
			UpdateDispatcher&                       dispatcher,
			FrameGraph&                             frameGraph,
			ReserveConstantBufferFunction&          constantBufferAllocator,
			const ComputeTiledDeferredShadeDesc&    scene);

        DEBUG_WorldRenderTimingValues GetTimingValues() const { return timingValues; }
	private:
		RenderSystem&			renderSystem;

        MemoryPoolAllocator     UAVPool;
        MemoryPoolAllocator     UAVTexturePool;
        MemoryPoolAllocator     RTPool;

        QueryHandle             timeStats;
        ReadBackResourceHandle  timingReadBack;

		TextureStreamingEngine&	streamingEngine;
		bool                    OcclusionCulling;

        FlexKit::IndirectLayout createClusterLightListLayout;
        FlexKit::IndirectLayout createDebugDrawLayout;

        DEBUG_WorldRenderTimingValues timingValues;
	};

	
}	/************************************************************************************************/

#endif
