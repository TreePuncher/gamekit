#include "PCH.h"
#include "EditorMainWindow.h"
#include "DXRenderWindow.h"

#include "EditorProject.h"
#include "EditorPrefabEditor.h"
#include "EditorCodeEditor.h"
#include "EditorOutputWindow.h"
#include "EditorOutliner.h"
#include "EditorViewport.h"
#include "EditorRenderer.h"
#include "EditorUndoRedo.h"

#include <chrono>
#include <QShortcut.h>
#include <QtWidgets/qmenubar.h>


using namespace std::chrono_literals;


/************************************************************************************************/


EditorMainWindow::EditorMainWindow(EditorRenderer& IN_renderer, EditorScriptEngine& IN_scriptEngine, EditorProject& IN_project, QApplication& IN_application, QWidget* parent) :
	QMainWindow		{ parent            },
	QtApplication	{ IN_application    },
	prefabEditor	{ new EditorPrefabEditor{ selectionContext, IN_scriptEngine, IN_renderer, IN_project } },
	project			{ IN_project        },
	renderer		{ IN_renderer       },
	scriptEngine	{ IN_scriptEngine   },
	viewport		{ new EditorViewport{ IN_renderer, selectionContext, this } },
	tabBar			{ new QTabWidget{} }
{
	fileMenu	= menuBar()->addMenu("File");
	editMenu	= menuBar()->addMenu("Edit");
	importMenu	= fileMenu->addMenu("Import");
	exportMenu	= fileMenu->addMenu("Export");

	tabBar->addTab(viewport, "Scene");
	tabBar->addTab(prefabEditor, "Prefab");

	setCentralWidget(tabBar);

	auto viewMenu	= menuBar()->addMenu("View");
	auto Add3DView	= viewMenu->addAction("Add 3D AddViewPort");
	connect(Add3DView, &QAction::triggered, this, &EditorMainWindow::AddViewPort);

	auto AddTextView = viewMenu->addAction("Add Text View");
	connect(AddTextView, &QAction::triggered, this, &EditorMainWindow::AddTextView);

	auto addResourceView = viewMenu->addAction("Add Resource List");
	connect(addResourceView, &QAction::triggered, this, &EditorMainWindow::AddResourceList);

	auto addTextureView = viewMenu->addAction("Add Texture View");
	connect(addTextureView, &QAction::triggered, this, [&] { AddTextureViewer(); });

	auto addCodeEditor = viewMenu->addAction("Add Code Editor");
	connect(addCodeEditor, &QAction::triggered, this, [&] { AddEditorView(); });

	auto addTextOutput = viewMenu->addAction("Add text Output");
	connect(addTextOutput, &QAction::triggered, this, [&] { AddOutputView(); });

	auto addInspector = viewMenu->addAction("Add Inspector");
	connect(addInspector, &QAction::triggered, this, [&] { AddInspector(); });

	auto addOutliner= viewMenu->addAction("Add Scene Outliner");
	connect(addOutliner, &QAction::triggered, this, [&] { AddSceneOutliner(); });

	auto tools = menuBar()->addMenu("Tools");
	gadgetMenu = tools->addMenu("Scripts");

	auto undo = editMenu->addAction("Undo");
	connect(undo, &QAction::triggered, []() { Undo(); });

	auto redo = editMenu->addAction("Redo");
	connect(redo, &QAction::triggered, []() { Redo(); });

	auto undoHotKey = new QShortcut(QKeySequence(tr("Ctrl+Z", "Edit|Undo")), this);
	connect(undoHotKey, &QShortcut::activated, [&]() { Undo(); });

	auto redoHotKey = new QShortcut(QKeySequence(tr("Ctrl+Y", "Edit|Redo")), this);
	connect(redoHotKey, &QShortcut::activated, []() { Redo(); });

	auto timer = new QTimer{ this };
	connect(timer, &QTimer::timeout, this, &EditorMainWindow::Update);
	timer->setTimerType(Qt::PreciseTimer);
	timer->start(16ms);

	setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks | QMainWindow::VerticalTabs);
	tabPosition(Qt::TopDockWidgetArea);

	this->setMaximumHeight(3000);
	this->setMaximumWidth(6000);

	showMaximized();

	QTimer::singleShot(0, this, SLOT(showMaximized()));

	frameEnd	= high_resolution_clock::now();
	frameBegin	= frameEnd;
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

	addDockWidget(Qt::RightDockWidgetArea, textDocklet);

	return textEditor;
}


/************************************************************************************************/


void EditorMainWindow::AddEditorView()
{
	auto docklet = new QDockWidget{};
	EditorCodeEditor* textEditor = new EditorCodeEditor{ project, scriptEngine ,docklet };

	docklet->setWindowTitle("Code Editor");
	docklet->setWidget(textEditor);
	docklet->show();

	addDockWidget(Qt::RightDockWidgetArea, docklet);
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
			const auto importText	= std::string{ "Import " } + importer->GetFileTypeName();
			const auto fileMenuText	= std::string{ "Files (*." } + importer->GetFileExt() + ")";
			const auto fileDir		= QFileDialog::getOpenFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());
			const auto fileStr		= fileDir.toStdString();

			if (fileDir.size() && !importer->Import(fileStr))
			{   // Log Error
				FK_LOG_ERROR("Import Failed!");
			}
		});
}


