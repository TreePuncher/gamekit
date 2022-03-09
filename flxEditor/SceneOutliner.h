#pragma once

#include <QWidget>
#include "ui_SceneOutliner.h"
#include "ViewportScene.h"
#include <QStringListModel>
#include <qtreewidget.h>
#include <qevent.h>
#include <unordered_map>


/************************************************************************************************/


class EditorViewport;
class QTimer;
class HierarchyItem;

class SceneTreeWidget : public QTreeWidget
{
public:
    SceneTreeWidget(EditorViewport&);

    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;

    void            UpdateLabels();
    HierarchyItem*  GetWidget(uint64_t);

    std::unordered_map<uint64_t, HierarchyItem*>    widgetMap;
    HierarchyItem*                                  draggedItem = nullptr;
    EditorViewport&                                 editorViewport;
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

	Ui::SceneOutliner                   ui;
    SceneTreeWidget                     treeWidget;
    EditorViewport&                     viewport;
    QTimer*                             timer;
};


/************************************************************************************************/
