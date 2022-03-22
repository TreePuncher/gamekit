#pragma once

#include <QWidget>
#include "ui_EditorAnimationEditor.h"


/************************************************************************************************/


class AnimatorSelectionContext;
class DXRenderWindow;
class EditorAnimationPreview;
class EditorRenderer;
class EditorProject;
class EditorCodeEditor;
class QMenuBar;
class EditorScriptEngine;


/************************************************************************************************/



class EditorAnimationEditor : public QWidget
{
	Q_OBJECT

public:
	EditorAnimationEditor(EditorScriptEngine& engine, EditorRenderer& IN_renderer, EditorProject& IN_project, QWidget *parent = Q_NULLPTR);
	~EditorAnimationEditor();

private:
    QMenuBar*                               menubar;
    AnimatorSelectionContext*               selection;
    EditorAnimationPreview*                 previewWindow;
    EditorProject&                          project;
    EditorCodeEditor*                       codeEditor;
	Ui::EditorAnimationEditor               ui;
};


/************************************************************************************************/
