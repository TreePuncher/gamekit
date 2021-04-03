#include "EditorCodeEditor.h"
#include "QtWidgets/qabstractbutton.h"
#include "QtWidgets/qmenubar.h"


EditorCodeEditor::EditorCodeEditor(EditorScriptEngine& IN_scriptEngine, QWidget *parent)
    : QWidget(parent), scriptEngine{ IN_scriptEngine }
{
	ui.setupUi(this);

    auto boxLayout  = findChild<QBoxLayout*>("verticalLayout");
    auto runButton  = findChild<QPushButton*>("run");
    textEditor      = findChild<QPlainTextEdit*>("plainTextEdit");

    textEditor->setTabStopWidth(textEditor->tabStopWidth() / 2);

    auto menuBar = new QMenuBar();

    auto menu = menuBar->addMenu("Hello");
    menu->addAction("World");

    boxLayout->setMenuBar(menuBar);

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
