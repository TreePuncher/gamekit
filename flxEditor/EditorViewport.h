#pragma once

#include <QtWidgets/QWidget>
#include "ui_EditorViewport.h"
#include "WorldRender.h"
#include "EditorRenderer.h"
#include "EditorProject.h"
#include "GraphicsComponents.h"

class QMenuBar;

class EditorViewport : public QWidget
{
	Q_OBJECT

public:
    EditorViewport(EditorRenderer& IN_renderer, QWidget *parent = Q_NULLPTR);
	~EditorViewport();

    void resizeEvent(QResizeEvent* event) override;

    void SetScene(EditorScene_ptr scene);


    std::shared_ptr<FlexKit::TriMesh> BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const;


private:


    void Render(FlexKit::FrameGraph& frameGraph);

	Ui::EditorViewport ui;

    DXRenderWindow*         renderWindow;
    EditorRenderer&         renderer;
    QMenuBar*               menuBar;

    FlexKit::GBuffer        gbuffer;
    FlexKit::DepthBuffer    depthBuffer;

    EditorScene_ptr         scene;
    FlexKit::CameraHandle   viewportCamera;
};
