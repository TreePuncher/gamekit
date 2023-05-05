#pragma once
#include <buildsettings.h>

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
struct PlayerContext;

struct SharedWindow : public QWidget
{
	Q_OBJECT

public:
	SharedWindow(QWidget* parent) :
		QWidget			{ parent }
	{
		setMinimumSize(100, 100);
		resize(800, 600);

		setFocusPolicy(Qt::StrongFocus);
		setAttribute(Qt::WA_NativeWindow);
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
		setContentsMargins(1, 1, 1, 1);

		show();
	}

	HWND	GetHWND() { return (HWND)winId(); }
};

class EditorPrefabPreview : public QWidget
{
public:
	EditorPrefabPreview(EditorRenderer& IN_renderer, EditorSelectedPrefabObject* IN_selection, EditorProject& project, QWidget* parent = nullptr);

	void resizeEvent(QResizeEvent* evt) override;

	void Update(
		double							dT,
		FlexKit::UpdateDispatcher&		dispatcher,
		FlexKit::ThreadSafeAllocator&	allocator);

	void RenderStatic(
		FlexKit::UpdateDispatcher&		dispatcher,
		FlexKit::FrameGraph&			frameGraph,
		FlexKit::GameObject&			gameObject,
		double							dT,
		TemporaryBuffers&				temporaryBuffers,
		FlexKit::ResourceHandle			renderTarget,
		FlexKit::ThreadSafeAllocator&	allocator);

	void RenderAnimated(
		FlexKit::UpdateDispatcher&		dispatcher,
		FlexKit::FrameGraph&			frameGraph,
		double							dT,
		TemporaryBuffers&				temporaryBuffers,
		FlexKit::ResourceHandle			renderTarget,
		FlexKit::ThreadSafeAllocator&	allocator);

	void RenderOverlays(
		FlexKit::UpdateDispatcher&		dispatcher,
		FlexKit::FrameGraph&			frameGraph,
		double							dT,
		TemporaryBuffers&				temporaryBuffers,
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
	FlexKit::CameraHandle			previewCamera;
	EditorRenderer&					renderer;
	EditorProject&					project;
	DXRenderWindow*					renderWindow;
	SharedWindow					sharedWindow;
	EditorSelectedPrefabObject*		selection;
	//FlexKit::DepthBuffer			depthBuffer;

	std::unique_ptr<PlayerContext>	playerContext;
};
