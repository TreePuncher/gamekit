#pragma once
#include <buildsettings.hpp>

#include "qwidget.h"
#include <ResourceHandles.hpp>
#include <ClusteredRendering.hpp>
#include <DepthBuffer.hpp>


/************************************************************************************************/


#define LOCALPLAYER 1

// forward declarations
class EditorProject;
class EditorRenderer;
class EditorSelectedPrefabObject;
class DXRenderWindow;

namespace FlexKit
{
	class GameObject;
	class UpdateDispatcher;
	class FrameGraph;
	class ThreadSafeAllocator;
}

struct TemporaryBuffers;
struct PlayerContext;


/************************************************************************************************/


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


/************************************************************************************************/


class EditorPrefabPreview : public QWidget
{
public:
	EditorPrefabPreview(EditorRenderer& IN_renderer, EditorSelectedPrefabObject* IN_selection, EditorProject& project, QWidget* parent = nullptr);
	~EditorPrefabPreview();

	void Reset();

	FlexKit::GameObject* GetGameObject();

	void SetBrush(FlexKit::AssetHandle handle);

	void resizeEvent(QResizeEvent* evt) override;

	void ProcessMessages();

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

	bool playBackWindow		= false;
	bool jointInfoWindow	= false;


	bool	singleStep			= false;
	float	stepSize			= 1.0f / 60.0f;
	float	yaw					= 0.0f;
	float	turnTableRate		= 1.0f;

	FlexKit::LayerHandle	layer = FlexKit::InvalidHandle;

	void CenterCamera();

	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	FlexKit::CameraHandle			previewCamera;
	EditorRenderer&					renderer;
	EditorProject&					project;
	DXRenderWindow*					renderWindow;
	EditorSelectedPrefabObject*		selection;
	FlexKit::DepthBuffer			depthBuffer;

	std::unique_ptr<PlayerContext>	playerContext;
};


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
