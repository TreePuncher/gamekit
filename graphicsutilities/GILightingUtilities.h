#pragma once

#include "graphics.h"
#include "FrameGraph.h"
#include "ClusteredRendering.h"

namespace FlexKit
{   /************************************************************************************************/


    class GITechniqueInterface
    {
    public:
        virtual ~GITechniqueInterface() {}

        virtual void Init(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB) = 0;

        virtual void BuildScene(
            FrameGraph&                     frameGraph,
            Scene&                          scene,
            GatherPassesTask&               passes,
            ReserveConstantBufferFunction   reserveCB) = 0;

        virtual void RayTrace(
            UpdateDispatcher&               dispatcher,
            FrameGraph&                     frameGraph,
            const CameraHandle              camera,
            GatherPassesTask&               passes,
            ResourceHandle                  depthTarget,
            FrameResourceHandle             renderTarget,
            GBuffer&                        gbuffer,
            ReserveConstantBufferFunction   reserveCB,
            iAllocator*                     allocator) = 0;
    };


    /************************************************************************************************/


    enum class EGITECHNIQUE
    {
        VXGL,
        RT_SURFELS,
        DISABLE,
        AUTOMATIC
    };


    class GlobalIlluminationEngine
    {
    public:
        GlobalIlluminationEngine(RenderSystem& renderSystem, iAllocator& allocator, EGITECHNIQUE technique = EGITECHNIQUE::AUTOMATIC);
        ~GlobalIlluminationEngine();


        void Init(FrameGraph& frameGraph, ReserveConstantBufferFunction& reserveCB);

        void BuildScene(
            FrameGraph&                     frameGraph,
            Scene&                          scene,
            GatherPassesTask&               passes,
            ReserveConstantBufferFunction   reserveCB);

        void RayTrace(
            UpdateDispatcher&               dispatcher,
            FrameGraph&                     frameGraph,
            const CameraHandle              camera,
            GatherPassesTask&               passes,
            ResourceHandle                  depthTarget,
            FrameResourceHandle             renderTarget,
            GBuffer&                        gbuffer,
            ReserveConstantBufferFunction   reserveCB,
            iAllocator*                     allocator);

    private:
        GITechniqueInterface*   technique = nullptr;
        iAllocator&             allocator;
    };


}   /************************************************************************************************/
