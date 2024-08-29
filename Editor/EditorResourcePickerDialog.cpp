#include "PCH.h"
#include "EditorResourcePickerDialog.h"
#include "qpushbutton.h"

/************************************************************************************************/


EditorResourcePickerDialog::EditorResourcePickerDialog(ResourceID_t resourceType, EditorProject& IN_project, QWidget *parent)
	: QDialog(parent, Qt::Dialog)
	, project     { IN_project }
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	for (auto resource : project.resources)
	{
		if (resource->resource->GetResourceTypeID() == resourceType)
			filteredResources.push_back(resource);
	}

	for (auto resource : filteredResources)
		ui.listWidget->addItem(resource->resource->GetResourceID().c_str());

	connect(
		ui.acceptButton, &QPushButton::pressed,
		[&]()
		{
			auto item = ui.listWidget->currentItem();

			if (item == nullptr)
				return;

			auto idx = ui.listWidget->indexFromItem(item);
			ProjectResource_ptr resource = filteredResources[idx.row()];

			if(onAccept)
				onAccept(resource);

			delete this;
		});

	connect(
		ui.cancelButton, &QPushButton::pressed,
		[&]()
		{
			close();
		});
}


/************************************************************************************************/


EditorResourcePickerDialog::~EditorResourcePickerDialog()
{
}


/************************************************************************************************/


void EditorResourcePickerDialog::OnSelection(OnSelectionCallback callback)
{
	onAccept = callback;
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
