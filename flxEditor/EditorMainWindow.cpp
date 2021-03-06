#include "EditorMainWindow.h"
#include "DXRenderWindow.h"
#include "EditorCodeEditor.h"
#include "EditorOutputWindow.h"

#include <QtWidgets/qmenubar.h>
#include <chrono>


using namespace std::chrono_literals;


/************************************************************************************************/


EditorMainWindow::EditorMainWindow(EditorRenderer& IN_renderer, EditorScriptEngine& IN_scriptEngine, EditorProject& IN_project, QApplication& IN_application, QWidget* parent) :
    QMainWindow     { parent            },
    QtApplication   { IN_application    },
    project         { IN_project        },
    renderer        { IN_renderer       },
    scriptEngine    { IN_scriptEngine   }
{
    resize({ 800, 600 });
    fileMenu       = menuBar()->addMenu("File");
    importMenu     = fileMenu->addMenu("Import");
    exportMenu     = fileMenu->addMenu("Export");

    auto quitAction = fileMenu->addAction("Quit");
    quitAction->setShortcut(tr("CTRL+Q"));
    connect(quitAction, &QAction::triggered, this, &EditorMainWindow::Close, Qt::QueuedConnection);

    auto viewMenu   = menuBar()->addMenu("View");
    auto Add3DView  = viewMenu->addAction("Add 3D AddViewPort");
    connect(Add3DView, &QAction::triggered, this, &EditorMainWindow::AddViewPort);

    auto AddTextView = viewMenu->addAction("Add Text View");
    connect(AddTextView, &QAction::triggered, this, &EditorMainWindow::AddTextView);

    auto addResourceView = viewMenu->addAction("Add Resource View");
    connect(addResourceView, &QAction::triggered, this, &EditorMainWindow::AddResourceList);

    auto addTextureView = viewMenu->addAction("Add Texture View");
    connect(addTextureView, &QAction::triggered, this, [&] { AddTextureViewer(); });

    auto addCodeEditor = viewMenu->addAction("Add Code Editor");
    connect(addCodeEditor, &QAction::triggered, this, [&] { AddEditorView(); });

    auto addTextOutput = viewMenu->addAction("Add text Output");
    connect(addTextOutput, &QAction::triggered, this, [&] { AddOutputView(); });

    auto tools = menuBar()->addMenu("Tools");
    gadgetMenu = tools->addMenu("Scripts");

    auto timer = new QTimer{ this };
    connect(timer, &QTimer::timeout, this, &EditorMainWindow::Update);
    timer->setTimerType(Qt::PreciseTimer);
    timer->start(1ms);

    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks);
    tabPosition(Qt::TopDockWidgetArea);

    show();
}


/************************************************************************************************/


QTextEdit* EditorMainWindow::AddTextView()
{
    auto textDocklet        = new QDockWidget{};
    QTextEdit* textEditor   = new QTextEdit{ textDocklet };

    textDocklet->setWindowTitle("TextEditor");
    textDocklet->setWidget(textEditor);
    textDocklet->setFloating(true);
    textDocklet->show();

    addDockWidget(Qt::LeftDockWidgetArea, textDocklet);

    return textEditor;
}


/************************************************************************************************/


void EditorMainWindow::AddEditorView()
{
    auto docklet = new QDockWidget{};
    EditorCodeEditor* textEditor    = new EditorCodeEditor{ scriptEngine ,docklet };

    docklet->setWindowTitle("Code Editor");
    docklet->setWidget(textEditor);
    docklet->show();

    addDockWidget(Qt::LeftDockWidgetArea, docklet);
}


/************************************************************************************************/


void EditorMainWindow::AddOutputView()
{
    auto docklet = new QDockWidget{};
    auto* output = new EditorOutputWindow{ docklet };

    docklet->setWindowTitle("Output");
    docklet->setWidget(output);
    docklet->show();
    
    output->SetInputSource([&]() -> const std::string& { return scriptEngine.GetTextBuffer(); }, 0);

    addDockWidget(Qt::RightDockWidgetArea, docklet);
}


/************************************************************************************************/


