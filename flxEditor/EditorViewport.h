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

    virtual void DrawImguI() {}
    virtual void Draw(FlexKit::UpdateDispatcher& Dispatcher, FlexKit::FrameGraph& frameGraph, TemporaryBuffers& temps, FlexKit::ResourceHandle renderTarget, FlexKit::ResourceHandle depthBuffer) {};
};

using ViewportMode_ptr = std::shared_ptr<IEditorViewportMode>;


/************************************************************************************************/


constexpr ViewportModeID VewportSelectionModeID = GetTypeGUID(EditorVewportSelectionMode);
constexpr ViewportModeID VewportPanModeID       = GetTypeGUID(IEditorVewportPanMode);
constexpr ViewportModeID TranslationModeID      = GetTypeGUID(EditorVewportTranslationMode);


/************************************************************************************************/


class EditorViewport : public QWidget
{
	Q_OBJECT

public:
    EditorViewport(EditorRenderer& IN_renderer, SelectionContext& IN_context, QWidget *parent = Q_NULLPTR);
	~EditorViewport();

    void resizeEvent    (QResizeEvent* event) override;
    void SetScene       (EditorScene_ptr scene);


    std::shared_ptr<ViewportScene>& GetScene()              { return scene; }
    SelectionContext&               GetSelectionContext()   { return selectionContext; }

    void PushMode(ViewportMode_ptr newMode)
    {
        if(mode.empty() || newMode->GetModeID() != mode.back()->GetModeID())
            mode.push_back(newMode);
    }

    void ClearMode()
    {
        while (mode.size())
            mode.pop_back();
    }

    void ClearSelection()
    {
        selectionContext.Clear();
    }


    FlexKit::ImGUIIntegrator& GetHUD()
    {
        return hud;
    }

    FlexKit::CameraHandle GetViewportCamera() const noexcept { return viewportCamera; }

    FlexKit::Ray GetMouseRay() const;

    FlexKit::uint2 WH() const noexcept { return renderWindow->WH(); }

    FlexKit::TriMeshHandle LoadTriMeshResource(ProjectResource_ptr res);

    FlexKit::MaterialHandle gbufferPass;

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

Copyright (c) 2022 Robert May

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
