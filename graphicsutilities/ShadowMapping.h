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

        ShadowMapper(RenderSystem& renderSystem, iAllocator& allocator) :
            rootSignature   { &allocator },
            resourcePool    { &allocator }
        {
            rootSignature.AllowIA = true;
            rootSignature.AllowSO = false;
            rootSignature.SetParameterAsCBV(1,      0, 0, PIPELINE_DEST_VS);
            rootSignature.SetParameterAsUINT(2, 16, 1, 0, PIPELINE_DEST_VS);
            //rootSignature.SetParameterAsCBV(2,      1, 0, PIPELINE_DEST_VS);
            rootSignature.SetParameterAsCBV(3,      2, 0, PIPELINE_DEST_VS);
            rootSignature.SetParameterAsUINT(0, 20, 3, 0, PIPELINE_DEST_VS);
            rootSignature.Build(renderSystem, &allocator);

            renderSystem.RegisterPSOLoader(SHADOWMAPPASS,          { &renderSystem.Library.RS6CBVs4SRVs, [&](RenderSystem* rs) { return CreateShadowMapPass(rs); }});
            renderSystem.RegisterPSOLoader(SHADOWMAPANIMATEDPASS,  { &renderSystem.Library.RS6CBVs4SRVs, [&](RenderSystem* rs) { return CreateShadowMapAnimatedPass(rs); } });
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

    private:

        struct ResourceEntry
        {
            ResourceHandle  resource;
            size_t          availibility;
        };

        ResourceHandle  GetResource(const size_t frameID);
        void            AddResource(ResourceHandle, const size_t frameID);

        std::mutex m;
        Vector<ResourceEntry>  resourcePool;

        ID3D12PipelineState* CreateShadowMapPass(RenderSystem* RS);
        ID3D12PipelineState* CreateShadowMapAnimatedPass(RenderSystem* RS);

        FlexKit::RootSignature rootSignature;
    };


}   /************************************************************************************************/
