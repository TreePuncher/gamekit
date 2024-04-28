#include "pch.h"
#include "EditorTaskList.h"
#include <qheaderview.h>
#include "EditorTaskManager.h"

std::mutex	m;
std::vector<EditorTask_ptr> tasks;


/************************************************************************************************/


void PostTask(FlexKit::iWork& work, EditorTask_ptr task)
{
	work.Subscribe(
		[task]
		{
			task->state = TaskState::Completed;
		});

	PushToLocalQueue(work);

	std::scoped_lock sl{ m };
	tasks.push_back(task);
}


/************************************************************************************************/


EditorTaskList::EditorTaskList(QWidget *parent)
	: QWidget(parent)
	, timer { new QTimer(this) }
{
	ui.setupUi(this);

	timer->setInterval(1000);

	connect(timer, &QTimer::timeout, this, &EditorTaskList::Update);

	Update();
}


/************************************************************************************************/


EditorTaskList::~EditorTaskList()
{
}


/************************************************************************************************/


void EditorTaskList::Update()
{
	if (m.try_lock())
	{
		ui.tableWidget->setRowCount(tasks.size());

		for (auto&& [idx, task] : enumerate(tasks))
		{
			auto name	= ui.tableWidget->item(idx, 0);
			auto status = ui.tableWidget->item(idx, 1);

			if (!name)
			{
				name = new QTableWidgetItem{};
				ui.tableWidget->setItem(idx, 0, name);
			}

			if (!status)
			{
				status = new QTableWidgetItem{};
				ui.tableWidget->setItem(idx, 1, status);
			}

			name->setData(Qt::DisplayRole,		{ task->GetName().c_str() });
			status->setData(Qt::DisplayRole,	{ task->GetDescription().c_str() });
		}

		m.unlock();
	}
	timer->start();
}


/**********************************************************************

Copyright (c) 2019-2024 Robert May

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
