#pragma once

#include <QtWidgets/QWidget>
#include "ui_EditorViewport.h"
#include "WorldRender.h"
#include "EditorRenderer.h"
#include "EditorProject.h"
#include "GraphicsComponents.h"
#include <qt>
class QMenuBar;


/************************************************************************************************/


struct ViewportGameObject
{
    FlexKit::GameObject gameObject;
    uint64_t            objectID;
};

using ViewportGameObject_ptr = std::shared_ptr<ViewportGameObject>;

struct ViewportScene
{
    ViewportScene(EditorScene_ptr IN_sceneResource) :
            sceneResource   { IN_sceneResource  } {}

    EditorScene_ptr                     sceneResource;
    std::vector<ViewportGameObject_ptr> sceneObjects;
    FlexKit::GraphicScene               scene           { FlexKit::SystemAllocator };
    FlexKit::PhysXSceneHandle           physicsLayer    = FlexKit::InvalidHandle_t;
};

/************************************************************************************************/


class EditorViewport : public QWidget
{
	Q_OBJECT

public:
    EditorViewport(EditorRenderer& IN_renderer, QWidget *parent = Q_NULLPTR);
	~EditorViewport();

    void resizeEvent(QResizeEvent* event) override;

    void SetScene(EditorScene_ptr scene);

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;


    std::shared_ptr<FlexKit::TriMesh> BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const;

private:

    void Render(FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers&, FlexKit::FrameGraph& graph, FlexKit::ResourceHandle renderTarget, FlexKit::ThreadSafeAllocator& allocator);

    double                          T = 0.0f;

    enum class InputState {
        None,
        Pan,
        Orbit
    } state = InputState::None;

    FlexKit::int2                   previousMousePosition{ -160000, -160000 };
    float                           panSpeed = 10.0f;

	Ui::EditorViewport              ui;

    DXRenderWindow*                 renderWindow;
    EditorRenderer&                 renderer;
    QMenuBar*                       menuBar;

    FlexKit::GBuffer                gbuffer;
    FlexKit::DepthBuffer            depthBuffer;
    FlexKit::CameraHandle           viewportCamera;

    std::shared_ptr<ViewportScene>  scene;
};


/************************************************************************************************/
