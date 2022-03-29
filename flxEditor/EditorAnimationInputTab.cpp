#include "pch.h"
#include "EditorAnimationInputTab.h"


/************************************************************************************************/


EditorAnimationInputTab::EditorAnimationInputTab(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    auto verticalHeader     = ui.tableView->verticalHeader();
    auto horizontalHeader   = ui.tableView->horizontalHeader();

    horizontalHeader->setVisible(true);
    verticalHeader->setVisible(true);

    connect(ui.pushButton, &QPushButton::pressed,
        [&]()
        {
            callback(ui.comboBox->currentIndex(), ui.lineEdit->text().toStdString());
        });
}


/************************************************************************************************/


EditorAnimationInputTab::~EditorAnimationInputTab()
{
}


/************************************************************************************************/


void EditorAnimationInputTab::Update(const uint32_t tableCount, ReadEntryData fetchData)
{
    for (uint32_t I = 0; I < tableCount; I++)
    {
    }
}


/************************************************************************************************/


void EditorAnimationInputTab::SetOnCreateEvent(OnCreationEventFN&& IN_callback)
{
    callback = IN_callback;
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
