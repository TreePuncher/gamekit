#include "PCH.h"
#include "EditorMeshResourceViewer.h"
#include "EditorMeshResource.hpp"
#include <qcombobox.h>
#include <ranges>

using std::views::iota;//
using std::views::zip;//

/************************************************************************************************/


EditorMeshResourceViewer::EditorMeshResourceViewer(FlexKit::Resource_ptr IN_resource, QWidget* parent) :
	QWidget	{ parent },
	docklet	{ new QDockWidget{ this } },
	resource{ IN_resource }
{
	ui.setupUi(docklet);

	connect(
		ui.applyButton, &QPushButton::pressed,
		this, &EditorMeshResourceViewer::Apply);

	connect(
		ui.cancelButton, &QPushButton::pressed,
		this, &EditorMeshResourceViewer::Cancel);

	ui.subMeshTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.subMeshTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

	connect(ui.subMeshTable,		&QTableWidget::clicked,		[&](auto...) { UpdateSubMeshUI(); UpdateAttributes();  });
	connect(ui.attributesTable,		&QTableWidget::clicked,		[&](auto...) { UpdateAttributes(); });
	connect(ui.subMeshTable,		&QTableWidget::itemChanged, this, &EditorMeshResourceViewer::ItemChanged);

	UpdateUI();
	UpdateSubMeshUI();
	UpdateAttributes();
}


/************************************************************************************************/


void EditorMeshResourceViewer::Apply()
{
	auto	mesh_ptr = std::static_pointer_cast<FlexKit::MeshResource>(resource);

	mesh_ptr->MarkDirty();
	mesh_ptr->Save();

	delete docklet;
	delete this;
}


/************************************************************************************************/


void EditorMeshResourceViewer::Cancel()
{
	auto	mesh_ptr = std::static_pointer_cast<FlexKit::MeshResource>(resource);
	mesh_ptr->Unload();

	delete docklet;
	delete this;
}


/************************************************************************************************/


void EditorMeshResourceViewer::UpdateUI()
{
	auto mesh_ptr	= std::static_pointer_cast<FlexKit::MeshResource>(resource);
	auto& meshData	= mesh_ptr->Object();
	auto table		= ui.LODTable;

	table->setRowCount(meshData.LODs.size());

	for (auto&& [idx, lod] : zip(std::views::iota(0), meshData.LODs))
	{
		auto lodLevel		= new QTableWidgetItem(QString{ "%1" }.arg(idx));
		auto attributeCount	= new QTableWidgetItem(QString{ "%1" }.arg(lod.buffers.size()));
		auto indexCount		= new QTableWidgetItem(QString{ "%1" }.arg(lod.IndexCount));
		auto SubMeshCount	= new QTableWidgetItem(QString{ "%1" }.arg(lod.submeshes.size()));

		table->setItem(idx, 0, lodLevel);
		table->setItem(idx, 1, attributeCount);
		table->setItem(idx, 2, indexCount);
		table->setItem(idx, 3, SubMeshCount);
	}
}


/************************************************************************************************/


void EditorMeshResourceViewer::UpdateSubMeshUI()
{
	auto mesh_ptr	= std::static_pointer_cast<FlexKit::MeshResource>(resource);
	auto& meshData	= mesh_ptr->Object();
	auto table		= ui.subMeshTable;

	auto row	= table->currentRow();
	row			= row == -1 ? 0 : row;

	auto& subMeshes = meshData.LODs[row].submeshes;
	table->setRowCount(subMeshes.size());

	for (auto&& [idx, subMesh] : zip(std::views::iota(0), subMeshes))
	{
		auto BaseIndex		= new QTableWidgetItem(QString{ "%1" }.arg(subMesh.BaseIndex));
		auto IndexCount		= new QTableWidgetItem(QString{ "%1" }.arg(subMesh.IndexCount));
		auto materialIndex	= new QTableWidgetItem(QString{ "%1" }.arg(subMesh.materialIndex));

		table->setItem(idx, 0, BaseIndex);
		table->setItem(idx, 1, IndexCount);
		table->setItem(idx, 2, materialIndex);
	}
}


/************************************************************************************************/


auto CreateAttributeTypeWidget()
{
	QComboBox* comboBox = new QComboBox{};
	comboBox->addItem("R8 uint");		// 0
	comboBox->addItem("RG8 uint");		// 1
	comboBox->addItem("RGB8 uint");		// 2
	comboBox->addItem("RGB8 uint");		// 3
	comboBox->addItem("RGB8 uint");		// 4

	comboBox->addItem("X16 float");		// 5
	comboBox->addItem("XY16 float");	// 6
	comboBox->addItem("XYZ16 float");	// 7
	comboBox->addItem("XYZW16 float");	// 8

	comboBox->addItem("X32 float");		// 9
	comboBox->addItem("XY32 float");	// 10
	comboBox->addItem("XYZ32 float");	// 11
	comboBox->addItem("XYZW32 float");	// 12
	comboBox->addItem("4x4 float");		// 13

	comboBox->addItem("I16 index");		// 14
	comboBox->addItem("I32 index");		// 15

	comboBox->addItem("Error!");		// 16

	return comboBox;
}

