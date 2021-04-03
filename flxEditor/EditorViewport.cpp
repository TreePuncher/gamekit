#include "EditorViewport.h"
#include "DXRenderWindow.h"
#include "EditorRenderer.h"
#include "qevent.h"

#include <QtWidgets/qmenubar.h>


/************************************************************************************************/


EditorViewport::EditorViewport(EditorRenderer& IN_renderer, QWidget *parent)
    :   QWidget{ parent }
    ,   menuBar     { new QMenuBar{ this } }
    ,   renderer    { IN_renderer }
    ,   gbuffer     { { 100, 200 }, IN_renderer.framework.GetRenderSystem() }
    ,   depthBuffer { IN_renderer.framework.GetRenderSystem(), { 100, 200 } }
{
	ui.setupUi(this);

    auto layout = findChild<QBoxLayout*>("verticalLayout");
    layout->setContentsMargins(0, 4, 0, 2);
    layout->setMenuBar(menuBar);

    setMinimumSize(100, 100);
    setMaximumSize({ 1024 * 16, 1024 * 16 });

    auto file   = menuBar->addMenu("View");
    auto select = file->addAction("Select Texture");

    menuBar->show();

    renderWindow = renderer.CreateRenderWindow();

    layout->addWidget(renderWindow);
    adjustSize();

    show();
}


/************************************************************************************************/


EditorViewport ::~EditorViewport()
{
}


/************************************************************************************************/


void EditorViewport::resizeEvent(QResizeEvent* evt)
{
    QWidget::resizeEvent(evt);

    evt->size().width();
    evt->size().height();

    renderWindow->resize({ evt->size().width(), evt->size().height() });
    depthBuffer.Resize({ (uint32_t)evt->size().width(), (uint32_t)evt->size().height() });
    gbuffer.Resize({ (uint32_t)evt->size().width(), (uint32_t)evt->size().height() });
}


/************************************************************************************************/


void EditorViewport::SetScene(EditorScene_ptr scene)
{
    
}


/************************************************************************************************/


void EditorViewport::Render(FlexKit::FrameGraph& frameGraph)
{

}


/************************************************************************************************/


std::shared_ptr<FlexKit::TriMesh> EditorViewport::BuildTriMesh(FlexKit::MeshUtilityFunctions::OptimizedMesh& mesh) const
{
    return {};
}
