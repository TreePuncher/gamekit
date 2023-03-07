#include "PCH.h"
#include "EditorSelectObjectDialog.h"
#include <QListWidgetItem>


/************************************************************************************************/


EditorSelectObjectDialog::EditorSelectObjectDialog(FlexKit::SceneResource* IN_scene, QWidget* parent) :
	scene { IN_scene }
{
	ui.setupUi(this);
	
	setAttribute(Qt::WA_DeleteOnClose, true);

	connect(
		ui.selectButton, &QPushButton::pressed,
		[&]()
		{
			auto selected = ui.listWidget->selectedItems();

			if (selected.size())
			{
				OnSelection(ui.listWidget->indexFromItem(selected.first()).row());
				delete this;
			}
		});

	connect(
		ui.cancelButton, &QPushButton::pressed,
		[&]()
		{
			delete this;
		});
}


/************************************************************************************************/


EditorSelectObjectDialog::~EditorSelectObjectDialog()
{

}


/************************************************************************************************/


void EditorSelectObjectDialog::UpdateContents()
{
	auto currentCount = ui.listWidget->count();
	auto updatedCount = GetItemCount();


	for (size_t i = 0; i < currentCount; i++)
	{
		auto item = ui.listWidget->item(i);

		auto label = GetItemLabel(i);
		item->setText(label.c_str());
	}

	for (size_t i = currentCount; i < updatedCount; i++)
	{
		auto label = GetItemLabel(i);
		ui.listWidget->addItem(label.c_str());
	}
}


/**********************************************************************

Copyright (c) 2021 - 2023 Robert May

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
