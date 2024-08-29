#pragma once
#include <ui_EditorMeshResourceForm.h>
#include <qdialog.h>
#include <ResourceBrowserWidget.h>
#include <qmainwindow.h>

/************************************************************************************************/

class EditorMeshResourceViewer : public QWidget
{
	Q_OBJECT

public:
	EditorMeshResourceViewer(FlexKit::Resource_ptr resource, QWidget* parent = nullptr);

	void Apply();
	void Cancel();

	void UpdateUI();
	void UpdateSubMeshUI();
	void UpdateAttributes();

	void ItemChanged(QTableWidgetItem* item);

	FlexKit::Resource_ptr	resource;
	QDockWidget*			docklet = nullptr;
	Ui_DockWidget			ui;
};


/************************************************************************************************/


struct MeshResourceViewer : public IResourceViewer
{
	MeshResourceViewer(QMainWindow* parentWindow);

	void operator () (FlexKit::Resource_ptr resource) override;

	QMainWindow* mainWindow;
};



/**********************************************************************

Copyright (c) 2023 Robert May

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
