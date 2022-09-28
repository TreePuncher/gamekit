#pragma once

#include "qwidget.h"
#include "ResourceHandles.h"
#include "ClusteredRendering.h"
#include "DepthBuffer.h"

// forward declarations
class EditorProject;
class EditorRenderer;
class EditorSelectedPrefabObject;
class DXRenderWindow;

namespace FlexKit
{
	class EditorRenderer;
	class GameObject;
	class UpdateDispatcher;
	class FrameGraph;
	class ThreadSafeAllocator;
}

struct TemporaryBuffers;

class EditorPrefabPreview : public QWidget
{
public:
	EditorPrefabPreview(EditorRenderer& IN_renderer, EditorSelectedPrefabObject* IN_selection, EditorProject& project, QWidget* parent = nullptr);

	void resizeEvent(QResizeEvent* evt) override;


	void RenderStatic(
		FlexKit::GameObject&			gameObject,
		FlexKit::UpdateDispatcher&		dispatcher,
		double							dT,
		TemporaryBuffers&				temporaryBuffers,
		FlexKit::FrameGraph&			frameGraph,
		FlexKit::ResourceHandle			renderTarget,
		FlexKit::ThreadSafeAllocator&	allocator);


	void RenderAnimated(
		FlexKit::UpdateDispatcher&		dispatcher,
		double							dT,
		TemporaryBuffers&				temporaryBuffers,
		FlexKit::FrameGraph&			frameGraph,
		FlexKit::ResourceHandle			renderTarget,
		FlexKit::ThreadSafeAllocator&	allocator);

	void RenderOverlays(
		FlexKit::UpdateDispatcher&		dispatcher,
		double							dT,
		TemporaryBuffers&				temporaryBuffers,
		FlexKit::FrameGraph&			frameGraph,
		FlexKit::ResourceHandle			renderTarget,
		FlexKit::ThreadSafeAllocator&	allocator);


	bool skeletonOverlay	= false;
	bool turnTable			= false;
	bool animate			= true;
	bool QDTreeOverlay		= false;
	bool boundingVolume		= false;
	bool SMboundingVolumes	= false;

	FlexKit::LayerHandle	layer = FlexKit::InvalidHandle;

	void CenterCamera();

private:
	FlexKit::CameraHandle		previewCamera;
	EditorRenderer&				renderer;
	EditorProject&				project;
	DXRenderWindow*				renderWindow;
	EditorSelectedPrefabObject*	selection;
	FlexKit::DepthBuffer		depthBuffer;
};
