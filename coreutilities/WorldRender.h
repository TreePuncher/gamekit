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
	const PSOHandle GBUFFERPASS             = PSOHandle(GetTypeGUID(GBUFFERPASS));
	const PSOHandle SHADINGPASS             = PSOHandle(GetTypeGUID(SHADINGPASS));
	const PSOHandle ENVIRONMENTPASS         = PSOHandle(GetTypeGUID(ENVIRONMENTPASS));
	const PSOHandle LIGHTPREPASS			= PSOHandle(GetTypeGUID(LIGHTPREPASS));
	const PSOHandle DEPTHPREPASS            = PSOHandle(GetTypeGUID(DEPTHPREPASS));
	const PSOHandle FORWARDDRAWINSTANCED	= PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	const PSOHandle FORWARDDRAW_OCCLUDE		= PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));

	ID3D12PipelineState* CreateForwardDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateForwardDrawInstancedPSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateOcclusionDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateLightPassPSO				(RenderSystem* RS);
    ID3D12PipelineState* CreateDepthPrePassPSO          (RenderSystem* RS);
    ID3D12PipelineState* CreateGBufferPassPSO           (RenderSystem* RS);
    ID3D12PipelineState* CreateDeferredShadingPassPSO   (RenderSystem* RS);
    ID3D12PipelineState* CreateEnvironmentPassPSO       (RenderSystem* RS);


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
		Vector<GPUPointLight>		    pointLights;
		const Vector<PointLightHandle>*	pointLightHandles;
        UploadSegment			        lightBuffer;	// immediate update

		CameraHandle			camera;

		CBPushBuffer			constants;
		TextureHandle			lightMap;
		TextureHandle			lightListBuffer;

		FrameResourceHandle		lightMapObject;
		FrameResourceHandle		lightListObject;
		FrameResourceHandle		lightBufferObject;


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
		UpdateTaskTyped<PointLightGather>&	lights;
		UpdateTask&							transforms;
		UpdateTask&							cameras;
		UpdateTaskTyped<GetPVSTaskData>&	PVS;
	};

    struct DepthPass
    {
        DepthPass(const PVS& IN_drawables) :
            drawables{ IN_drawables } {}

        const PVS&          drawables;
        TextureHandle       depthPassTarget;
        FrameResourceHandle depthBufferObject;

        CBPushBuffer passConstantsBuffer;
        CBPushBuffer entityConstantsBuffer;

        DescriptorHeap Heap; // Null Filled
    };

    using PointLightHandleList = Vector<PointLightHandle>;


    struct ForwardDrawConstants
    {
        float LightCount;
        float t;
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

        const CBPushBuffer&         entityConstants;

        const PointLightHandleList& pointLights;
        const PVS&                  drawables;

        DescriptorHeap				Heap; // Null Filled
    };


	/************************************************************************************************/


    class GBuffer
    {
    public:
        GBuffer(const uint2 WH, RenderSystem& RS_IN) :
            RS          { RS_IN },
            Albedo      { RS_IN.CreateTexture2D(TextureDesc::RenderTarget(WH, FORMAT_2D::R8G8B8A8_UNORM)) },
            Specular    { RS_IN.CreateTexture2D(TextureDesc::RenderTarget(WH, FORMAT_2D::R8G8B8A8_UNORM)) },
            Normal      { RS_IN.CreateTexture2D(TextureDesc::RenderTarget(WH, FORMAT_2D::R16G16B16A16_FLOAT)) },
            Tangent     { RS_IN.CreateTexture2D(TextureDesc::RenderTarget(WH, FORMAT_2D::R16G16B16A16_FLOAT)) },
            IOR_ANISO   { RS_IN.CreateTexture2D(TextureDesc::RenderTarget(WH, FORMAT_2D::R16G16_FLOAT)) }
        {
            RS.SetDebugName(Albedo,     "Albedo");
            RS.SetDebugName(Specular,   "Specular");
            RS.SetDebugName(Normal,     "Normal");
            RS.SetDebugName(Tangent,    "Tangent");
            RS.SetDebugName(IOR_ANISO,  "IOR_ANISO");
        }

        ~GBuffer()
        {
            RS.ReleaseTexture(Albedo);
            RS.ReleaseTexture(Specular);
            RS.ReleaseTexture(Normal);
            RS.ReleaseTexture(Tangent);
            RS.ReleaseTexture(IOR_ANISO);
        }

        TextureHandle Albedo;	 // rgba_UNORM, Albedo + Metal
        TextureHandle Specular;	 // rgba_UNORM, Specular + roughness
        TextureHandle Normal;	 // float16_RGBA
        TextureHandle Tangent;	 // float16_RGBA
        TextureHandle IOR_ANISO; // float16_RG

        RenderSystem&    RS;
    };


    void ClearGBuffer(GBuffer& gbuffer, FrameGraph& frameGraph)
    {
        struct GBufferClear
        {
            FrameResourceHandle albedo;
            FrameResourceHandle specular;
            FrameResourceHandle normal;
            FrameResourceHandle tangent;
            FrameResourceHandle IOR_ANISO;
        };

        auto& clear = frameGraph.AddNode<GBufferClear>(
            GBufferClear{},
            [&](FrameGraphNodeBuilder& builder, GBufferClear& data)
            {
                data.albedo     = builder.WriteRenderTarget(gbuffer.Albedo);
                data.specular   = builder.WriteRenderTarget(gbuffer.Specular);
                data.normal     = builder.WriteRenderTarget(gbuffer.Normal);
                data.tangent    = builder.WriteRenderTarget(gbuffer.Tangent);
                data.IOR_ANISO  = builder.WriteRenderTarget(gbuffer.IOR_ANISO);
            },
            [](GBufferClear& data, FrameResources& resources, Context* ctx)
            {
                ctx->ClearRenderTarget(resources.GetRenderTargetObject(data.albedo));
                ctx->ClearRenderTarget(resources.GetRenderTargetObject(data.specular));
                ctx->ClearRenderTarget(resources.GetRenderTargetObject(data.normal));
                ctx->ClearRenderTarget(resources.GetRenderTargetObject(data.tangent));
                ctx->ClearRenderTarget(resources.GetRenderTargetObject(data.IOR_ANISO));
            });
    }


    void AddGBufferResource(GBuffer& gbuffer, FrameGraph& frameGraph)
    {
        frameGraph.Resources.AddShaderResource(gbuffer.Albedo,      true);
        frameGraph.Resources.AddShaderResource(gbuffer.Specular,    true);
        frameGraph.Resources.AddShaderResource(gbuffer.Normal,      true);
        frameGraph.Resources.AddShaderResource(gbuffer.IOR_ANISO,   true);
        frameGraph.Resources.AddShaderResource(gbuffer.Tangent,     true);
    }


    struct GBufferPass
    {
        GBuffer&        gbuffer;
        const PVS&      pvs;

        CBPushBuffer    entityConstants;
        CBPushBuffer    passConstants;

        DescriptorHeap	Heap;

        FrameResourceHandle AlbedoTargetObject;     // RGBA8
        FrameResourceHandle NormalTargetObject;     // RGBA16Float
        FrameResourceHandle SpecularTargetObject;
        FrameResourceHandle TangentTargetObject;
        FrameResourceHandle IOR_ANISOTargetObject;  // RGBA8

        FrameResourceHandle depthBufferTargetObject;
    };


    struct BackgroundEnvironmentPass
    {
        TextureHandle       HDRMap;
        DescriptorHeap	    descHeap;

        FrameResourceHandle renderTargetObject;

        VBPushBuffer        passVertices;
        CBPushBuffer        passConstants;

        CameraHandle        camera;
    };

    struct TiledDeferredShade
    {
        GBuffer&                gbuffer;
        PointLightGatherTask&   lights;

        CBPushBuffer            passConstants;
        VBPushBuffer            passVertices;
        DescriptorHeap	        descHeap;

        FrameResourceHandle     AlbedoTargetObject;     // RGBA8
        FrameResourceHandle     NormalTargetObject;     // RGBA16Float
        FrameResourceHandle     SpecularTargetObject;
        FrameResourceHandle     TangentTargetObject;
        FrameResourceHandle     IOR_ANISOTargetObject;  // RGBA8
        FrameResourceHandle     depthBufferTargetObject;

        FrameResourceHandle     renderTargetObject;
    };


    /************************************************************************************************/


	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(iAllocator* Memory, RenderSystem& RS_IN, TextureStreamingEngine& IN_streamingEngine, const uint2 WH) :

			RS                  { RS_IN                                                             },
			ConstantBuffer		{ RS.CreateConstantBuffer(64 * MEGABYTE, false)						},
			OcclusionCulling	{ false																},
			lightLists			{ RS.CreateUAVBufferResource(sizeof(uint32_t) * 108 * 192 * 1024)   },
			pointLightBuffer	{ RS.CreateUAVBufferResource(sizeof(GPUPointLight) * 1024)          },
			lightMap			{ RS.CreateUAVTextureResource(WH / 10, FORMAT_2D::R32G32_UINT)      },
			streamingEngine		{ IN_streamingEngine											    },
			lightMapWH			{ WH / 10                                                           }
		{
			RS_IN.RegisterPSOLoader(FORWARDDRAW,			{ &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,			});
			RS_IN.RegisterPSOLoader(FORWARDDRAWINSTANCED,	{ &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO	});
			RS_IN.RegisterPSOLoader(FORWARDDRAW_OCCLUDE,	{ &RS_IN.Library.RS6CBVs4SRVs,	    CreateOcclusionDrawPSO			});
			RS_IN.RegisterPSOLoader(LIGHTPREPASS,			{ &RS_IN.Library.ComputeSignature,  CreateLightPassPSO				});
			RS_IN.RegisterPSOLoader(DEPTHPREPASS,			{ &RS_IN.Library.RS6CBVs4SRVs,      CreateDepthPrePassPSO           });
			RS_IN.RegisterPSOLoader(GBUFFERPASS,			{ &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferPassPSO            });
			RS_IN.RegisterPSOLoader(SHADINGPASS,			{ &RS_IN.Library.RS6CBVs4SRVs,      CreateDeferredShadingPassPSO    });
            RS_IN.RegisterPSOLoader(ENVIRONMENTPASS,        { &RS_IN.Library.RS6CBVs4SRVs,      CreateEnvironmentPassPSO        });

            RS_IN.QueuePSOLoad(GBUFFERPASS);
            RS_IN.QueuePSOLoad(DEPTHPREPASS);
            RS_IN.QueuePSOLoad(LIGHTPREPASS);
            RS_IN.QueuePSOLoad(FORWARDDRAW);
            RS_IN.QueuePSOLoad(FORWARDDRAWINSTANCED);
            RS_IN.QueuePSOLoad(SHADINGPASS);

            RS.SetDebugName(lightLists,        "lightLists");
            RS.SetDebugName(pointLightBuffer,  "pointLightBuffer");
            RS.SetDebugName(lightMap,          "lightMap");
		}


		~WorldRender()
		{
			RS.ReleaseCB(ConstantBuffer);
			RS.ReleaseUAV(lightMap);
			RS.ReleaseUAV(lightLists);
            RS.ReleaseUAV(pointLightBuffer);


		}


        void Begin(FrameGraph& frameGraph)
        {
        }


        DepthPass& DepthPrePass(
                UpdateDispatcher&   dispatcher,
                FrameGraph&         frameGraph,
                const CameraHandle  camera,
                GatherTask&         pvs,
                const TextureHandle depthBufferTarget,
                iAllocator*         allocator);

        
        BackgroundEnvironmentPass& BackgroundPass(
                UpdateDispatcher&       dispatcher,
                FrameGraph&             frameGraph,
                const CameraHandle      camera,
                const TextureHandle     renderTarget,
                const TextureHandle     hdrMap,
                VertexBufferHandle      vertexBuffer,
                iAllocator*             tempMemory);


        ForwardPlusPass& RenderPBR_ForwardPlus(
                UpdateDispatcher&           dispatcher,
                FrameGraph&                 frameGraph,
                const DepthPass&            depthPass,
                const CameraHandle          camera,
                const WorldRender_Targets&  Target,
                const SceneDescription&     desc,
                const float                 t,
                TextureHandle               environmentMap,
                iAllocator*                 allocator);


		LightBufferUpdate& UpdateLightBuffers(
                UpdateDispatcher&       dispatcher,
                FrameGraph&             frameGraph,
                const CameraHandle      camera,
                const GraphicScene&     scene,
                const SceneDescription& desc,
                iAllocator*             tempMemory,
                LighBufferDebugDraw*    drawDebug = nullptr);


        GBufferPass&        RenderPBR_GBufferPass(
            UpdateDispatcher&   dispatcher,
            FrameGraph&         frameGraph,
            const CameraHandle  camera,
            GatherTask&         pvs,
            GBuffer&            gbuffer,
            TextureHandle       depthTarget,
            iAllocator*         allocator);


        TiledDeferredShade& RenderPBR_DeferredShade(
            UpdateDispatcher&       dispatcher,
            FrameGraph&             frameGraph,
            const CameraHandle      camera,
            PointLightGatherTask&   gather,
            GBuffer&                gbuffer,
            TextureHandle           depthTarget,
            TextureHandle           renderTarget,
            TextureHandle           environmentMap,
            VertexBufferHandle      vertexBuffer,
            float                   t,
            iAllocator*             allocator);


	private:
		RenderSystem&			RS;
        ConstantBufferHandle	ConstantBuffer;
		//QueryHandle			OcclusionQueries;
		//TextureHandle			OcclusionBuffer;


		UAVTextureHandle		lightMap;			// GPU
        UAVResourceHandle		lightLists;			// GPU
		UAVResourceHandle		pointLightBuffer;	// GPU

		uint2					WH;					// Output Size
		uint2					lightMapWH;			// Output Size

		TextureStreamingEngine&	streamingEngine;
		bool                    OcclusionCulling;

		// GBuffer
		//RenderTargetHandle		Albedo;
		//RenderTargetHandle		Color;
		//RenderTargetHandle		Normal;
		//RenderTargetHandle		Depth;
	};


}	/************************************************************************************************/

#endif
