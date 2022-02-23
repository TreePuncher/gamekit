#pragma once

#include "CSGComponent.h"
#include "FrameGraph.h"
#include <vector>


class CSGRender
{
public:
    struct CSGRenderData
    {
        FlexKit::ReserveConstantBufferFunction& cb;
        FlexKit::ReserveVertexBufferFunction    ReserveVertexBuffer;
    };

    CSGRenderData& Render(
        FlexKit::UpdateDispatcher&              dispatcher,
        FlexKit::FrameGraph&                    frameGraph,
        FlexKit::ReserveConstantBufferFunction& cb,
        FlexKit::ReserveVertexBufferFunction&   vb,
        FlexKit::FrameResourceHandle            renderTarget,
        const double                            dt);
};

