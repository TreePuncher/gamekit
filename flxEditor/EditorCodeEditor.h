#pragma once

#include <QtWidgets/QWidget>
#include "ui_EditorCodeEditor.h"
#include "EditorScriptEngine.h"
#include "QtWidgets/qplaintextedit.h"


class BasicHighlighter;

class EditorCodeEditor : public QWidget
{
	Q_OBJECT

public:
	EditorCodeEditor(EditorScriptEngine& scriptEngine, QWidget *parent = Q_NULLPTR);
	~EditorCodeEditor();

    void RunCode();
    void LoadDocument();
    void SaveDocument();
    void SaveDocumentCopy();

private:

    QAction* undo;
    QAction* redo;

    std::string             fileDir;
    BasicHighlighter*       highlighter = nullptr;
    QPlainTextEdit*         textEditor;
    EditorScriptEngine&     scriptEngine;
	Ui::EditorCodeEditor    ui;
};
