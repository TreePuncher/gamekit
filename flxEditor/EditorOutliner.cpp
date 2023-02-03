#include "PCH.h"
#include "EditorOutliner.h"
#include "EditorViewport.h"
#include "EditorInspectors.h"
#include "EditorUndoRedo.h"

#include <chrono>
#include <qtimer.h>
#include <QtWidgets/qmenubar.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qmimedata.h>


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

	uint64_t GetID() const
	{
		return viewportObject->objectID;
	}

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

	void ClearChildren(ViewportScene& scene)
	{
		const auto end = childCount();

		auto children = takeChildren();
		for (int itr = 0; itr < end; itr++)
		{
			auto currentItem = static_cast<HierarchyItem*>(children[itr]);
			currentItem->ClearChildren(scene);

			scene.RemoveObject(currentItem->viewportObject);
			delete currentItem;
		}
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
			struct OutlinerContext : public ComponentConstructionContext
			{
				OutlinerContext(ViewportScene& IN_scene, ViewportGameObject_ptr& IN_object)
				: scene	{ IN_scene }
				, object{ IN_object }{}

				void AddToScene(FlexKit::GameObject& go) final
				{
					scene.scene.AddGameObject(go);
				}

				FlexKit::LayerHandle GetSceneLayer() final
				{
					return scene.physicsLayer;
				}

				uint64_t GetEditorIdentifier() final
				{
					return object->objectID;
				}

				ViewportScene&				scene;
				ViewportGameObject_ptr&		object;
			} ctx{ scene, viewportObject };

			if (!viewportObject->gameObject.hasView(FlexKit::TransformComponentID))
				TransformComponentFactory::ConstructNode(*viewportObject, ctx);
			else if (!newParent->viewportObject->gameObject.hasView(FlexKit::TransformComponentID))
				TransformComponentFactory::ConstructNode(*newParent->viewportObject, ctx);

			FlexKit::SetParentNode(
				FlexKit::GetSceneNode(newParent->viewportObject->gameObject),
				FlexKit::GetSceneNode(viewportObject->gameObject));

			newParent->addChild(this);
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
		auto idx		= indexAt(event->pos());
		auto item		= itemAt(event->pos());
		auto parent		= static_cast<HierarchyItem*>(item->parent());
		auto oldParent	= draggedItem->parent();

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
		auto idx		= indexAt(event->pos());
		auto item		= itemAt(event->pos());
		auto parent		= static_cast<HierarchyItem*>(item->parent());
		auto oldParent	= draggedItem->parent();

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
		auto item	= static_cast<HierarchyItem*>(itemAt(event->pos()));
		auto idx	= indexAt(event->pos());

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

	auto& scene = *editorViewport.GetScene();

	auto AddAtTopLevel =
		[&](auto& obj)
		{
			auto item = new HierarchyItem(obj);
			widgetMap.insert({ obj->objectID, item });

			addTopLevelItem(item);
		};

	for (auto& pending : editorViewport.GetScene()->markedForDeletion)
	{
		if (auto res = widgetMap.find(pending); res != widgetMap.end())
		{
			auto widget = res->second;
			widget->ClearChildren(scene);
			removeItemWidget(widget, 0);

			widgetMap.erase(res);
			delete widget;
		}
	}

	for (auto& obj : editorViewport.GetScene()->sceneObjects)
	{
		if (auto res = widgetMap.find(obj->objectID); res == widgetMap.end())
		{
			if (obj->gameObject.hasView(FlexKit::TransformComponentID))
			{
				auto node = FlexKit::GetParentNode(obj->gameObject);

				if (node == FlexKit::InvalidHandle)
				{
					AddAtTopLevel(obj);
					continue;
				}

				auto parentObj = scene.FindObject(FlexKit::GetParentNode(obj->gameObject));
				if (parentObj)
				{

					if (auto widget = GetWidget(parentObj->objectID); widget)
					{
						auto item = new HierarchyItem(obj);
						widgetMap[obj->objectID] = item;
						item->SetParent(widget, scene, item->childCount());
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
	QWidget		{ parent },
	viewport	{ IN_viewport },
	treeWidget	{ IN_viewport }
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

	viewport.sceneChangeSlot.Connect(sceneChangeSlot,
		[&]
		{
			viewport.GetScene()->OnSceneChange.Connect(sceneChangeSlot, [&] { Update();	});
		});
}


/************************************************************************************************/


SceneOutliner::~SceneOutliner()
{
}


/************************************************************************************************/


void SceneOutliner::Update()
{
	treeWidget.UpdateLabels();
	treeWidget.update();
	treeWidget.repaint();

	timer->stop();
	timer->start(500ms);
}


/************************************************************************************************/


void SceneOutliner::on_clicked()
{
	auto& selectionCtx	= viewport.GetSelectionContext();
	HierarchyItem* selectedItem			= static_cast<HierarchyItem*>(treeWidget.currentItem());

	ViewportSelection selection;
	selection.scene = viewport.GetScene().get();
	selection.viewportObjects.clear();
	selection.viewportObjects.push_back(selectedItem->viewportObject);

	uint64_t		prevObjId = 0xffffffffffffffff;
	HierarchyItem*	prevItem = nullptr;

	if (selectionCtx.type == ViewportObjectList_ID)
	{
		prevObjId = selectionCtx.GetSelection<ViewportSelection>().viewportObjects.front()->objectID;
	}

	if (GetCurrentState().userID != selectedItem->GetID())
	{
		ObjectState state{
			.stateID	= GetTypeGUID(SelectionOP),
			.userID		= selectedItem->GetID(),

			.undo =
				[this, prev = selectionCtx.selection, prevID = selectionCtx.type, prevItem = selectedItem, prevObjId]()
				{
					auto& selectionCtx = viewport.GetSelectionContext();

					selectionCtx.Clear(false);
					selectionCtx.selection	= prev;
					selectionCtx.type		= prevID;
					selectionCtx.OnChange();

					if (prevObjId != 0xffffffffffffffff)
					{
						auto item = treeWidget.GetWidget(prevObjId);
						treeWidget.setCurrentItem(item);
					}
				},

			.redo =
				[this, selection, objectID = selectedItem->viewportObject->objectID]()
				{
					auto& selectionCtx = viewport.GetSelectionContext();

					selectionCtx.Clear(false);
					selectionCtx.selection	= selection;
					selectionCtx.type		= ViewportObjectList_ID;
					selectionCtx.OnChange();

					auto item = treeWidget.GetWidget(objectID);
					treeWidget.setCurrentItem(item);
				}
		};

		PushState(std::move(state));
	}

	selectionCtx.Clear();
	selectionCtx.selection	= std::move(selection);
	selectionCtx.type		= ViewportObjectList_ID;
}


/************************************************************************************************/


HierarchyItem* SceneOutliner::CreateObject(HierarchyItem* parent) noexcept
{
	auto& scene = viewport.GetScene();
	if (!scene)
		return nullptr;

	auto obj	= scene->CreateObject(parent != nullptr ? parent->viewportObject.get() : nullptr);
	auto widget = treeWidget.widgetMap[obj->objectID];

	return widget;
}


/************************************************************************************************/


HierarchyItem* SceneOutliner::CreatePointLight() noexcept
{
	auto& scene = viewport.GetScene();
	if (!scene)
		return nullptr;

	auto obj = CreateObject();

	struct OutlinerContext : public ComponentConstructionContext
	{
		OutlinerContext(ViewportScene& IN_scene, ViewportGameObject& IN_object)
			: scene		{ IN_scene  }
			, object	{ IN_object } {}

		void AddToScene(FlexKit::GameObject& go) final
		{
			scene.scene.AddGameObject(go);
		}

		FlexKit::LayerHandle GetSceneLayer() final
		{
			return scene.physicsLayer;
		}

		uint64_t GetEditorIdentifier() final
		{
			return object.objectID;
		}

		ViewportScene&		scene;
		ViewportGameObject&	object;
	} ctx{ *scene, *obj->viewportObject };

	PointLightFactory::ConstructPointLight(*obj->viewportObject, ctx);

	return obj;
}


/************************************************************************************************/


void SceneOutliner::ShowContextMenu(const QPoint& point)
{
	auto index			= treeWidget.indexAt(point);

	if (index.isValid())
	{
		QMenu contextMenu(tr("Context menu"), this);

		auto removeAction = contextMenu.addAction("Remove",
			[&, row = index.row()]()
			{
				viewport.ClearMode(); // Mode needs to be cleared since deleting actively selected object may cause a crash
				viewport.ClearSelection();

				auto currentItem = static_cast<HierarchyItem*>(treeWidget.currentItem());
				currentItem->ClearChildren(*viewport.GetScene());

				viewport.GetScene()->RemoveObject(currentItem->viewportObject);
				treeWidget.removeItemWidget(currentItem, 0);

				delete currentItem;
			});
		auto addChildAction = contextMenu.addAction("Create Child",
			[&, point]()
			{
				if (viewport.GetScene() == nullptr)
					return;

				auto index			= treeWidget.indexAt(point);
				auto parentWidget	= (HierarchyItem*)treeWidget.currentItem();

				auto& scene			= *viewport.GetScene();
				auto newItem		= CreateObject(parentWidget);
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
