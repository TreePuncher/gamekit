#pragma once

#include "ResourceHandles.h"
#include "FrameGraph.h"


namespace FlexKit
{
	struct PhysicsDebugOverlayPass
	{
		ReserveVertexBufferFunction		reserveVB;
		ReserveConstantBufferFunction	reserveCB;
		FrameResourceHandle				renderTarget;
		FrameResourceHandle				depthTarget;
	};

	void RegisterPhysicsDebugVis(RenderSystem&);

	PhysicsDebugOverlayPass& RenderPhysicsOverlay(
		FrameGraph&							frameGraph,
		ResourceHandle						renderTarget,
		ResourceHandle						depthTarget,
		LayerHandle							layer,
		CameraHandle						camera,
		const ReserveVertexBufferFunction&,
		const ReserveConstantBufferFunction&);
}
