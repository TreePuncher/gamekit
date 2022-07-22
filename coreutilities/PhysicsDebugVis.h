#pragma once
#include "ResourceHandles.h"
#include "FrameGraph.h"


namespace FlexKit
{
    struct PhysicsDebugOverlayPass
    {
        ReserveVertexBufferFunction     reserveVB;
        ReserveConstantBufferFunction   reserveCB;
        FrameResourceHandle             renderTarget;
    };

    enum class OverlayMode
    {
        Wireframe,
        Solid
    };

    PhysicsDebugOverlayPass& RenderPhysicsOverlay(
        UpdateDispatcher&                   dispatcher,
        FrameGraph&                         frameGraph,
        ResourceHandle                      renderTarget,
        LayerHandle                         layer,
        CameraHandle                        camera,
        ReserveVertexBufferFunction&,
        ReserveConstantBufferFunction&);
}
