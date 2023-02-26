#pragma once
#include <ui_EditorSelectObjectDialog.h>
#include <functional>

namespace FlexKit
{
	class SceneResource;
};

class EditorSelectObjectDialog : public QDialog
{
	Q_OBJECT
public:
	
	EditorSelectObjectDialog(FlexKit::SceneResource*, QWidget* parent = Q_NULLPTR);
	~EditorSelectObjectDialog();

	void UpdateContents();

	std::function<size_t()>				GetItemCount	= []{ return 0; };
	std::function<std::string (size_t)>	GetItemLabel	= [](size_t){ return ""; };
	std::function<void (size_t)>		OnSelection		= [](size_t) {};


	Ui::SelectSceneObjectDialog ui;
	FlexKit::SceneResource*		scene;
};

