#pragma once

#include <QtWidgets/QWidget>
#include "ui_EditorCodeEditor.h"
#include "EditorScriptEngine.h"
#include "QtWidgets/qplaintextedit.h"
#include "EditorScriptObject.h"
#include <chrono>


/************************************************************************************************/


class asIScriptContext;
class BasicHighlighter;
class EditorCodeEditor;
class EditorProject;
class LineNumberArea;
class ScriptResource;


class QListWidget;
class QTabWidget;

struct BreakPoint;


/************************************************************************************************/


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

    int  GetLine(QPoint);

    void AddWarningLine(std::string msg, int line);
    void ClearWarnings();

    LineNumberArea* lineNumberArea;

    struct LineMessage
    {
        std::string message;
        int         line;
    };

    std::vector<LineMessage>    lineMessages;
};


/************************************************************************************************/


class EditorCodeEditor : public QWidget
{
	Q_OBJECT

public:
	EditorCodeEditor(EditorProject& IN_project, EditorScriptEngine& scriptEngine, QWidget *parent = Q_NULLPTR);
	~EditorCodeEditor();

    void Resume();
    void Stop();
    void RunCode();
    void LoadDocument();
    void SaveDocument();
    void SaveDocumentCopy();
    void SetResource(ScriptResource_ptr);

    QMenuBar* GetMenuBar() noexcept;
private:

    void LineCallback(asIScriptContext* ctx);

    EditorProject&          project;
    ScriptResource_ptr      currentResource = nullptr;
    QAction*                undo;
    QAction*                redo;
    QListWidget*            callStackWidget;
    QListWidget*            variableListWidget;
    QListWidget*            errorListWidget;
    QMenuBar*               menuBar;
    EditorTextEditorWidget* textEditor;

    std::string             fileDir;
    BasicHighlighter*       highlighter = nullptr;
    QTabWidget*             tabBar;

    std::chrono::system_clock::time_point begin;

    EditorScriptEngine&     scriptEngine;
    asIScriptContext*       scriptContext;

	Ui::EditorCodeEditor    ui;
};


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
