#include "PCH.h"

#include <scn/scn.h>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/qdockwidget>
#include <QtWidgets/qmenubar.h>
#include <qshortcut>
#include <memory>

#include "ResourceBrowserWidget.h"
#include "TextureViewer.h"
#include "Assets.h"
#include "EditorRenderer.h"
#include "graphics.h"
#include "memoryutilities.h"
#include "EditorProject.h"

#include "TextureResourceUtilities.h"


/************************************************************************************************/


ResourceBrowserWidget::ResourceBrowserWidget(EditorProject& IN_project, EditorRenderer& IN_renderer, QWidget *parent) :
	QWidget		{ parent },
	renderer	{ IN_renderer },
	menuBar		{ new QMenuBar{ this } },
	timer		{ new QTimer(this) },
	project		{ IN_project }
{
	ui.setupUi(this);
	ui.verticalLayout->setMenuBar(menuBar);

	table = findChild<QTableWidget*>("tableWidget");
	table->setContextMenuPolicy(Qt::CustomContextMenu);
	table->setUpdatesEnabled(true);
	table->setColumnCount(4);
	table->setSelectionMode(QAbstractItemView::MultiSelection);

	connect(table, &QTableWidget::cellChanged, this, &ResourceBrowserWidget::OnCellChange);

	connect(timer, &QTimer::timeout, this, &ResourceBrowserWidget::Update);
	timer->setInterval(1000);
	timer->start();

	connect(
		table, &QTableWidget::customContextMenuRequested,
		this, &ResourceBrowserWidget::ShowContextMenu);

	auto createDropDown = menuBar->addMenu("Create");
	auto createScene    = createDropDown->addAction("Scene");

	createScene->connect(createScene, &QAction::triggered,
		[&]
		{
			auto scene			= std::make_shared<FlexKit::SceneResource>();
			auto editorScene	= std::make_shared<EditorScene>(scene);

			scene->ID				= "NewScene";
			editorScene->sceneName	= "NewScene";
			
			IN_project.AddScene(editorScene);
			IN_project.AddResource(std::static_pointer_cast<FlexKit::iResource>(scene));
		});

	auto* deleteHotKey = new QShortcut{ QKeySequence::Delete, this };

	connect(deleteHotKey, &QShortcut::activated,
		[this]()
		{
			RemoveSelectedItems();
		});
}


/************************************************************************************************/


ResourceBrowserWidget::~ResourceBrowserWidget()
{
}


/************************************************************************************************/


void ResourceBrowserWidget::Update()
{
	auto resourceCount = project.resources.size();
	table->setRowCount(resourceCount);

	static const std::map<ResourceID_t, const char*> IDTypeMap = {
		{ TextureResourceTypeID,	"Texture" },
		{ MeshResourceTypeID,		"Mesh" },
		{ SceneResourceTypeID,		"Scene" },
	};

	for (size_t I = 0; I < resourceCount; I++)
	{
		auto useCount   = project.resources[I].use_count();
		auto& resource  = project.resources[I]->resource;

		auto name			= table->item(I, 0);
		auto ID				= table->item(I, 1);
		auto userCount		= table->item(I, 2);
		auto resourceType	= table->item(I, 3);

		if (!name)
		{
			name			= new QTableWidgetItem;
			ID				= new QTableWidgetItem;
			userCount		= new QTableWidgetItem;
			resourceType	= new QTableWidgetItem;

			auto& idTxt = resource->GetResourceID();
			name->setText(idTxt.c_str());
			ID->setText(fmt::format("{}", resource->GetResourceGUID()).c_str());
			userCount->setText(fmt::format("{}", useCount).c_str());

			table->setItem(I, 0, name);
			table->setItem(I, 1, ID);
			table->setItem(I, 2, userCount);
			table->setItem(I, 3, resourceType);
		}
		else
		{
			auto& idTxt = resource->GetResourceID();
			name->setText(idTxt.c_str());
			ID->setText(fmt::format("{}", resource->GetResourceGUID()).c_str());
			userCount->setText(fmt::format("{}", useCount).c_str());

			if (auto res = IDTypeMap.find(resource->GetResourceTypeID()); res != IDTypeMap.end())
				resourceType->setText(res->second);
			else
				resourceType->setText("Unknown");
		}
	}

	table->update();

	timer->start();
}


/************************************************************************************************/


void ResourceBrowserWidget::OnCellChange(int row, int column)
{
	auto item = table->item(row, column);

	switch (column)
	{
	case 0:
	{
		auto name = item->text().toStdString();
		project.resources[row]->resource->SetResourceID(name);
	}   break;
	case 1:
	{
		uint64_t GUID;
		auto text = item->text().toStdString();
		if(scn::scan(text, "{}", GUID))
			project.resources[row]->resource->SetResourceGUID(GUID);

	}   break;
	case 2:
	{
	}   break;
	case 3:
	{
	}   break;
	default:
		return;
	}
}


/************************************************************************************************/


void ResourceBrowserWidget::RemoveSelectedItems()
{
	table = findChild<QTableWidget*>("tableWidget");

	std::vector<ProjectResource_ptr> removeList;

	for (auto* item : table->selectedItems())
	{
		auto& projRes = project.resources[item->row()];
		removeList.push_back(projRes);
	}

	for (auto&& res : removeList)
		project.RemoveResource(res->resource);

	table->clearSelection();
	table->update();
}


/************************************************************************************************/


void ResourceBrowserWidget::resizeEvent(QResizeEvent* evt) 
{
	QWidget::resizeEvent(evt);

	const auto widgetSize	= size();
	const auto width		= widgetSize.width();
	const auto height		= widgetSize.height();

	resize(widgetSize);

	auto child = findChild<QTableWidget*>("tableWidget");

	child->resize(widgetSize);
}


/************************************************************************************************/


void ResourceBrowserWidget::AddResourceViewer(ResourceViewer_ptr viewer_ptr)
{
	resourceViewers[viewer_ptr->resourceID] = viewer_ptr;
}


/************************************************************************************************/

void ResourceBrowserWidget::ShowContextMenu(const QPoint& pos)
{
	auto index = table->indexAt(pos);

	if (index.isValid())
	{
		QMenu contextMenu(tr("Context menu"), this);

		std::vector<QAction> actions;

		auto item = table->itemAt(pos);

		auto& projectResource = project.resources[item->row()];
		auto typeID = projectResource->resource->GetResourceTypeID();

		if (auto res = resourceViewers.find(typeID); res != resourceViewers.end()) {
			contextMenu.addAction("View", [&, index = index, type = typeID, resource = project.resources[index.row()]]
				{
					(*resourceViewers[type])(projectResource->resource);
				});
		}

		contextMenu.addAction("Remove", [&, index = index, projRes = project.resources[index.row()]]
			{
				RemoveSelectedItems();
				//project.RemoveResource(projRes->resource);
			});

		contextMenu.exec(table->viewport()->mapToGlobal(pos));
	}
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
