#include "PCH.h"
#include "SceneOutliner.h"
#include "EditorViewport.h"
#include "SceneInspectors.h"

#include <chrono>
#include <qtimer.h>
#include <QtWidgets/qmenubar.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qmimedata.h>

using namespace std::chrono_literals;

/************************************************************************************************/


class HierarchyItem : public QTreeWidgetItem
{
public:
    HierarchyItem() : QTreeWidgetItem{}
    {
        setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled);
    }

    HierarchyItem(ViewportGameObject_ptr _ptr) :
        QTreeWidgetItem{},
        viewportObject{ _ptr }
    {
        setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled);
    }

    ~HierarchyItem() {}

    ViewportGameObject_ptr viewportObject = std::make_shared<ViewportGameObject>();

    void Update()
    {
        std::string ID;
        if (viewportObject->gameObject.hasView(FlexKit::StringComponentID))
            ID = FlexKit::GetStringID(viewportObject->gameObject);
        else
            ID = fmt::format("obj.{}", viewportObject->objectID);

        setData(0, Qt::DisplayRole, { ID.c_str() });
    }

    void ClearParent()
    {
        if (auto currentParent = parent(); currentParent)
            currentParent->removeChild(this);

        FlexKit::SetParentNode(
            FlexKit::NodeHandle{ 0 },
            FlexKit::GetSceneNode(viewportObject->gameObject));
    }

    void SetParent(HierarchyItem* newParent, ViewportScene& scene, int idx)
    {
        auto currentParent  = parent();

        if (currentParent && currentParent != newParent)
        {
            FlexKit::SetParentNode(
                FlexKit::NodeHandle{ 0 },
                FlexKit::GetSceneNode(viewportObject->gameObject));

            currentParent->removeChild(this);
        }

        if (newParent && newParent != parent())
        {
            if (!viewportObject->gameObject.hasView(FlexKit::TransformComponentID))
                TransformComponentFactory::ConstructNode(*viewportObject, scene);

            if (!newParent->viewportObject->gameObject.hasView(FlexKit::TransformComponentID))
                TransformComponentFactory::ConstructNode(*newParent->viewportObject, scene);

            FlexKit::SetParentNode(
                FlexKit::GetSceneNode(newParent->viewportObject->gameObject),
                FlexKit::GetSceneNode(viewportObject->gameObject));

            newParent->insertChild(idx, this);
        }
    }
};

/************************************************************************************************/


SceneTreeWidget::SceneTreeWidget(EditorViewport& IN_viewport) :
    editorViewport{ IN_viewport }
{
    header()->close();

    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
}


/************************************************************************************************/


void SceneTreeWidget::dragEnterEvent(QDragEnterEvent* event) 
{
    QModelIndex droppedIndex = indexAt(event->pos());

    if (!droppedIndex.isValid())
        return;

    draggedItem = static_cast<HierarchyItem*>(currentItem());
    event->acceptProposedAction();

    QTreeWidget::dragEnterEvent(event);
}


/************************************************************************************************/


void SceneTreeWidget::dropEvent(QDropEvent* event)
{
    switch (dropIndicatorPosition())
    {
    case QAbstractItemView::DropIndicatorPosition::AboveItem:
    {
        auto idx        = indexAt(event->pos());
        auto item       = itemAt(event->pos());
        auto parent     = static_cast<HierarchyItem*>(item->parent());
        auto oldParent  = draggedItem->parent();

        if (oldParent)
            oldParent->removeChild(draggedItem);
        else
            removeItemWidget(draggedItem, 0);

        if (!parent)
            insertTopLevelItem(idx.row(), draggedItem);
        else
            draggedItem->SetParent(parent, *editorViewport.GetScene(), idx.row());
    }   break;
    case QAbstractItemView::DropIndicatorPosition::BelowItem:
    {
        auto idx        = indexAt(event->pos());
        auto item       = itemAt(event->pos());
        auto parent     = static_cast<HierarchyItem*>(item->parent());
        auto oldParent  = draggedItem->parent();

        if (oldParent)
            oldParent->removeChild(draggedItem);
        else
            removeItemWidget(draggedItem, 0);

        if (!parent)
            insertTopLevelItem(idx.row() + 1, draggedItem);
        else
            draggedItem->SetParent(parent, *editorViewport.GetScene(), idx.row() + 1);
    }   break;
    case QAbstractItemView::DropIndicatorPosition::OnItem:
    {
        auto item   = static_cast<HierarchyItem*>(itemAt(event->pos()));
        auto idx    = indexAt(event->pos());

        if (item)
        {
            draggedItem->SetParent(item, *editorViewport.GetScene(), idx.row());
        }
        else
        {
            draggedItem->ClearParent();
            if (idx.isValid())
                insertTopLevelItem(idx.row(), draggedItem);
            else
                addTopLevelItem(draggedItem);
        }
    }   break;
    case QAbstractItemView::DropIndicatorPosition::OnViewport:
    {

    }   break;
    }
    draggedItem = nullptr;
    QTreeWidget::dropEvent(event);
}


/************************************************************************************************/


