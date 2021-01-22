#pragma once

#include <QtWidgets/QWidget>
#include "ui_EditorCodeEditor.h"
#include "EditorScriptEngine.h"
#include "QtWidgets/qplaintextedit.h"


class EditorCodeEditor : public QWidget
{
	Q_OBJECT

public:
	EditorCodeEditor(EditorScriptEngine& scriptEngine, QWidget *parent = Q_NULLPTR);
	~EditorCodeEditor();

    void RunCode();

private:
    QPlainTextEdit*         textEditor;
    EditorScriptEngine&     scriptEngine;
	Ui::EditorCodeEditor    ui;
};
