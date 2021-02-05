#include <QtWidgets/QTableWidget>
#include <QtWidgets/qdockwidget>

#include "ResourceBrowserWidget.h"
#include "TextureViewer.h"
#include "Assets.h"
#include "EditorRenderer.h"
#include "graphics.h"
#include "memoryutilities.h"


/************************************************************************************************/


ResourceBrowserWidget::ResourceBrowserWidget(EditorProject& IN_project, EditorRenderer& IN_renderer, QWidget *parent) :
    QWidget     { parent },
    renderer    { IN_renderer },
    model       { IN_project }
{
	ui.setupUi(this);

    table = findChild<QTableView*>("tableView");
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    table->setModel(&model);

    connect(
        table, SIGNAL(customContextMenuRequested(const QPoint&)),
        SLOT(ShowContextMenu(const QPoint&)));
}


/************************************************************************************************/


ResourceBrowserWidget::~ResourceBrowserWidget()
{
}


/************************************************************************************************/


void ResourceBrowserWidget::resizeEvent(QResizeEvent* evt) 
{
    QWidget::resizeEvent(evt);

    const auto widgetSize   = size();
    const auto width        = widgetSize.width();
    const auto height       = widgetSize.height();

    resize(widgetSize);

    auto child = findChild<QTableView*>("tableView");


    child->resize(widgetSize);
}


/************************************************************************************************/


void ResourceBrowserWidget::ShowContextMenu(const QPoint& pos)
{

    auto index = table->indexAt(pos);

    if (index.isValid())
    {
        QMenu contextMenu(tr("Context menu"), this);

        auto type = model.GetResourceType(index.row());

        switch (type)
        {
        case MeshResourceTypeID:
        {

        }   break;
        case TextureResourceTypeID:
        {
            QAction viewTexture("View Texture", this);
            connect(
                &viewTexture,
                &QAction::triggered, this,
                [&, resource = model.GetResource(index.row())]
                {   // Create 2D Texture viewer
                    auto&   renderSystem = renderer.framework.GetRenderSystem();
                    auto    textureBlob  = resource.get()->CreateBlob();

                    char* textureBuffer         = textureBlob.buffer;
                    auto textureResourceBlob    = reinterpret_cast<FlexKit::TextureResourceBlob*>(textureBuffer);
                    
                    const auto format       = textureResourceBlob->format;
                    const auto WH           = textureResourceBlob->WH;

                    memset((char*)textureResourceBlob->GetBuffer(), 0xf, 128);

                    FlexKit::TextureBuffer  uploadBuffer{ WH, (byte*)textureResourceBlob->GetBuffer(), textureResourceBlob->GetBufferSize(), 16, nullptr };

                    auto queue      = renderSystem.GetImmediateUploadQueue();
                    auto texture    = FlexKit::MoveTextureBufferToVRAM(renderSystem, queue, &uploadBuffer, format);

                    auto docklet        = new QDockWidget{};
                    auto textureViewer  = new TextureViewer(renderer, nullptr, texture);

                    docklet->setWidget(textureViewer);
                    docklet->resize({ 400, 300 });
                    docklet->setFloating("true");
                    docklet->show();
                });

            contextMenu.addAction(&viewTexture);
            contextMenu.exec(table->viewport()->mapToGlobal(pos));
        }   break;
        default:
            break;
        }

    }
}


/************************************************************************************************/


ResourceItemModel::ResourceItemModel(EditorProject& IN_project) :
    QAbstractTableModel{},
    project{ IN_project },
    timer{ new QTimer(this) }
{
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &ResourceItemModel::RefreshTable);
    timer->start();
}


/************************************************************************************************/


int ResourceItemModel::rowCount(const QModelIndex& parent) const
{
    return project.resources.size();
}


/************************************************************************************************/


int ResourceItemModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}


/************************************************************************************************/


FlexKit::ResourceBuilder::Resource_ptr  ResourceItemModel::GetResource(const uint64_t index)
{
    return project.resources[index].resource;
}


/************************************************************************************************/


ResourceID_t  ResourceItemModel::GetResourceType(uint64_t index) const
{
    return project.resources[index].resource->GetResourceTypeID();
}


/************************************************************************************************/


QVariant ResourceItemModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0:
            return QVariant{ project.resources[index.row()].resource->GetResourceID().c_str() };
        case 1:
        {
            static const std::map<ResourceID_t, const char*> IDTypeMap = {
                { TextureResourceTypeID,    "Texture" },
                { MeshResourceTypeID,       "Mesh" },
                { SceneResourceTypeID,      "Scene" },
            };

            const auto editorResource = project.resources[index.row()];

            if (auto res = IDTypeMap.find(editorResource.resource->GetResourceTypeID()); res != IDTypeMap.end())
                return QVariant{ res->second };

            return QVariant{ "Unknown Type" };
        }
        default:
            return QVariant{ tr("Datass!") };
        }
    }

    return QVariant{};
}


/************************************************************************************************/


void ResourceItemModel::RefreshTable()
{
    if (rowCount())
    {
        QModelIndex topLeft = createIndex(0, 0);
        QModelIndex bottomRight = createIndex(project.resources.size(), 0);

        if (project.resources.size() != rows)
        {
            const auto newRowCount = project.resources.size();

            beginInsertRows(QModelIndex{}, rows, newRowCount);
            endInsertRows();

            rows = newRowCount;
            emit layoutChanged();
        }
        else
            emit dataChanged(topLeft, bottomRight, { Qt::DisplayRole });
    }

    timer->start();
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