void SceneTreeWidget::UpdateLabels()
{
    if (!editorViewport.GetScene() || draggedItem)
        return;

    auto& scene = editorViewport.GetScene();

    auto AddAtTopLevel =
        [&](auto& obj)
        {
            auto item = new HierarchyItem(obj);
            widgetMap.insert({ obj->objectID, item });

            addTopLevelItem(item);
        };

    for (auto& obj : editorViewport.GetScene()->sceneObjects)
    {
        if (auto res = widgetMap.find(obj->objectID); res == widgetMap.end())
        {
            if (obj->gameObject.hasView(FlexKit::TransformComponentID))
            {
                auto node       = FlexKit::GetParentNode(obj->gameObject);

                if (node == FlexKit::NodeHandle{ 0 } || node == FlexKit::InvalidHandle_t)
                {
                    AddAtTopLevel(obj);
                    continue;
                }

                auto parentObj  = scene->FindObject(FlexKit::GetParentNode(obj->gameObject));
                if (parentObj)
                {

                    if (auto widget = GetWidget(parentObj->objectID); widget)
                    {
                        auto item = new HierarchyItem(obj);
                        widgetMap[obj->objectID] = item;
                        item->SetParent(widget, *scene, 0);
                    }
                }
                else
                    continue;
            }
            else
                AddAtTopLevel(obj);
        }
        else
        {
            auto& item = res->second;
            item->Update();
        }
    }
}


/************************************************************************************************/


HierarchyItem* SceneTreeWidget::GetWidget(uint64_t ID)
{
    if (auto res = widgetMap.find(ID); res != widgetMap.end())
        return res->second;
    else
        return nullptr;
}


/************************************************************************************************/


SceneOutliner::SceneOutliner(EditorViewport& IN_viewport, QWidget *parent) :
    QWidget     { parent },
    viewport    { IN_viewport },
    treeWidget  { IN_viewport }
{
	ui.setupUi(this);

    timer = new QTimer{ this };
    connect(timer, &QTimer::timeout, this, &SceneOutliner::Update);
    timer->setTimerType(Qt::PreciseTimer);
    timer->start(500ms);

    ui.verticalLayout->addWidget(&treeWidget);

    treeWidget.setEditTriggers(QAbstractItemView::EditTrigger::SelectedClicked);
    treeWidget.setContextMenuPolicy(Qt::CustomContextMenu);

    setAcceptDrops(true);

    connect(
        &treeWidget,
        &QTreeView::clicked,
        this,
        &SceneOutliner::on_clicked);

    connect(
        &treeWidget,
        SIGNAL(customContextMenuRequested(const QPoint&)),
        SLOT(ShowContextMenu(const QPoint&)));
}


/************************************************************************************************/


SceneOutliner::~SceneOutliner()
{
}


/************************************************************************************************/


void SceneOutliner::Update()
{
    treeWidget.UpdateLabels();
    timer->start(500ms);
}


/************************************************************************************************/


void SceneOutliner::on_clicked()
{
    HierarchyItem* selectedItem = static_cast<HierarchyItem*>(treeWidget.currentItem());

    auto& selectionCtx          = viewport.GetSelectionContext();
    ViewportSelection selection;

    selection.scene = viewport.GetScene().get();
    selection.viewportObjects.clear();
    selection.viewportObjects.push_back(selectedItem->viewportObject);

    selectionCtx.selection  = std::move(selection);
    selectionCtx.type       = ViewportObjectList_ID;
}


/************************************************************************************************/


HierarchyItem* SceneOutliner::CreateObject() noexcept
{
    auto& scene = viewport.GetScene();
    if (!scene)
        return nullptr;

    HierarchyItem* item = new HierarchyItem(scene->CreateObject());
    treeWidget.widgetMap[item->viewportObject->objectID] = item;

    auto ID = fmt::format("obj.{}", item->viewportObject->objectID);
    item->setData(0, Qt::DisplayRole, ID.c_str());

    treeWidget.addTopLevelItem(item);

    return item;
}


/************************************************************************************************/


HierarchyItem* SceneOutliner::CreatePointLight() noexcept
{
    auto& scene = viewport.GetScene();
    if (!scene)
        return nullptr;

    auto obj = CreateObject();

    PointLightFactory::ConstructPointLight(*obj->viewportObject, *scene);

    return obj;
}


/************************************************************************************************/


void SceneOutliner::ShowContextMenu(const QPoint& point)
{
    auto index = treeWidget.indexAt(point);

    if (index.isValid())
    {
        QMenu contextMenu(tr("Context menu"), this);

        auto removeAction = contextMenu.addAction("Remove",
            [&, row = index.row()]()
            {
                auto currentItem = static_cast<HierarchyItem*>(treeWidget.currentItem());

                viewport.ClearMode(); // Mode needs to be cleared since deleting actively selected object may cause a crash
                viewport.GetScene()->RemoveObject(currentItem->viewportObject);
                treeWidget.removeItemWidget(currentItem, 0);

                delete currentItem;
            });
        auto addChildAction = contextMenu.addAction("Create Child",
            [&]()
            {
                auto& scene = viewport.GetScene();

                if (scene == nullptr)
                    return;

                auto newItem = CreateObject();

                treeWidget.removeItemWidget(newItem, 0);
                treeWidget.currentItem()->addChild(newItem);

                HierarchyItem* parentItem = static_cast<HierarchyItem*>(treeWidget.currentItem());
                auto& parent = parentItem->viewportObject->gameObject;
                
                if (!parent.hasView(FlexKit::TransformComponentID))
                    parent.AddView<FlexKit::SceneNodeView<>>();

                scene->sceneObjects.push_back(newItem->viewportObject);
                auto& nodeView = newItem->viewportObject->gameObject.AddView<FlexKit::SceneNodeView<>>();

                nodeView.SetParentNode(FlexKit::GetSceneNode(parent));
            });

        contextMenu.exec(treeWidget.viewport()->mapToGlobal(point));
    }
    else
    {
        if (viewport.GetScene() == nullptr)
            return;

        QMenu contextMenu(tr("Context menu"), this);

        auto createObjectAction = contextMenu.addAction("Create Object",
            [&]() { CreateObject(); });

        auto createPointLightAction = contextMenu.addAction("Create PointLight",
            [&]() { CreatePointLight(); });

        contextMenu.exec(treeWidget.viewport()->mapToGlobal(point));
    }
}


/************************************************************************************************/