void EditorMainWindow::AddImporter(iEditorImportor* importer)
{
    auto importAction = importMenu->addAction(importer->GetFileTypeName().c_str());

    connect(importAction, &QAction::triggered, this,
        [=]
        {
            const auto importText   = std::string{ "Import " } + importer->GetFileTypeName();
            const auto fileMenuText = std::string{ "Files (*." } + importer->GetFileExt() + ")";
            const auto fileDir      = QFileDialog::getOpenFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());
            const auto fileStr      = fileDir.toStdString();

            if (fileDir.size() && !importer->Import(fileStr))
            {   // Log Error
                FK_LOG_ERROR("Import Failed!");
            }
        });
}


void EditorMainWindow::AddExporter(iEditorExporter* exporter)
{
    auto importAction = exportMenu->addAction(exporter->GetFileTypeName().c_str());

    connect(importAction, &QAction::triggered, this,
        [=]
        {
            const auto importText   = std::string{ "Export" } + exporter->GetFileTypeName();
            const auto fileMenuText = std::string{ "Files (*." } + exporter->GetFileExt() + ")";
            const auto fileDir      = QFileDialog::getSaveFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());
            const auto fileStr      = fileDir.toStdString();

            auto selectedResources  = selectionContext.GetSelectedResources().value_or(ResourceList{});

            if(!selectedResources.size())
                selectedResources = project.GetResources();

            if (fileDir.size() && selectedResources.size() && !exporter->Export(fileStr, selectedResources))
            {   // Log Error
                FK_LOG_ERROR("Export Failed!");
            }
        });
}


/************************************************************************************************/


void EditorMainWindow::RegisterGadget(iEditorGadget* gadget)
{
    auto gadgetID = gadget->GadgetID();
    auto action = gadgetMenu->addAction(QString(gadgetID.c_str()));

    connect(
        action,
        &QAction::triggered, this,
        [=]
        {
            gadget->Execute();
        });
}


/************************************************************************************************/


DXRenderWindow* EditorMainWindow::AddViewPort()
{
    auto viewPortWidget = renderer.CreateRenderWindow();

    viewPortWidget->ResizeEventHandler =
        [&](DXRenderWindow* renderWindow)
        {
            //renderer.DrawRenderWindow(renderWindow);
        };

    auto docklet = new QDockWidget{};
    docklet->setWidget(viewPortWidget);
    docklet->show();

    addDockWidget(Qt::LeftDockWidgetArea, docklet);

    return viewPortWidget;
}


/************************************************************************************************/


ResourceBrowserWidget* EditorMainWindow::AddResourceList()
{
    auto docklet            = new QDockWidget{ this };
    auto resourceBrowser    = new ResourceBrowserWidget(project, renderer, this);

    resourceBrowser->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    docklet->setWindowTitle("Resource List");
    docklet->setWidget(resourceBrowser);
    docklet->show();

    addDockWidget(Qt::LeftDockWidgetArea, docklet);

    return resourceBrowser;
}


/************************************************************************************************/


void EditorMainWindow::AddModelViewer()
{
    auto docklet        = new QDockWidget{ this };
    auto modelViewer    = new ModelViewerWidget(renderer, this);

    modelViewer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    docklet->setWindowTitle("Model Viewer");
    docklet->setWidget(modelViewer);
    docklet->show();

    addDockWidget(Qt::LeftDockWidgetArea, docklet);
}


/************************************************************************************************/


TextureViewer* EditorMainWindow::AddTextureViewer(TextureResource* res)
{
    auto docklet        = new QDockWidget{ this };
    auto textureViewer  = new TextureViewer{ renderer, nullptr };

    docklet->setWidget(textureViewer);
    docklet->setWindowTitle("TextureViewer");
    docklet->resize({800, 600});
    docklet->show();

    return textureViewer;
}


/************************************************************************************************/


void EditorMainWindow::CloseEvent(QCloseEvent* event)
{
    QSettings settings("Flex", "Kit");
}


/************************************************************************************************/


void EditorMainWindow::Close()
{
    QtApplication.closeAllWindows();
    QtApplication.quit();
    QtApplication.exit();
}


/************************************************************************************************/


void EditorMainWindow::timerEvent(QTimerEvent* event)
{
    OutputDebugString(L"Hello!\n");
}


/************************************************************************************************/


EditorMainWindow::~EditorMainWindow()
{
}


/************************************************************************************************/


void EditorMainWindow::Update()
{
    //OutputDebugString(L"Update!\n");
    renderer.DrawOneFrame();
}


/**********************************************************************

Copyright (c) 2019-2021 Robert May

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
