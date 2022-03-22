#include "EditorResourcePickerDialog.h"
#include "qpushbutton.h"

/************************************************************************************************/


EditorResourcePickerDialog::EditorResourcePickerDialog(ResourceID_t resourceType, EditorProject& IN_project, QWidget *parent)
	: QDialog(parent, Qt::Dialog)
    , project     { IN_project }
{
	ui.setupUi(this);


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
            delete this;
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


/************************************************************************************************/
