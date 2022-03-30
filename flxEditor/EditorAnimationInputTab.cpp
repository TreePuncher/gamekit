#include "pch.h"
#include "EditorAnimationInputTab.h"


/************************************************************************************************/


EditorAnimationInputTab::EditorAnimationInputTab(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    auto verticalHeader     = ui.tableWidget->verticalHeader();
    auto horizontalHeader   = ui.tableWidget->horizontalHeader();

    horizontalHeader->setVisible(true);
    verticalHeader->setVisible(true);

    ui.tableWidget->setHorizontalHeaderLabels({ "Name", "Type", "Value" });

    connect(ui.pushButton, &QPushButton::pressed,
        [&]()
        {
            callback(ui.comboBox->currentIndex(), ui.lineEdit->text().toStdString());
        });

    connect(ui.tableWidget, &QTableWidget::itemChanged,
        [&](QTableWidgetItem* item)
        {
            const auto row = item->row();

            auto name   = ui.tableWidget->item(row, 0);
            auto value  = ui.tableWidget->item(row, 1);

            if (name && value)
            {
                auto nameString     = name->text().toStdString();
                auto valueString    = value->text().toStdString();

                writeData((size_t) row, nameString, valueString);
            }
        });
}


/************************************************************************************************/


EditorAnimationInputTab::~EditorAnimationInputTab()
{
}

void EditorAnimationInputTab::Update(const uint32_t rowCount, ReadEntryDataFN fetchData)
{
    ui.tableWidget->setRowCount(rowCount);
    ui.tableWidget->setColumnCount(2);

    for (uint32_t row = 0; row < rowCount; row++)
    {
        std::string value;
        std::string name;
        fetchData(row, name, value);

        if (value.size() == 0)
            value = "0";

        if (!ui.tableWidget->item(row, 1) || ui.tableWidget->item(row, 1)->text().size() == 0 || ui.tableWidget->item(row, 1)->text().toStdString() != value)
        {
            QTableWidgetItem* nameItem  = new QTableWidgetItem(name.c_str());
            QTableWidgetItem* valueItem = new QTableWidgetItem(value.c_str());

            ui.tableWidget->setItem(row, 0, nameItem);
            ui.tableWidget->setItem(row, 1, valueItem);
        }
    }
}


/************************************************************************************************/


void EditorAnimationInputTab::SetOnCreateEvent(OnCreationEventFN&& IN_callback)
{
    callback = IN_callback;
}


/************************************************************************************************/


void EditorAnimationInputTab::SetOnChangeEvent(WriteEntryDataFN&& in)
{
    writeData = in;
}


void EditorAnimationInputTab::AddInput(const std::string& name, uint32_t type)
{

}


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
