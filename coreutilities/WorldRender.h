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
#include "Scene.h"

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

    using AdditionalShadowMapPass =
        TypeErasedCallable
        <256, void,
            ReserveConstantBufferFunction&,
            ReserveVertexBufferFunction&,
            ConstantBufferDataSet*,
            ResourceHandle*,
            const size_t,
            const ResourceHandler&,
            Context&,
            iAllocator&>;


    using AdditionalGbufferPass = TypeErasedCallable<256>;


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


    struct DrawSceneDescription
    {
        CameraHandle        camera;
        Scene&       scene;
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

    constexpr PSOHandle LIGHTPREPASS		            = PSOHandle(GetTypeGUID(LIGHTPREPASS));

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

    constexpr PSOHandle CREATEINITIALLUMANANCELEVEL     = PSOHandle(GetTypeGUID(CREATEINITIALLUMANANCELEVEL));
    constexpr PSOHandle CREATELUMANANCELEVEL            = PSOHandle(GetTypeGUID(CREATELUMANANCELEVEL));

    constexpr PSOHandle OITBLEND                        = PSOHandle(GetTypeGUID(OITBLEND));
    constexpr PSOHandle OITDRAW                         = PSOHandle(GetTypeGUID(OITDRAW));
    constexpr PSOHandle OITDRAWANIMATED                 = PSOHandle(GetTypeGUID(OITDRAWANIMATED));

    constexpr PSOHandle DEPTHPREPASS                    = PSOHandle(GetTypeGUID(DEPTHPREPASS));
    constexpr PSOHandle FORWARDDRAWINSTANCED	        = PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
    constexpr PSOHandle FORWARDDRAW_OCCLUDE		        = PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));
    constexpr PSOHandle TEXTURE2CUBEMAP_IRRADIANCE      = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_IRRADIANCE));
    constexpr PSOHandle TEXTURE2CUBEMAP_GGX             = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_GGX));

    constexpr PSOHandle SHADOWMAPPASS                   = PSOHandle(GetTypeGUID(SHADOWMAPPASS));
    constexpr PSOHandle SHADOWMAPANIMATEDPASS           = PSOHandle(GetTypeGUID(SHADOWMAPANIMATEDPASS));
    constexpr PSOHandle DEBUG_DrawBVH                   = PSOHandle(GetTypeGUID(DEBUG_DrawBVH1));

    constexpr PSOHandle ZPYRAMIDBUILDLEVEL              = PSOHandle(GetTypeGUID(ZPYRAMIDBUILDLEVEL));
    constexpr PSOHandle DEPTHCOPY                       = PSOHandle(GetTypeGUID(DEPTHCOPY));

    constexpr PSOHandle VXGI_INITOCTREE                 = PSOHandle(GetTypeGUID(VXGI_INITOCTREE));
    constexpr PSOHandle VXGI_CLEAR                      = PSOHandle(GetTypeGUID(VXGI_CLEAR));

    constexpr PSOHandle VXGI_CLEANUPVOXELVOLUMES        = PSOHandle(GetTypeGUID(VXGI_CLEANUPVOXELVOLUMES));
    constexpr PSOHandle VXGI_SAMPLEINJECTION            = PSOHandle(GetTypeGUID(VXGI_SAMPLEINJECTION));
    constexpr PSOHandle VXGI_DRAWVOLUMEVISUALIZATION    = PSOHandle(GetTypeGUID(VXGI_DRAWVOLUMEVISUALIZATION));
    constexpr PSOHandle VXGI_GATHERARGS1                = PSOHandle(GetTypeGUID(VXGI_GATHERARGS1));
    constexpr PSOHandle VXGI_GATHERARGS2                = PSOHandle(GetTypeGUID(VXGI_GATHERARGS2));
    constexpr PSOHandle VXGI_GATHERSUBDIVISIONREQUESTS  = PSOHandle(GetTypeGUID(VXGI_GATHERSUBDIVISIONREQUESTS));
    constexpr PSOHandle VXGI_PROCESSSUBDREQUESTS        = PSOHandle(GetTypeGUID(VXGI_PROCESSSUBDREQUESTS));


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
    ID3D12PipelineState* CreateResolutionMatch_PSO          (RenderSystem* RS);
    ID3D12PipelineState* CreateClearResolutionMatch_PSO     (RenderSystem* RS);

    ID3D12PipelineState* CreateInitialLumananceLevel        (RenderSystem* rs);
    ID3D12PipelineState* CreateLumananceLevel               (RenderSystem* rs);

    ID3D12PipelineState* CreateClustersPSO                  (RenderSystem* RS);
    ID3D12PipelineState* CreateClusterBufferPSO             (RenderSystem* RS);
    ID3D12PipelineState* CreateClearClusterCountersPSO      (RenderSystem* RS);

	ID3D12PipelineState* CreateBilaterialBlurHorizontalPSO  (RenderSystem* RS);
	ID3D12PipelineState* CreateBilaterialBlurVerticalPSO    (RenderSystem* RS);

	ID3D12PipelineState* CreateShadowMapPass                (RenderSystem* RS);
    ID3D12PipelineState* CreateShadowMapAnimatedPass        (RenderSystem* RS);

    ID3D12PipelineState* CreateOITBlendPSO                  (RenderSystem* RS);
    ID3D12PipelineState* CreateOITDrawPSO                   (RenderSystem* RS);
    ID3D12PipelineState* CreateOITDrawAnimatedPSO           (RenderSystem* RS);

    ID3D12PipelineState* CreateDEBUGBVHVIS                  (RenderSystem* RS);

    ID3D12PipelineState* CreateBuildZLayer                  (RenderSystem* RS);
    ID3D12PipelineState* CreateDepthBufferCopy              (RenderSystem* RS);


    /************************************************************************************************/


    ID3D12PipelineState* CreateVXGI_InitOctree              (RenderSystem* RS);
    ID3D12PipelineState* CreateInjectVoxelSamplesPSO        (RenderSystem* RS);
    ID3D12PipelineState* CreateUpdateVoxelVolumesPSO        (RenderSystem* RS);
    ID3D12PipelineState* CreateUpdateVolumeVisualizationPSO (RenderSystem* RS);
    ID3D12PipelineState* CreateVXGIGatherArgs1PSO           (RenderSystem* RS);
    ID3D12PipelineState* CreateVXGIGatherArgs2PSO           (RenderSystem* RS);
    ID3D12PipelineState* CreateVXGIGatherSubDRequestsPSO    (RenderSystem* RS);
    ID3D12PipelineState* CreateVXGIProcessSubDRequestsPSO   (RenderSystem* RS);


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
		Scene*	        scene;

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


	class GBuffer
	{
	public:
		GBuffer(const uint2 WH, RenderSystem& RS_IN) :
			RS              { RS_IN },
			albedo          { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM)) },
			MRIA            { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT)) },
			normal          { RS_IN.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT)) }
		{
			RS.SetDebugName(albedo,     "Albedo");
			RS.SetDebugName(MRIA,       "MRIA");
			RS.SetDebugName(normal,     "Normal");
		}

		~GBuffer()
		{
			RS.ReleaseResource(albedo);
			RS.ReleaseResource(MRIA);
			RS.ReleaseResource(normal);
		}

		void Resize(const uint2 WH)
		{
			RS.ReleaseResource(albedo);
			RS.ReleaseResource(MRIA);
			RS.ReleaseResource(normal);

			albedo  = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R8G8B8A8_UNORM));
			MRIA    = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT));
			normal  = RS.CreateGPUResource(GPUResourceDesc::RenderTarget(WH, DeviceFormat::R16G16B16A16_FLOAT));

			RS.SetDebugName(albedo,  "Albedo");
			RS.SetDebugName(MRIA,    "MRIA");
			RS.SetDebugName(normal,  "Normal");
		}

		ResourceHandle albedo;	 // rgba_UNORM, Albedo + Metal
		ResourceHandle MRIA;	 // rgba_UNORM, Metal + roughness + IOR + ANISO
		ResourceHandle normal;	 // float16_RGBA

		RenderSystem& RS;
	};


	inline void ClearGBuffer(GBuffer& gbuffer, FrameGraph& frameGraph)
	{
		struct GBufferClear
		{
			FrameResourceHandle albedo;
			FrameResourceHandle MRIA;
			FrameResourceHandle normal;
		};

		auto& clear = frameGraph.AddNode<GBufferClear>(
			GBufferClear{},
			[&](FrameGraphNodeBuilder& builder, GBufferClear& data)
			{
				data.albedo  = builder.RenderTarget(gbuffer.albedo);
				data.MRIA    = builder.RenderTarget(gbuffer.MRIA);
				data.normal  = builder.RenderTarget(gbuffer.normal);
			},
			[](GBufferClear& data, ResourceHandler& resources, Context& ctx, iAllocator&)
			{
                ctx.BeginEvent_DEBUG("Clear GBuffer");

				ctx.ClearRenderTarget(resources.GetResource(data.albedo));
				ctx.ClearRenderTarget(resources.GetResource(data.MRIA));
				ctx.ClearRenderTarget(resources.GetResource(data.normal));

                ctx.EndEvent_DEBUG();
			});
	}


	inline void AddGBufferResource(GBuffer& gbuffer, FrameGraph& frameGraph)
	{
		frameGraph.Resources.AddResource(gbuffer.albedo, true);
		frameGraph.Resources.AddResource(gbuffer.MRIA, true);
		frameGraph.Resources.AddResource(gbuffer.normal, true);
	}


	struct GBufferPass
	{
		GBuffer&                            gbuffer;
        UpdateTaskTyped<GetPVSTaskData>&    pvs;
		const PosedBrushList&               skinned;

		ReserveConstantBufferFunction   reserveCB;

		FrameResourceHandle AlbedoTargetObject;     // RGBA8
		FrameResourceHandle NormalTargetObject;     // RGBA16Float
		FrameResourceHandle MRIATargetObject;
		FrameResourceHandle IOR_ANISOTargetObject;  // RGBA8

		FrameResourceHandle depthBufferTargetObject;
	};


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


	struct ShadowMapPassData
	{
		Vector<TemporaryFrameResourceHandle> shadowMapTargets;
	};

	struct LocalShadowMapPassData
	{
        const Vector<PointLightHandle>&         pointLightShadows;
		ShadowMapPassData&                      sharedData;
		ReserveConstantBufferFunction           reserveCB;
        ReserveVertexBufferFunction             reserveVB;

        static_vector<AdditionalShadowMapPass>  additionalShadowPass;
	};

    struct AcquireShadowMapResources
    {
    };

    using AcquireShadowMapTask = UpdateTaskTyped<AcquireShadowMapResources>;

	struct TiledDeferredShade
	{
		GBuffer&                            gbuffer;
		const PointLightGatherTask&         lights;
		LightBufferUpdate&                  lightPass;
		const PointLightShadowGatherTask&   pointLightShadowMaps;
		ShadowMapPassData&                  shadowMaps;

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


    /************************************************************************************************/


    struct OITPass
	{

		const PointLightGatherTask&         lights;
		const LightBufferUpdate&            lightPass;
		const PointLightShadowGatherTask&   pointLightShadowMaps;
		const ShadowMapPassData&            shadowMaps;
		const Vector<PointLightHandle>&     pointLightHandles;

        ReserveConstantBufferFunction       reserveCB;

        UpdateTaskTyped<GetPVSTaskData>&    PVS;

        CameraHandle            camera;
        FrameResourceHandle     renderTargetObject;
        FrameResourceHandle     accumalatorObject;
        FrameResourceHandle     counterObject;
        FrameResourceHandle     depthTarget;
	};


    /************************************************************************************************/


    struct UpdateVoxelVolume
    {
        ReserveConstantBufferFunction   reserveCB;
        CameraHandle                    camera;

        FrameResourceHandle     primaryVolume;
        FrameResourceHandle     secondaryVolume;
        FrameResourceHandle     depthTarget;
        FrameResourceHandle     voxelBuffer;
        FrameResourceHandle     counters;
        FrameResourceHandle     octree;
        FrameResourceHandle     indirectArgs;
    };


    /************************************************************************************************/


    struct DEBUGVIS_VoxelVolume
    {
        ReserveConstantBufferFunction   reserveCB;
        CameraHandle                    camera;

        FrameResourceHandle     depthTarget;
        FrameResourceHandle     volume;
        FrameResourceHandle     renderTarget;
        FrameResourceHandle     indirectArgs;
        FrameResourceHandle     octree;
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


    AcquireShadowMapTask& AcquireShadowMaps(UpdateDispatcher& dispatcher, RenderSystem& renderSystem, MemoryPoolAllocator&, PointLightUpdate& pointLightUpdate);


    /************************************************************************************************/


	inline ShadowMapPassData& ShadowMapPass(
		FrameGraph&                             frameGraph,
		const SceneDescription                  sceneDesc,
		ReserveConstantBufferFunction           reserveCB,
        ReserveVertexBufferFunction             reserveVB,
        static_vector<AdditionalShadowMapPass>& additional,
		const double                            t,
		iAllocator*                             allocator);


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
        GatherTask&                     sceneGather;

        CBPushBuffer&                   GetConstants()
        {
            const auto reserveSize = sceneGather.GetData().solid.size() * AlignedSize<Brush::VConstantsLayout>();
            return getConstantBuffer(reserveSize);
        }
    };

    struct OcclusionCullingResults
    {
        GatherTask&                     sceneGather;
        EntityConstants&                entityConstants;
        ReserveConstantBufferFunction   reserveCB;

        FrameResourceHandle             depthBuffer;
        FrameResourceHandle             ZPyramid;
    };

    struct DrawOutputs
    {
        GatherTask&                 PVS;
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
                GatherTask&                     pvs,
                ReserveConstantBufferFunction&  reserveConstants,
                iAllocator&                     allocator);


        OcclusionCullingResults& OcclusionCulling(
                UpdateDispatcher&               dispatcher,
                FrameGraph&                     frameGraph,
                EntityConstants&                entityConstants,
                GatherTask&                     pvs,
                CameraHandle                    camera,
                ReserveConstantBufferFunction&  reserveConstants,
                DepthBuffer&                    depthBuffer,
                ThreadSafeAllocator&            temporary);


		DepthPass& DepthPrePass(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
				GatherTask&                     pvs,
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
				const Scene&                    scene,
				const SceneDescription&         desc,
                ResourceHandle                  depthBuffer,
				ReserveConstantBufferFunction   reserveCB,
				iAllocator*                     tempMemory,
                bool                            releaseTemporaries = true);


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


        OITPass& RenderPBR_OITPass(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const SceneDescription&         sceneDescription,
			    const PointLightGatherTask&     gather,
			    ResourceHandle                  depthTarget,
			    FrameResourceHandle             renderTarget,
			    const ShadowMapPassData&        shadowMaps,
			    const LightBufferUpdate&        lightPass,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator);


        ToneMap& RenderPBR_ToneMapping(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
                FrameResourceHandle             source,
			    ResourceHandle                  target,
			    ReserveConstantBufferFunction   reserveCB,
			    ReserveVertexBufferFunction     reserveVB,
			    float                           t,
			    iAllocator*                     allocator);

        void RenderPBR_GI_InitializeOctree(
			    FrameGraph&                     frameGraph);

        UpdateVoxelVolume&        RenderPBR_GI_UpdateVoxelVolumes(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
			    ResourceHandle                  depthTarget,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator);


        DEBUGVIS_VoxelVolume&     DEBUGVIUS_DrawVoxelVolume(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
			    const CameraHandle              camera,
                FrameResourceHandle             renderTarget,
			    ResourceHandle                  depthTarget,
			    ReserveConstantBufferFunction   reserveCB,
			    iAllocator*                     allocator);


        DEBUGVIS_DrawBVH&   DEBUGVIS_DrawLightBVH(
				UpdateDispatcher&               dispatcher,
				FrameGraph&                     frameGraph,
				const CameraHandle              camera,
                ResourceHandle                  renderTarget,
                LightBufferUpdate&              lightBufferUpdate,
				ReserveConstantBufferFunction   reserveCB,
                ClusterDebugDrawMode            mode,
				iAllocator*                     tempMemory);


        void DEBUGVIS_BVH(
			    UpdateDispatcher&               dispatcher,
			    FrameGraph&                     frameGraph,
                SceneBVH&                       bvh,
                CameraHandle                    camera,
			    ResourceHandle                  renderTarget,
			    ReserveConstantBufferFunction   reserveCB,
			    ReserveVertexBufferFunction     reserveVB,
                BVHVisMode                      mode,
			    iAllocator*                     allocator);


        DEBUG_WorldRenderTimingValues GetTimingValues() const { return timingValues; }


	private:


        void CreateClusterBuffer(uint2 WH, CameraHandle camera, ReserveConstantBufferFunction& reserveCB);


		RenderSystem&			renderSystem;

        MemoryPoolAllocator     UAVPool;
        MemoryPoolAllocator     UAVTexturePool;
        MemoryPoolAllocator     RTPool;

        QueryHandle             timeStats;
        ReadBackResourceHandle  timingReadBack;

        ResourceHandle          clusterBuffer = InvalidHandle_t;

        IndirectLayout          dispatch;
        IndirectLayout          draw;

        ResourceHandle          voxelVolumes[2] = { InvalidHandle_t, InvalidHandle_t };
        ResourceHandle          voxelBuffer;
        ResourceHandle          octreeBuffer;
        uint8_t                 primaryVolume = 0;

        using RenderTask = FlexKit::TypeErasedCallable<48, void, FrameGraph&>;

        static_vector<RenderTask> pendingGPUTasks; // Tasks must be completed prior to rendering

        CircularBuffer<ReadBackResourceHandle, 6> readBackBuffers;

		TextureStreamingEngine&	streamingEngine;
		bool                    enableOcclusionCulling;


        DEBUG_WorldRenderTimingValues timingValues;
	};

	
}	/************************************************************************************************/

#endif
