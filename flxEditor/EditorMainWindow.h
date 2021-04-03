#pragma once

#include <memory>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qmainwindow>
#include <QtWidgets/qapplication.h>
#include <qtimer>
#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qtextedit.h>
#include <qsettings.h>
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qtabwidget.h>

#include "ResourceBrowserWidget.h"
#include "EditorImport.h"
#include "EditorRenderer.h"
#include "EditorProject.h"
#include "EditorGadgetInterface.h"
#include "EditorViewport.h"
#include "TextureViewer.h"

#include "ModelViewerWidget.h"


/************************************************************************************************/

class TextureResource;
class EditorScriptEngine;
class EditorViewport;


/************************************************************************************************/

using FlexKit::ResourceBuilder::ResourceList;

class SelectionContext
{
public:
    std::optional<ResourceList> GetSelectedResources() { return {}; }
};

/************************************************************************************************/

class EditorMainWindow : public QMainWindow
{
	Q_OBJECT

public:
    EditorMainWindow(EditorRenderer& IN_renderer, EditorScriptEngine& scriptEngine, EditorProject& IN_project, QApplication& application, QWidget *parent = Q_NULLPTR);
	~EditorMainWindow();


    void                    AddImporter(iEditorImportor* importer);
    void                    AddExporter(iEditorExporter* exporter);

    QTextEdit*              AddTextView();
    DXRenderWindow*         AddViewPort();
    ResourceBrowserWidget*  AddResourceList();
    void                    AddEditorView();
    void                    AddOutputView();
    void                    AddModelViewer();
    TextureViewer*          AddTextureViewer(TextureResource* res = nullptr);

    void AddFileAction(const std::string& name, auto callable)
    {
        auto action = fileMenu->addAction(name.c_str());
        connect(action, &QAction::triggered, this, callable);
    }


    void RegisterGadget(iEditorGadget* gadget);

    void CloseEvent(QCloseEvent* event);
    void Close();
    void timerEvent(QTimerEvent* event);
    void showEvent(QShowEvent* event) override {}

public slots:

    void Update();


signals:
    void onClose();

private:

    SelectionContext    selectionContext;

    QTabWidget*         tabBar;
    EditorViewport*     viewport;
    QApplication&       QtApplication;
    EditorProject&      project;
    EditorRenderer&     renderer;

    EditorScriptEngine& scriptEngine;

    QMenu*              fileMenu    = nullptr;
    QMenu*              importMenu  = nullptr;
    QMenu*              exportMenu  = nullptr;
    QMenu*              gadgetMenu  = nullptr;
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
