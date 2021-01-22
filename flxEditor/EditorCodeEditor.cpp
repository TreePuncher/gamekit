#include "EditorCodeEditor.h"
#include "QtWidgets/qabstractbutton.h"


EditorCodeEditor::EditorCodeEditor(EditorScriptEngine& IN_scriptEngine, QWidget *parent)
    : QWidget(parent), scriptEngine{ IN_scriptEngine }
{
	ui.setupUi(this);

    auto runButton  = findChild<QPushButton*>("run");
    textEditor      = findChild<QPlainTextEdit*>("plainTextEdit");

    textEditor->setTabStopWidth(textEditor->tabStopWidth() / 2);

    connect(runButton, &QPushButton::pressed, [&] { RunCode(); });
}

EditorCodeEditor::~EditorCodeEditor()
{
}

void EditorCodeEditor::RunCode()
{
    auto code = textEditor->toPlainText().toStdString();

    scriptEngine.RunStdString(code);
}
