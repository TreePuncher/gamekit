#pragma once

#include "EditorRenderer.h"
#include "EditorProject.h"
#include "GraphicsComponents.h"
#include "SelectionContext.h"
#include "WorldRender.h"
#include "ViewportScene.h"

#include "ui_EditorViewport.h"

#include <QtWidgets/QWidget>
#include <qt>


class QMenuBar;


/************************************************************************************************/


class EditorViewport : public QWidget
{
	Q_OBJECT

public:
    EditorViewport(EditorRenderer& IN_renderer, SelectionContext& IN_context, QWidget *parent = Q_NULLPTR);
	~EditorViewport();

    void resizeEvent    (QResizeEvent* event) override;
    void SetScene       (EditorScene_ptr scene);

protected:
    void keyPressEvent      (QKeyEvent* event) override;
    void keyReleaseEvent    (QKeyEvent* event) override;
    void mousePressEvent    (QMouseEvent* event) override;
    void mouseMoveEvent     (QMouseEvent* event) override;
    void mouseReleaseEvent  (QMouseEvent* event) override;
    void wheelEvent         (QWheelEvent* event) override;

    std::shared_ptr<FlexKit::TriMesh> BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const;

private:

    struct DrawSceneOverlay_Desc
    {
        const FlexKit::PVS&                     brushes;
        FlexKit::PointLightShadowGatherTask&    lights;

        TemporaryBuffers&               buffers;
        FlexKit::ResourceHandle         renderTarget;
        FlexKit::ThreadSafeAllocator&   allocator;
    };


    void Render             (FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers&, FlexKit::FrameGraph& graph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator);
    void DrawSceneOverlay   (FlexKit::UpdateDispatcher& Dispatcher, FlexKit::FrameGraph& frameGraph, DrawSceneOverlay_Desc& desc);

    double                          T = 0.0f;

    enum class InputState {
        PanOrbit,
        ClickSelect
    } state = InputState::ClickSelect;

    FlexKit::int2                   previousMousePosition{ -160000, -160000 };
    float                           panSpeed = 10.0f;

	Ui::EditorViewport              ui;

    DXRenderWindow*                 renderWindow;
    SelectionContext&               selectionContext;
    EditorRenderer&                 renderer;
    QMenuBar*                       menuBar;

    FlexKit::GBuffer                gbuffer;
    FlexKit::DepthBuffer            depthBuffer;
    FlexKit::CameraHandle           viewportCamera = FlexKit::InvalidHandle_t;

    std::shared_ptr<ViewportScene>  scene;
};


/**********************************************************************

Copyright (c) 2021 Robert May

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