void EditorMainWindow::AddExporter(iEditorExporter* exporter)
{
	auto exportAction = exportMenu->addAction(exporter->GetFileTypeName().c_str());

	connect(exportAction, &QAction::triggered, this,
		[=]
		{
			const auto exportLabel	= std::string{ "Export" } + exporter->GetFileTypeName();
			const auto fileMenuText	= std::string{ "Files (*." } + exporter->GetFileExt() + ")";
			const auto fileDir		= QFileDialog::getSaveFileName(this, tr(exportLabel.c_str()), QDir::currentPath(), fileMenuText.c_str());
			const auto fileStr		= fileDir.toStdString();

			std::vector<FlexKit::Resource_ptr> selectedResources{};

			for (auto& resource : project.GetResources())
					selectedResources.emplace_back(resource);

			std::sort(
				std::begin(selectedResources),
				std::end(selectedResources),
				[](FlexKit::Resource_ptr& lhs, FlexKit::Resource_ptr& rhs) { return lhs->GetResourceGUID() < rhs->GetResourceGUID(); });

			selectedResources.erase(
				std::unique(
					std::begin(selectedResources),
					std::end(selectedResources)),
				std::end(selectedResources));

			if (fileDir.size() && selectedResources.size() && !exporter->Export(fileStr, selectedResources))
			{   // Log Error
				const auto fileDir = QFileDialog::getOpenFileName(this, tr(exportLabel.c_str()), QDir::currentPath(), fileMenuText.c_str());
				const auto fileStr = fileDir.toStdString();

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
	docklet->setFeatures(docklet->features() | QDockWidget::DockWidgetFeature::DockWidgetVerticalTitleBar);
	docklet->show();

	addDockWidget(Qt::RightDockWidgetArea, docklet, Qt::Orientation::Vertical);

	return viewPortWidget;
}


/************************************************************************************************/


ResourceBrowserWidget* EditorMainWindow::AddResourceList()
{
	auto docklet			= new QDockWidget{ this };
	auto resourceBrowser	= new ResourceBrowserWidget(project, renderer, this);

	resourceBrowser->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	docklet->setWindowTitle("Resource List");
	docklet->setWidget(resourceBrowser);
	docklet->setFeatures(docklet->features() | QDockWidget::DockWidgetFeature::DockWidgetVerticalTitleBar);
	docklet->show();

	addDockWidget(Qt::LeftDockWidgetArea, docklet, Qt::Orientation::Vertical);

	return resourceBrowser;
}


/************************************************************************************************/


void EditorMainWindow::AddSceneOutliner()
{
	auto docklet    = new QDockWidget{ this };
	auto outliner   = new SceneOutliner{ Get3DView(), this};

	docklet->setWindowTitle("Outliner");
	docklet->setWidget(outliner);
	docklet->setFeatures(docklet->features() | QDockWidget::DockWidgetFeature::DockWidgetVerticalTitleBar);
	docklet->show();

	addDockWidget(Qt::LeftDockWidgetArea, docklet, Qt::Orientation::Vertical);
}


/************************************************************************************************/


void EditorMainWindow::AddModelViewer()
{
	auto docklet        = new QDockWidget{ this };
	auto modelViewer    = new ModelViewerWidget(renderer, this);

	modelViewer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	docklet->setWindowTitle("Model Viewer");
	docklet->setWidget(modelViewer);
	docklet->setFeatures(docklet->features() | QDockWidget::DockWidgetFeature::DockWidgetVerticalTitleBar);
	docklet->setFloating(true);
	docklet->show();
}


/************************************************************************************************/


TextureViewer* EditorMainWindow::AddTextureViewer(TextureResource* res)
{
	auto docklet        = new QDockWidget{ this };
	auto textureViewer  = new TextureViewer{ renderer, this };

	docklet->setWidget(textureViewer);
	docklet->setWindowTitle("TextureViewer");
	docklet->setFeatures(docklet->features() | QDockWidget::DockWidgetFeature::DockWidgetVerticalTitleBar);
	docklet->setFloating(true);
	docklet->show();
	
	return textureViewer;
}


/************************************************************************************************/


void EditorMainWindow::AddInspector()
{
	auto docklet    = new QDockWidget{ this };
	auto inspector  = new EditorInspectorView{ selectionContext, this };

	docklet->setWidget(inspector);
	docklet->setWindowTitle("Inspector");
	docklet->setFeatures(docklet->features() | QDockWidget::DockWidgetFeature::DockWidgetVerticalTitleBar);
	docklet->show();

	addDockWidget(Qt::RightDockWidgetArea, docklet, Qt::Orientation::Vertical);
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
	duration_t lastFrameDuration = frameEnd - frameBegin;

	renderer.DrawOneFrame(lastFrameDuration.count());

	frameBegin  = frameEnd;
	frameEnd    = high_resolution_clock::now();
}


/**********************************************************************

Copyright (c) 2019-2022 Robert May

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
