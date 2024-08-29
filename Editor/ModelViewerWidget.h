#pragma once

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qmenubar.h>
#include <qevent.h>
#include <WorldRender.hpp>

#include "ui_ModelViewerWidget.h"
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


/**********************************************************************

Copyright (c) 2022 - 2024 Robert May

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
