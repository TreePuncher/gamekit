#pragma once

#include "EditorRenderer.h"
#include "EditorProject.h"
#include "GraphicsComponents.h"
#include "SelectionContext.h"
#include "WorldRender.h"
#include "ViewportScene.h"
#include "DebugUI.h"
#include "ui_EditorViewport.h"

#include <QtWidgets/QWidget>
#include <qt>
#include <imgui.h>
#include <ImGuizmo.h>


class QMenuBar;


/************************************************************************************************/


using ViewportModeID = uint32_t;

class IEditorViewportMode
{
public:
    virtual ~IEditorViewportMode() {}

    virtual ViewportModeID GetModeID() const = 0;

    virtual void keyPressEvent      (QKeyEvent* event) {};
    virtual void keyReleaseEvent    (QKeyEvent* event) {};

    virtual void mousePressEvent    (QMouseEvent* event) {};
    virtual void mouseMoveEvent     (QMouseEvent* event) {};
    virtual void mouseReleaseEvent  (QMouseEvent* event) {};
    virtual void wheelEvent         (QWheelEvent* event) {};

    virtual void DrawImguI() {};
    virtual void Draw(FlexKit::UpdateDispatcher& Dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps) {};
};

using ViewportMode_ptr = std::shared_ptr<IEditorViewportMode>;


/************************************************************************************************/


constexpr ViewportModeID VewportSelectionModeID = GetTypeGUID(EditorVewportSelectionMode);

class EditorVewportSelectionMode : public IEditorViewportMode
{
public:
    EditorVewportSelectionMode(SelectionContext&, std::shared_ptr<ViewportScene>&, DXRenderWindow* window, FlexKit::CameraHandle camera);

    ViewportModeID GetModeID() const override { return VewportSelectionModeID; };

    void mousePressEvent    (QMouseEvent* event) override;

    DXRenderWindow*         renderWindow;
    FlexKit::CameraHandle   viewportCamera;

    SelectionContext&               selectionContext;
    std::shared_ptr<ViewportScene>  scene;
};


/************************************************************************************************/


constexpr ViewportModeID VewportPanModeID = GetTypeGUID(IEditorVewportPanMode);

class EditorVewportPanMode : public IEditorViewportMode
{
public:
    EditorVewportPanMode(SelectionContext&, std::shared_ptr<ViewportScene>&, DXRenderWindow* window, FlexKit::CameraHandle camera);

    ViewportModeID GetModeID() const override { return VewportPanModeID; };

    void keyPressEvent      (QKeyEvent* event) override;

    void mouseMoveEvent     (QMouseEvent* event) override;
    void mouseReleaseEvent  (QMouseEvent* event) override;
    void wheelEvent         (QWheelEvent* event) override;

    DXRenderWindow*         renderWindow;
    FlexKit::CameraHandle   viewportCamera;

    SelectionContext&               selectionContext;
    std::shared_ptr<ViewportScene>  scene;

    FlexKit::int2 previousMousePosition = FlexKit::int2{ -160000, -160000 };
    float panSpeed = 10.0f;
};


/************************************************************************************************/


constexpr ViewportModeID TranslationModeID = GetTypeGUID(EditorVewportTranslationMode);


class EditorVewportTranslationMode : public IEditorViewportMode
{
public:
    EditorVewportTranslationMode(SelectionContext&, DXRenderWindow*, FlexKit::CameraHandle, FlexKit::ImGUIIntegrator& hud);

    ViewportModeID GetModeID() const override { return TranslationModeID; };

    void keyPressEvent      (QKeyEvent* event) override;

    void mousePressEvent    (QMouseEvent* event) override;
    void mouseReleaseEvent  (QMouseEvent* event) override;

    void DrawImguI  () override;

    DXRenderWindow*             renderWindow;
    SelectionContext&           selectionContext;
    FlexKit::CameraHandle       viewportCamera;
    FlexKit::ImGUIIntegrator&   hud;

    FlexKit::int2 previousMousePosition     = FlexKit::int2{ -160000, -160000 };
    ImGuizmo::OPERATION manipulatorState    = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE mode                     = ImGuizmo::MODE::LOCAL;
};


/************************************************************************************************/


class EditorViewport : public QWidget
{
	Q_OBJECT

public:
    EditorViewport(EditorRenderer& IN_renderer, SelectionContext& IN_context, QWidget *parent = Q_NULLPTR);
	~EditorViewport();

    void resizeEvent    (QResizeEvent* event) override;
    void SetScene       (EditorScene_ptr scene);
    void SetMode        (IEditorViewportMode* mode);

    std::shared_ptr<ViewportScene>& GetScene()              { return scene; }
    SelectionContext&               GetSelectionContext()   { return selectionContext; }

protected:
    void keyPressEvent      (QKeyEvent* event) override;
    void keyReleaseEvent    (QKeyEvent* event) override;
    void mousePressEvent    (QMouseEvent* event) override;
    void mouseMoveEvent     (QMouseEvent* event) override;
    void mouseReleaseEvent  (QMouseEvent* event) override;
    void wheelEvent         (QWheelEvent* event) override;

    std::shared_ptr<FlexKit::TriMesh> BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const;

private:

    void SaveScene();

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

    std::vector<ViewportMode_ptr>   mode;

    FlexKit::int2                   previousMousePosition{ -160000, -160000 };

	Ui::EditorViewport              ui;
    FlexKit::ImGUIIntegrator        hud;

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
