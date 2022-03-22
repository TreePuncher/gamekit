#pragma once

#include <QtWidgets/QWidget>
#include "ui_EditorCodeEditor.h"
#include "EditorScriptEngine.h"
#include "QtWidgets/qplaintextedit.h"

class BasicHighlighter;
class EditorCodeEditor;
class LineNumberArea;

class QListWidget;
class QTabWidget;

struct BreakPoint;

class EditorTextEditorWidget : public QPlainTextEdit
{
    Q_OBJECT

public:

    EditorTextEditorWidget(QWidget* parent);

    void LineNumberAreaPaintEvent(QPaintEvent* event);
    int  LineNumberAreaWidth();

    void PaintBreakPoint(BreakPoint* begin, BreakPoint* end, QPaintEvent* event);

    void UpdateLineNumberAreaWidth(int newBlockCount);
    void HighlightCurrentLine();
    void UpdateLineNumberArea(const QRect& rect, int dy);

    void resizeEvent(QResizeEvent* e);

    int GetLine(QPoint);

    LineNumberArea* lineNumberArea;
};

class EditorCodeEditor : public QWidget
{
	Q_OBJECT

public:
	EditorCodeEditor(EditorScriptEngine& scriptEngine, QWidget *parent = Q_NULLPTR);
	~EditorCodeEditor();

    void Resume();
    void RunCode();
    void LoadDocument();
    void SaveDocument();
    void SaveDocumentCopy();

private:

    void LineCallback(asIScriptContext* ctx);

    QAction*                undo;
    QAction*                redo;
    QListWidget*            callStackWidget;
    QListWidget*            variableListWidget;

    std::string             fileDir;
    BasicHighlighter*       highlighter = nullptr;
    QTabWidget*             tabBar;
    EditorScriptEngine&     scriptEngine;
	Ui::EditorCodeEditor    ui;
};
