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


#include "buildsettings.h"
#include "Components.h"
#include "FrameGraph.h"
#include "graphics.h"
#include "CoreSceneObjects.h"
#include "AnimationComponents.h"
#include "GraphicScene.h"

#include <d3dx12.h>

namespace FlexKit
{	/************************************************************************************************/


    class TextureStreamingEngine;


    /************************************************************************************************/


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};


    /************************************************************************************************/


    struct SceneDescription
	{
        CameraHandle                            camera;
		UpdateTaskTyped<PointLightGather>&	    lights;
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
	static const PSOHandle LIGHTPREPASS			       = PSOHandle(GetTypeGUID(LIGHTPREPASS));
	static const PSOHandle DEPTHPREPASS                = PSOHandle(GetTypeGUID(DEPTHPREPASS));
	static const PSOHandle FORWARDDRAWINSTANCED	       = PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	static const PSOHandle FORWARDDRAW_OCCLUDE		   = PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));
    static const PSOHandle TEXTURE2CUBEMAP_IRRADIANCE  = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_IRRADIANCE));
    static const PSOHandle TEXTURE2CUBEMAP_GGX         = PSOHandle(GetTypeGUID(TEXTURE2CUBEMAP_GGX));


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

    ID3D12PipelineState* CreateBilaterialBlurHorizontalPSO  (RenderSystem* RS);
    ID3D12PipelineState* CreateBilaterialBlurVerticalPSO    (RenderSystem* RS);


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


	struct LightBufferUpdate 
	{
		Vector<GPUPointLight>		    pointLights;
		const Vector<PointLightHandle>*	pointLightHandles;
        UploadSegment			        lightBuffer;	// immediate update

		CameraHandle			camera;

		CBPushBuffer			constants;
		ResourceHandle			lightListBuffer;

		FrameResourceHandle		lightListObject;
		FrameResourceHandle		lightBufferObject;
	};


	struct LighBufferDebugDraw
	{
		VertexBufferHandle		vertexBuffer;
		ConstantBufferHandle	constantBuffer;
		ResourceHandle			renderTarget;
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


        FrameResourceHandle			BackBuffer;
        FrameResourceHandle			DepthBuffer;
        FrameResourceHandle			OcclusionBuffer;
        FrameResourceHandle			lightMap;
        FrameResourceHandle			lightLists;
        FrameResourceHandle			pointLightBuffer;
        VertexBufferHandle			VertexBuffer;

        CBPushBuffer passConstantsBuffer;
        CBPushBuffer entityConstantsBuffer;

        uint2       WH;

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
            RS.ReleaseTexture(albedo);
            RS.ReleaseTexture(MRIA);
            RS.ReleaseTexture(normal);
            RS.ReleaseTexture(tangent);
        }

        void Resize(const uint2 WH)
        {
            RS.ReleaseTexture(albedo);
            RS.ReleaseTexture(MRIA);
            RS.ReleaseTexture(normal);
            RS.ReleaseTexture(tangent);

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
            [](GBufferClear& data, FrameResources& resources, Context& ctx, iAllocator&)
            {
                ctx.ClearRenderTarget(resources.GetTexture(data.albedo));
                ctx.ClearRenderTarget(resources.GetTexture(data.MRIA));
                ctx.ClearRenderTarget(resources.GetTexture(data.normal));
                ctx.ClearRenderTarget(resources.GetTexture(data.tangent));
            });
    }


    void AddGBufferResource(GBuffer& gbuffer, FrameGraph& frameGraph)
    {
        frameGraph.Resources.AddShaderResource(gbuffer.albedo, true);
        frameGraph.Resources.AddShaderResource(gbuffer.MRIA, true);
        frameGraph.Resources.AddShaderResource(gbuffer.normal, true);
        frameGraph.Resources.AddShaderResource(gbuffer.tangent, true);
    }


    struct GBufferPass
    {
        GBuffer&                    gbuffer;
        const PVS&                  pvs;
        const PosedDrawableList&    skinned;

        ReserveConstantBufferFunction reserveCB;

        FrameResourceHandle AlbedoTargetObject;     // RGBA8
        FrameResourceHandle NormalTargetObject;     // RGBA16Float
        FrameResourceHandle MRIATargetObject;
        FrameResourceHandle TangentTargetObject;
        FrameResourceHandle IOR_ANISOTargetObject;  // RGBA8

        FrameResourceHandle depthBufferTargetObject;
    };


    struct BackgroundEnvironmentPass
    {
        ResourceHandle      diffuseMap;
        ResourceHandle      GGX;

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


    struct TiledDeferredShade
    {
        GBuffer&                gbuffer;
        PointLightGatherTask&   lights;
        UploadSegment			lightBuffer;	// immediate update

        Vector<GPUPointLight>		    pointLights;
        const Vector<PointLightHandle>* pointLightHandles;

        CBPushBuffer            passConstants;
        VBPushBuffer            passVertices;

        FrameResourceHandle     AlbedoTargetObject;     // RGBA8
        FrameResourceHandle     NormalTargetObject;     // RGBA16Float
        FrameResourceHandle     MRIATargetObject;
        FrameResourceHandle     TangentTargetObject;
        FrameResourceHandle     depthBufferTargetObject;
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

        uint3                       dispatchDims;
        uint2                       WH;
        CameraHandle                activeCamera;

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


	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(iAllocator* Memory, RenderSystem& RS_IN, TextureStreamingEngine& IN_streamingEngine, const uint2 WH) :
			renderSystem                { RS_IN                                                                                     },
			OcclusionCulling	        { false																                        },
			lightLists			        { renderSystem.CreateUAVBufferResource(sizeof(uint32_t) * (WH / 10).Product() * 32)         },
			pointLightBuffer	        { renderSystem.CreateUAVBufferResource(sizeof(GPUPointLight) * 1024)                        },
            tempBuffer                  { renderSystem.CreateUAVTextureResource(WH, DeviceFormat::R16G16B16A16_FLOAT)               },
			streamingEngine		        { IN_streamingEngine											                            },
			lightMapWH			        { WH / 10                                                                                   }
		{
			RS_IN.RegisterPSOLoader(FORWARDDRAW,			    { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,		   });
			RS_IN.RegisterPSOLoader(FORWARDDRAWINSTANCED,	    { &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO });

			RS_IN.RegisterPSOLoader(LIGHTPREPASS,			    { &RS_IN.Library.ComputeSignature,  CreateLightPassPSO			   });
			RS_IN.RegisterPSOLoader(DEPTHPREPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateDepthPrePassPSO         });

			RS_IN.RegisterPSOLoader(GBUFFERPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferPassPSO          });
			RS_IN.RegisterPSOLoader(GBUFFERPASS_SKINNED,	    { &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferSkinnedPassPSO   });

			RS_IN.RegisterPSOLoader(SHADINGPASS,			    { &RS_IN.Library.RS6CBVs4SRVs,      CreateDeferredShadingPassPSO  });
            RS_IN.RegisterPSOLoader(ENVIRONMENTPASS,            { &RS_IN.Library.RS6CBVs4SRVs,      CreateEnvironmentPassPSO      });
            RS_IN.RegisterPSOLoader(COMPUTETILEDSHADINGPASS,    { &RS_IN.Library.RSDefault,         CreateComputeTiledDeferredPSO });

            RS_IN.RegisterPSOLoader(BILATERALBLURPASSHORIZONTAL,{ &RS_IN.Library.RSDefault,         CreateBilaterialBlurHorizontalPSO });
            RS_IN.RegisterPSOLoader(BILATERALBLURPASSVERTICAL,  { &RS_IN.Library.RSDefault,         CreateBilaterialBlurVerticalPSO   });

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

            RS_IN.SetDebugName(tempBuffer,        "tempBuffer");
            RS_IN.SetDebugName(lightLists,        "lightLists");
            RS_IN.SetDebugName(pointLightBuffer,  "pointLightBuffer");
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
            renderSystem.ReleaseUAV(lightLists);
            renderSystem.ReleaseUAV(pointLightBuffer);
        }


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
                ReserveConstantBufferFunction   reserveCB,
                iAllocator*                     tempMemory,
                LighBufferDebugDraw*            drawDebug = nullptr);


        BackgroundEnvironmentPass& RenderPBR_IBL_Deferred(
            UpdateDispatcher&               dispatcher,
            FrameGraph&                     frameGraph,
            const SceneDescription&         sceneDescription,
            const CameraHandle              camera,
            const ResourceHandle            renderTarget,
            const ResourceHandle            depthTarget,
            const ResourceHandle            diffuseMap,
            const ResourceHandle            GGXMap,
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
            ResourceHandle                  GGXSpecularMap,
            ResourceHandle                  diffuseMap,
            ReserveConstantBufferFunction   reserveCB,
            ReserveVertexBufferFunction     reserveVB,
            float                           t,
            iAllocator*                     allocator);


        ComputeTiledPass& RenderPBR_ComputeDeferredTiledShade(
            UpdateDispatcher&                       dispatcher,
            FrameGraph&                             frameGraph,
            ReserveConstantBufferFunction&          constantBufferAllocator,
            const ComputeTiledDeferredShadeDesc&    scene);

	private:
		RenderSystem&			renderSystem;

        UAVResourceHandle		lightLists;			// GPU
        UAVResourceHandle		pointLightBuffer;	// GPU
        UAVTextureHandle		tempBuffer;	        // GPU

		uint2					lightMapWH;			// Output Size

		TextureStreamingEngine&	streamingEngine;
		bool                    OcclusionCulling;
	};


}	/************************************************************************************************/

#endif
