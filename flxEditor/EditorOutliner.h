#pragma once

#include <QWidget>
#include "ui_SceneOutliner.h"
#include "ViewportScene.h"
#include <QStringListModel>
#include <qtreewidget.h>
#include <qevent.h>
#include <unordered_map>
#include "Signal.h"

/************************************************************************************************/


class EditorViewport;
class HierarchyItem;
class QTimer;

class SceneTreeWidget : public QTreeWidget
{
public:
	SceneTreeWidget(EditorViewport&);

	virtual void dragEnterEvent(QDragEnterEvent* event) override;
	virtual void dropEvent(QDropEvent* event) override;

	void			UpdateLabels();
	HierarchyItem*	GetWidget(uint64_t);

	std::unordered_map<uint64_t, HierarchyItem*>	widgetMap;
	HierarchyItem*									draggedItem = nullptr;
	EditorViewport&									editorViewport;
};

class SceneOutliner : public QWidget
{
	Q_OBJECT

public:
	SceneOutliner(EditorViewport&, QWidget *parent = Q_NULLPTR);
	~SceneOutliner();

public slots:
	void ShowContextMenu(const QPoint&);

private:
	HierarchyItem* CreateObject() noexcept;
	HierarchyItem* CreatePointLight() noexcept;

	void on_clicked();
	void Update();

	Ui::SceneOutliner				ui;
	SceneTreeWidget					treeWidget;
	EditorViewport&					viewport;
	QTimer*							timer;

	FlexKit::Signal<void()>::Slots	sceneChangeSlot;
};


/************************************************************************************************/

/**********************************************************************

Copyright (c) 2022 Robert May

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
