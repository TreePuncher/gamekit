#pragma once

#include "graphics.h"
#include "FrameGraph.h"
#include "Scene.h"

namespace FlexKit
{   /************************************************************************************************/


    using AdditionalShadowMapPass =
        TypeErasedCallable
        <void (
            ReserveConstantBufferFunction&,
            ReserveVertexBufferFunction&,
            ConstantBufferDataSet*,
            ResourceHandle*,
            const size_t,
            const ResourceHandler&,
            Context&,
            iAllocator&), 256>;

    struct ShadowMapPassData
	{
		Vector<TemporaryFrameResourceHandle> shadowMapTargets;
	};

	struct LocalShadowMapPassData
	{
        size_t                                  threadIdx;
        size_t                                  threadTotal;
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


    struct ShadowPassMatrices
    {
        float4x4 View[6];
        float4x4 ViewI[6];
        float4x4 PV[6];
    };


    struct AnimationPoseUpload;


    /************************************************************************************************/

    ShadowPassMatrices CalculateShadowMapMatrices(const float3 pos, const float r, const float T);

    /************************************************************************************************/


    constexpr PSOHandle SHADOWMAPPASS               = PSOHandle(GetTypeGUID(SHADOWMAPPASS));
    constexpr PSOHandle SHADOWMAPANIMATEDPASS       = PSOHandle(GetTypeGUID(SHADOWMAPANIMATEDPASS));


    constexpr PassHandle ShadowMapPassID            = PassHandle{ GetTypeGUID(SHADOWMAPPASS) };
    constexpr PassHandle ShadowMapAnimatedPassID    = PassHandle{ GetTypeGUID(SHADOWMAPANIMATEDPASS) };


    /************************************************************************************************/

    class ShadowMapper
    {
    public:
        ShadowMapper(RenderSystem& renderSystem, iAllocator& allocator);

        AcquireShadowMapTask& AcquireShadowMaps(
                                UpdateDispatcher&       dispatcher,
                                RenderSystem&           renderSystem,
                                MemoryPoolAllocator&    shadowMapAllocator,
                                PointLightUpdate&       pointLightUpdate);


	    ShadowMapPassData&  ShadowMapPass(
		                        FrameGraph&                             frameGraph,
                                const PointLightShadowGatherTask&       shadowMaps,
                                UpdateTask&                             cameraUpdate,
                                GatherPassesTask&                       passes,
                                UpdateTask&                             shadowMapAcquire,
                                ReserveConstantBufferFunction           reserveCB,
                                ReserveVertexBufferFunction             reserveVB,
                                static_vector<AdditionalShadowMapPass>& additional,
		                        const double                            t,
		                        iAllocator*                             allocator,
                                AnimationPoseUpload*                    poses = nullptr);

    private:

        struct ResourceEntry
        {
            ResourceHandle  resource;
            size_t          availibility;
        };

        ResourceHandle  GetResource(const size_t frameID);
        void            AddResource(ResourceHandle, const size_t frameID);

        Vector<ResourceEntry>  resourcePool;

        ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS);
        ID3D12PipelineState* CreateShadowMapAnimatedPass(RenderSystem* RS);

        FlexKit::RootSignature rootSignature;
    };


}   /************************************************************************************************/
