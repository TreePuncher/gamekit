#pragma once

#include "graphics.h"
#include "FrameGraph.h"
#include "Scene.h"

namespace FlexKit
{   /************************************************************************************************/


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


    struct ShadowPassMatrices
    {
        float4x4 ViewI[6];
        float4x4 PV[6];
    };


    /************************************************************************************************/

    ShadowPassMatrices CalculateShadowMapMatrices(const float3 pos, const float r, const float T);

    /************************************************************************************************/


    constexpr PSOHandle SHADOWMAPPASS           = PSOHandle(GetTypeGUID(SHADOWMAPPASS));
    constexpr PSOHandle SHADOWMAPANIMATEDPASS   = PSOHandle(GetTypeGUID(SHADOWMAPANIMATEDPASS));


    /************************************************************************************************/

    class ShadowMapper
    {
    public:

        ShadowMapper(RenderSystem& renderSystem)
        {
            renderSystem.RegisterPSOLoader(SHADOWMAPPASS,          { &renderSystem.Library.RS6CBVs4SRVs, CreateShadowMapPass });
            renderSystem.RegisterPSOLoader(SHADOWMAPANIMATEDPASS,  { &renderSystem.Library.RS6CBVs4SRVs, CreateShadowMapAnimatedPass });
        }

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
		                        iAllocator*                             allocator);


        static ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS);
        static ID3D12PipelineState* CreateShadowMapAnimatedPass(RenderSystem* RS);



    };


}   /************************************************************************************************/