auto CreateAttributeUsageWidget()
{
	QComboBox* comboBox = new QComboBox{};

	comboBox->addItem("COLOR");
	comboBox->addItem("NORMAL");
	comboBox->addItem("TANGENT");
	comboBox->addItem("UV");
	comboBox->addItem("POSITION");
	comboBox->addItem("USERTYPE");
	comboBox->addItem("USERTYPE2");
	comboBox->addItem("USERTYPE3");
	comboBox->addItem("USERTYPE4");
	comboBox->addItem("COMBINED");
	comboBox->addItem("PACKED");
	comboBox->addItem("PACKEDANIMATION");
	comboBox->addItem("INDEX");
	comboBox->addItem("ANIMATION1");
	comboBox->addItem("ANIMATION2");
	comboBox->addItem("ANIMATION3");
	comboBox->addItem("ANIMATION4");
	comboBox->addItem("MORPHTARGETPOS");
	comboBox->addItem("MORPHTARGETNORMAL");
	comboBox->addItem("MORPHTARGETTANGENT");

	return comboBox;
}

void EditorMeshResourceViewer::UpdateAttributes()
{
	auto mesh_ptr	= std::static_pointer_cast<FlexKit::MeshResource>(resource);
	auto& meshData	= mesh_ptr->Object();
	auto table		= ui.attributesTable;

	auto row	= table->currentRow();
	row			= row == -1 ? 0 : row;

	table->setRowCount(meshData.LODs[row].buffers.size());

	for (auto&& [idx, buffer] : zip(std::views::iota(0), meshData.LODs[row].buffers))
	{
		auto typeE		= buffer->GetBufferType();
		auto formatE	= buffer->GetBufferFormat();

		auto typeWidget		= CreateAttributeTypeWidget();
		auto usageWidget	= CreateAttributeUsageWidget();

		switch (formatE)
		{
			break;
		case VERTEXBUFFER_FORMAT::R8:
			typeWidget->setCurrentIndex(0);
			break;
		case VERTEXBUFFER_FORMAT::R8G8B8:
			typeWidget->setCurrentIndex(1);
			break;
		case VERTEXBUFFER_FORMAT::R8G8B8A8:
			typeWidget->setCurrentIndex(2);
			break;
		case VERTEXBUFFER_FORMAT::R16:
			if(typeE == VERTEXBUFFER_TYPE::INDEX)
				typeWidget->setCurrentIndex(14);
			else
				typeWidget->setCurrentIndex(5);
			break;
		case FlexKit::VERTEXBUFFER_FORMAT::R16G16:
			if (typeE == FlexKit::VERTEXBUFFER_TYPE::INDEX)
				typeWidget->setCurrentIndex(15);
			else
				typeWidget->setCurrentIndex(6);
			break;
		case FlexKit::VERTEXBUFFER_FORMAT::R16G16B16:
			typeWidget->setCurrentIndex(7);
			break;
		case FlexKit::VERTEXBUFFER_FORMAT::R32G32B32:
			typeWidget->setCurrentIndex(11);
		case FlexKit::VERTEXBUFFER_FORMAT::R32G32B32A32:
			typeWidget->setCurrentIndex(12);
			break;
		case FlexKit::VERTEXBUFFER_FORMAT::MATRIX:
			typeWidget->setCurrentIndex(13);
			break;
		default:
			typeWidget->setCurrentIndex(14);
			break;
		}

		usageWidget->setCurrentIndex((int)typeE);

		table->setIndexWidget(table->model()->index(idx, 0), usageWidget);
		table->setIndexWidget(table->model()->index(idx, 1), typeWidget);
	}
}


/************************************************************************************************/


void EditorMeshResourceViewer::ItemChanged(QTableWidgetItem* item)
{
	if (item->column() == 2 && item->tableWidget() == ui.subMeshTable)
	{
		bool ok = false;
		int materialIndex = item->text().toInt(&ok);

		if (!ok)
			return;

		auto table	= ui.subMeshTable;
		auto row	= table->currentRow();

		if (row == -1)
			return;

		auto mesh_ptr	= std::static_pointer_cast<FlexKit::MeshResource>(resource);
		auto& meshData	= mesh_ptr->Object();
		auto& subMeshes	= meshData.LODs[row].submeshes;

		subMeshes[row].materialIndex = materialIndex;
	}
}


/************************************************************************************************/


MeshResourceViewer::MeshResourceViewer(QMainWindow* parentWindow) :
	IResourceViewer	{ MeshResourceTypeID	},
	mainWindow		{ parentWindow			} {}


/************************************************************************************************/


void MeshResourceViewer::operator () (FlexKit::Resource_ptr resource)
{
	auto widget = new EditorMeshResourceViewer{ resource, mainWindow };

	mainWindow->addDockWidget(Qt::RightDockWidgetArea, widget->docklet);
	widget->docklet->setFloating(false);
}


/**********************************************************************

Copyright (c) 2019-2023 Robert May

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
