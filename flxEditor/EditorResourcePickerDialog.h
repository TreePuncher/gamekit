#pragma once

#include "ui_EditorResourcePickerDialog.h"
#include "EditorProject.h"
#include "ResourceIDs.h"

#include <QWidget>
#include <functional>
#include <qdialog.h>
#include <vector>

/************************************************************************************************/

using OnSelectionCallback = std::function<void (ProjectResource_ptr)>;

class EditorProject;

class EditorResourcePickerDialog : public QDialog
{
	Q_OBJECT

public:
	EditorResourcePickerDialog(ResourceID_t resourceType, EditorProject&, QWidget *parent = Q_NULLPTR);
	~EditorResourcePickerDialog();

	void OnSelection(OnSelectionCallback);

private:
	std::vector<ProjectResource_ptr>	filteredResources;
	OnSelectionCallback					onAccept;
	Ui::EditorResourcePickerDialog		ui;
	EditorProject&						project;
};


/************************************************************************************************/
