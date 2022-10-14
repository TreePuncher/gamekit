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
#include <chrono>

#include "ResourceBrowserWidget.h"
#include "EditorImport.h"
#include "EditorGadgetInterface.h"
#include "EditorViewport.h"
#include "EditorTextureViewer.h"
#include "SelectionContext.h"

#include "ModelViewerWidget.h"
#include "EditorInspectorView.h"

/************************************************************************************************/

class TextureResource;
class EditorScriptEngine;
class EditorPrefabEditor;
class EditorProject;
class EditorViewport;
class EditorRenderer;

/************************************************************************************************/


using FlexKit::ResourceList;
using high_resolution_clock = std::chrono::high_resolution_clock;
using time_point            = high_resolution_clock::time_point;
using duration_t            = std::chrono::duration<double>;


/************************************************************************************************/

class EditorMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	EditorMainWindow(EditorRenderer& IN_renderer, EditorScriptEngine& scriptEngine, EditorProject& IN_project, QApplication& application, QWidget *parent = Q_NULLPTR);
	~EditorMainWindow();


	void					AddImporter(iEditorImportor* importer);
	void					AddExporter(iEditorExporter* exporter);

	void					AddEditorView();
	void					AddInspector();
	void					AddOutputView();
	void					AddModelViewer();
	ResourceBrowserWidget*	AddResourceList();
	void					AddSceneOutliner();
	TextureViewer*			AddTextureViewer(TextureResource* res = nullptr);
	QTextEdit*				AddTextView();
	DXRenderWindow*			AddViewPort();

	void AddFileAction(const std::string& name, auto&& callable)
	{
		auto action = fileMenu->addAction(name.c_str());
		connect(action, &QAction::triggered, this, callable);
	}

	EditorViewport&			Get3DView()			{ return *viewport; }
	SelectionContext&		GetSelectionCtx()	{ return selectionContext; };
	EditorPrefabEditor*		GetPrefabEditor()	{ return prefabEditor; };


	void RegisterGadget(iEditorGadget* gadget);
	
	void CloseEvent(QCloseEvent* event);
	void Close();
	void timerEvent(QTimerEvent* event);
	void showEvent(QShowEvent* event) override {}

	QMenu* GetFileMenu() { return fileMenu; }

public slots:

	void Update();


signals:
	void onClose();

private:
	time_point          frameEnd;
	time_point          frameBegin;

	SelectionContext    selectionContext;
	EditorScriptEngine& scriptEngine;

	QTabWidget*             tabBar;
	EditorViewport*         viewport;
	QApplication&           QtApplication;
	EditorProject&          project;
	EditorRenderer&         renderer;

	EditorPrefabEditor*     prefabEditor;

	QMenu*              fileMenu    = nullptr;
	QMenu*              editMenu    = nullptr;
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
