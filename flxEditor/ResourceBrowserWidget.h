#pragma once


/************************************************************************************************/


#include "ui_ResourceBrowserWidget.h"
#include "EditorProject.h"
#include "../FlexKitResourceCompiler/ResourceIDs.h"

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QtWidgets/qmenu.h>


/************************************************************************************************/


class EditorRenderer;

/************************************************************************************************/


class ResourceItemModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ResourceItemModel(EditorProject& IN_project);
    int rowCount        (const QModelIndex& parent = QModelIndex()) const override;
    int columnCount     (const QModelIndex& parent = QModelIndex()) const override;

    QVariant                                data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    ResourceID_t                            GetResourceType(uint64_t index) const;
    FlexKit::ResourceBuilder::Resource_ptr  GetResource(const uint64_t index);
    void                                    RefreshTable();

private:

    size_t          rows = 0;
    QTimer*         timer;
    EditorProject&  project;
};


/************************************************************************************************/


class ResourceBrowserWidget : public QWidget
{
	Q_OBJECT

public:
	ResourceBrowserWidget(EditorProject& IN_project, EditorRenderer& renderer, QWidget *parent = Q_NULLPTR);
	~ResourceBrowserWidget();

    void resizeEvent(QResizeEvent* event) override;

public slots:

    void ShowContextMenu(const QPoint& pos);
private:

    QTableView*                 table;
    ResourceItemModel           model;
    EditorRenderer&             renderer;
	Ui::ResourceBrowserWidget   ui;
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
