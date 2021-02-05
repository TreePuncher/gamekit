#pragma once

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qmenubar.h>
#include <qevent.h>

#include "ui_ModelViewerWidget.h"
#include "WorldRender.h"
#include "DXRenderWindow.h"

class EditorRenderer;
class DXRenderWindow;

class ModelViewerWidget : public QWidget
{
	Q_OBJECT

public:
	ModelViewerWidget(EditorRenderer& IN_renderer, QWidget* parent);
	~ModelViewerWidget();

    void OnDraw(FlexKit::UpdateDispatcher& Dispatcher, double dT, TemporaryBuffers&, FlexKit::FrameGraph& frameGraph, FlexKit::ResourceHandle renderTarget);

private:
	Ui::ModelViewerWidget       ui;
    EditorRenderer&             renderer;
    DXRenderWindow&             renderWindow;

    FlexKit::GBuffer            gbuffer;
    FlexKit::ResourceHandle     depthBuffer;
    QMenuBar* menuBar;
};
