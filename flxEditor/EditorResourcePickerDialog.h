#pragma once

#include <QWidget>
#include "ui_EditorResourcePickerDialog.h"
#include "..\FlexKitResourceCompiler\ResourceIDs.h"
#include "..\FlexKitResourceCompiler\Common.h"
#include "EditorProject.h"

#include <functional>
#include <qdialog.h>
#include <vector>

/************************************************************************************************/


using OnSelectionCallback = std::function<void (ProjectResource_ptr)>;

class EditorProject;
class EditorViewport;

class EditorResourcePickerDialog : public QDialog
{
	Q_OBJECT

public:
	EditorResourcePickerDialog(ResourceID_t resourceType, EditorProject&, EditorViewport&, QWidget *parent = Q_NULLPTR);
	~EditorResourcePickerDialog();

    void OnSelection(OnSelectionCallback);

private:
    std::vector<ProjectResource_ptr>    filteredResources;
    OnSelectionCallback                 onAccept;
	Ui::EditorResourcePickerDialog      ui;
    EditorProject&                      project;
    EditorViewport&                     viewport;
};


/************************************************************************************************/
