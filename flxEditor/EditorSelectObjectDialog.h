#pragma once
#include <ui_EditorSelectObjectDialog.h>
#include <functional>
#include <type.h>

namespace FlexKit
{
	class SceneObject;
}

class EditorSelectObjectDialog : public QDialog
{
	Q_OBJECT
public:
	
	EditorSelectObjectDialog(FlexKit::SceneObject*, QWidget* parent = Q_NULLPTR);
	~EditorSelectObjectDialog();

	void UpdateContents();

	std::function<size_t()>				GetItemCount	= []{ return 0; };
	std::function<std::string (size_t)>	GetItemLabel	= [](size_t){ return ""; };
	std::function<void (size_t)>		OnSelection		= [](size_t) {};


	Ui::SelectSceneObjectDialog		ui;
	FlexKit::SceneObject*			scene;
};

