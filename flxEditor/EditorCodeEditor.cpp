#include "PCH.h"
#include "EditorCodeEditor.h"
#include "QtWidgets/qabstractbutton.h"
#include "QtWidgets/qmenubar.h"
#include "QtWidgets/qlistwidget.h"
#include "EditorPrefabObject.h"
#include "EditorProject.h"
#include "EditorResourcePickerDialog.h"
#include "EditorScriptEngine.h"

#include <qsyntaxhighlighter.h>
#include <qregularexpression.h>
#include <qfile.h>
#include <QtWidgets/qfiledialog.h>
#include <qtextdocumentwriter.h>
#include <qshortcut.h>
#include <qtablewidget.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <angelscript/debugger/debugger.h>

/************************************************************************************************/


class BasicHighlighter : public QSyntaxHighlighter
{
public:
    BasicHighlighter(QTextDocument* document) : QSyntaxHighlighter{document}
    {
        HighlightingRule rule;

        keywordFormat.setForeground(Qt::yellow);
        keywordFormat.setFontWeight(QFont::Bold);
        const QString keywordPatterns[] = {
            QStringLiteral("\\bchar\\b"),       QStringLiteral("\\bclass\\b"),      QStringLiteral("\\bconst\\b"),
            QStringLiteral("\\bdouble\\b"),     QStringLiteral("\\benum\\b"),       QStringLiteral("\\bexplicit\\b"),
            QStringLiteral("\\bfriend\\b"),     QStringLiteral("\\binline\\b"),     QStringLiteral("\\bint\\b"),
            QStringLiteral("\\blong\\b"),       QStringLiteral("\\bnamespace\\b"),  QStringLiteral("\\boperator\\b"),
            QStringLiteral("\\bprivate\\b"),    QStringLiteral("\\bprotected\\b"),  QStringLiteral("\\bpublic\\b"),
            QStringLiteral("\\bshort\\b"),      QStringLiteral("\\bsignals\\b"),    QStringLiteral("\\bsigned\\b"),
            QStringLiteral("\\bslots\\b"),      QStringLiteral("\\bstatic\\b"),     QStringLiteral("\\bstruct\\b"),
            QStringLiteral("\\btemplate\\b"),   QStringLiteral("\\btypedef\\b"),    QStringLiteral("\\btypename\\b"),
            QStringLiteral("\\bunion\\b"),      QStringLiteral("\\bunsigned\\b"),   QStringLiteral("\\bvirtual\\b"),
            QStringLiteral("\\bvoid\\b"),       QStringLiteral("\\bvolatile\\b"),   QStringLiteral("\\bbool\\b")
        };

        for (const QString& pattern : keywordPatterns)
        {
            rule.pattern    = QRegularExpression(pattern);
            rule.format     = keywordFormat;
            highlightingRules.append(rule);
        }

        classFormat.setFontWeight(QFont::Bold);
        classFormat.setForeground(Qt::darkMagenta);
        rule.pattern    = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
        rule.format     = classFormat;
        highlightingRules.append(rule);

        quotationFormat.setForeground(Qt::darkGray);
        rule.pattern    = QRegularExpression(QStringLiteral("\".*\""));
        rule.format     = quotationFormat;
        highlightingRules.append(rule);

        functionFormat.setFontItalic(true);
        functionFormat.setForeground(Qt::cyan);
        rule.pattern    = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
        rule.format     = functionFormat;
        highlightingRules.append(rule);

        singleLineCommentFormat.setForeground(Qt::red);
        rule.pattern    = QRegularExpression(QStringLiteral("//[^\n]*"));
        rule.format     = singleLineCommentFormat;
        highlightingRules.append(rule);

        multiLineCommentFormat.setForeground(Qt::red);

        commentStartExpression  = QRegularExpression(QStringLiteral("/\\*"));
        commentEndExpression    = QRegularExpression(QStringLiteral("\\*/"));
    }

    virtual ~BasicHighlighter() final {}

    virtual void highlightBlock(const QString& text) final
    {
        for (const HighlightingRule& rule : qAsConst(highlightingRules))
        {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext())
            {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1)
            startIndex = text.indexOf(commentStartExpression);

        while (startIndex >= 0)
        {
            QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength = 0;

            if (endIndex == -1)
            {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            }
            else
            {
                commentLength = endIndex - startIndex
                    + match.capturedLength();
            }

            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
        }
    }

    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;

    bool enabled = true;
};


/************************************************************************************************/


class LineNumberArea : public QWidget
{
public:
    LineNumberArea(EditorTextEditorWidget* editor)
        : QWidget       { editor }
        , codeEditor    { editor } { }

    QSize sizeHint() const override
    {
        return QSize(codeEditor->LineNumberAreaWidth() + 10, 0);
    }

    virtual void mousePressEvent(QMouseEvent* evt) override
    {
        QWidget::mousePressEvent(evt);

        const int line = codeEditor->GetLine(evt->pos());

        auto res =
            std::find_if(
                breakPoints.begin(),
                breakPoints.end(),
                [&](auto& item)
                {
                    return item.line == line;
                });

        if(res == breakPoints.end())
            breakPoints.push_back({ line });
        else
            breakPoints.erase(res);

        codeEditor->repaint();
    }


    const auto& GetBreakponts() const
    {
        return breakPoints;
    }

protected:
    void paintEvent(QPaintEvent* evt) override
    {
        std::sort(breakPoints.begin(), breakPoints.end());

        codeEditor->LineNumberAreaPaintEvent(evt);
        codeEditor->PaintBreakPoint(breakPoints.data(), breakPoints.data() + breakPoints.size(), evt);
    }

private:

    std::vector<BreakPoint>     breakPoints;
    EditorTextEditorWidget*     codeEditor;
};


/************************************************************************************************/


EditorTextEditorWidget::EditorTextEditorWidget(QWidget* parent)
    : QPlainTextEdit    { parent }
    , lineNumberArea    { new LineNumberArea{ this }}
{
    auto temp = tabStopDistance();
    setTabStopDistance(tabStopDistance() / 3);

    connect(this, &QPlainTextEdit::blockCountChanged,       this, &EditorTextEditorWidget::UpdateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,           this, &EditorTextEditorWidget::UpdateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,   this, &EditorTextEditorWidget::HighlightCurrentLine);

    UpdateLineNumberAreaWidth(0);
    HighlightCurrentLine();

    auto f = font();
    f.setPointSize(f.pointSize() + 2);
    setFont(f);
}


/************************************************************************************************/


void EditorTextEditorWidget::LineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::darkCyan);

    QTextBlock block    = firstVisibleBlock();
    int blockNumber     = block.blockNumber();
    int top             = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom          = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                Qt::AlignRight, number);
        }

        block   = block.next();
        top     = bottom;
        bottom  = top + qRound(blockBoundingRect(block).height());

        ++blockNumber;
    }
}


/************************************************************************************************/


void EditorTextEditorWidget::resizeEvent(QResizeEvent* e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), LineNumberAreaWidth(), cr.height()));
}


/************************************************************************************************/


void EditorTextEditorWidget::PaintBreakPoint(BreakPoint* itr, BreakPoint* end, QPaintEvent* event)
{
    QPainter painter(lineNumberArea);

    QTextBlock block            = firstVisibleBlock();
    auto format                 = block.blockFormat();
    const auto firstVisableLine = block.firstLineNumber();

    QFontMetrics fontMetrics(font());
    auto lineHeight = fontMetrics.height();

    while (itr != end)
    {
        const auto linePosition = lineHeight * itr->line - firstVisableLine * lineHeight;

        if(linePosition >= 0)
            painter.fillRect(4, linePosition + 4, 10, lineHeight - 4, Qt::red);

        itr++;
    }
}


int EditorTextEditorWidget::LineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());

    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 20 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}


/************************************************************************************************/


void EditorTextEditorWidget::UpdateLineNumberAreaWidth(int newBlockCount)
{
    setViewportMargins(LineNumberAreaWidth(), 0, 0, 0);

}

int EditorTextEditorWidget::GetLine(QPoint point)
{
    auto cursor = cursorForPosition(point);
    return cursor.block().firstLineNumber();
}

void EditorTextEditorWidget::AddWarningLine(std::string msg, int line)
{
    lineMessages.emplace_back(msg, line);
}

void EditorTextEditorWidget::ClearWarnings()
{
    lineMessages.clear();
}


/************************************************************************************************/


void EditorTextEditorWidget::HighlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::darkGreen).darker(200);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}


/************************************************************************************************/


void EditorTextEditorWidget::UpdateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        UpdateLineNumberAreaWidth(0);
}


/************************************************************************************************/


EditorCodeEditor::EditorCodeEditor(EditorProject& IN_project, EditorScriptEngine& IN_scriptEngine, QWidget *parent)
    : QWidget               { parent            }
    , scriptEngine          { IN_scriptEngine   }
    , callStackWidget       { new QListWidget{} }
    , variableListWidget    { new QListWidget{} }
    , errorListWidget       { new QListWidget{} }
    , scriptContext         { IN_scriptEngine.GetScriptEngine()->CreateContext() }
    , project               { IN_project }
    , menuBar               { new QMenuBar() }
    , textEditor            { new EditorTextEditorWidget{ this } }
{
	ui.setupUi(this);
    ui.verticalLayout->setContentsMargins(0, 0, 0, 0);

    auto boxLayout  = findChild<QBoxLayout*>("verticalLayout");
    tabBar          = findChild<QTabWidget*>("tabWidget");

    tabBar->addTab(textEditor,          "Code Editor");
    tabBar->addTab(callStackWidget,     "CallStack");
    tabBar->addTab(variableListWidget,  "Variables");
    tabBar->addTab(errorListWidget,     "Errors");

    highlighter = new BasicHighlighter{ textEditor->document() };

    // Setup Menu
    menuBar->setVisible(true);

    auto fileMenu    = menuBar->addMenu("File");

    auto newResource = fileMenu->addAction("New Script Resource");
    connect(newResource, &QAction::triggered,
        [&]()
        {
            if (currentResource)
                textEditor->document()->clear();

            currentResource = std::make_shared<ScriptResource>();

            project.AddResource(FlexKit::Resource_ptr{ currentResource });
        });

    auto selectResource = fileMenu->addAction("Select Resource");
    connect(selectResource, &QAction::triggered,
        [&]()
        {
            auto resourcePicker = new EditorResourcePickerDialog{ ScriptResourceTypeID, project, this };
            resourcePicker->OnSelection(
                [&](ProjectResource_ptr resource)
                {
                    if (currentResource)
                        textEditor->document()->clear();

                    currentResource = std::static_pointer_cast<ScriptResource>(resource->resource);

                    textEditor->document()->setPlainText(currentResource->source.c_str());
                });

            resourcePicker->show();
        });

    auto fileLoad               = fileMenu->addAction("Load File");
    connect(fileLoad, &QAction::triggered, this, &EditorCodeEditor::LoadDocument);

    auto fileSave               = fileMenu->addAction("Save File");
    connect(fileSave, &QAction::triggered, this, &EditorCodeEditor::SaveDocument);

    auto fileSaveCopy            = fileMenu->addAction("Save File Copy");
    connect(fileSaveCopy, &QAction::triggered, this, &EditorCodeEditor::SaveDocumentCopy);

    auto editMenu               = menuBar->addMenu("Edit");
    undo = editMenu->addAction("Undo");
    redo = editMenu->addAction("Redo");

    undo->setEnabled(textEditor->document()->availableUndoSteps() > 0);
    redo->setEnabled(textEditor->document()->availableRedoSteps() > 0);

    textEditor->document()->connect(textEditor->document(), &QTextDocument::redoAvailable, redo, &QAction::setEnabled);
    textEditor->document()->connect(textEditor->document(), &QTextDocument::undoAvailable, undo, &QAction::setEnabled);

    connect(undo, &QAction::triggered, textEditor, &QPlainTextEdit::undo);
    connect(redo, &QAction::triggered, textEditor, &QPlainTextEdit::redo);

    auto viewMenu               = menuBar->addMenu("View");
    auto syntaxHightlightToggle = viewMenu->addAction("Toggle Hightlighing");

    syntaxHightlightToggle->connect(syntaxHightlightToggle, &QAction::triggered,
        [this]()
        {
            if(highlighter)
                highlighter->enabled = !highlighter->enabled;
        });

    auto debugMenu  = menuBar->addMenu("Debug");
    auto run        = debugMenu->addAction("Run");
    connect(run, &QAction::triggered, [&] { Run(); });

    auto debug      = debugMenu->addAction("Resume");
    connect(debug, &QAction::triggered, [&] { Resume(); });

    auto compile    = debugMenu->addAction("Compile");
    connect(compile, &QAction::triggered, [&] { Compile(); });

    auto stop = debugMenu->addAction("Stop");
    connect(stop, &QAction::triggered, [&] { Stop(); });

    boxLayout->setMenuBar(menuBar);

    // HotKeys
    auto loadHotKey = new QShortcut(QKeySequence(tr("Ctrl+O", "File|Open")), parent);
    loadHotKey->connect(loadHotKey, &QShortcut::activated, this, &EditorCodeEditor::LoadDocument);

    auto saveHotKey = new QShortcut(QKeySequence(tr("Ctrl+S", "File|Save")), parent);
    saveHotKey->connect(saveHotKey, &QShortcut::activated, this, &EditorCodeEditor::SaveDocument);

    auto saveCopyHotKey = new QShortcut(QKeySequence(tr("Shift+Ctrl+S", "File|SaveCopy")), parent);
    saveCopyHotKey->connect(saveCopyHotKey, &QShortcut::activated, this, &EditorCodeEditor::SaveDocumentCopy);

    auto undoHotKey = new QShortcut(QKeySequence(tr("Ctrl+U", "Edit|Undo")), parent);
    connect(undoHotKey, &QShortcut::activated,
        [&]()
        {
            auto textEditor = static_cast<QPlainTextEdit*>(tabBar->currentWidget());
            textEditor->undo();
        });

    auto redoHotKey = new QShortcut(QKeySequence(tr("Ctrl+R", "Edit|Redo")), parent);
    connect(redoHotKey, &QShortcut::activated,
        [&]()
        {
            auto textEditor = static_cast<QPlainTextEdit*>(tabBar->currentWidget());
            textEditor->redo();
        });

    auto copyHotKey = new QShortcut(QKeySequence(tr("Ctrl+C", "Edit|Copy")), parent);
    connect(copyHotKey, &QShortcut::activated,
        [&]
        {
            auto textEditor = static_cast<QPlainTextEdit*>(tabBar->currentWidget());
            textEditor->copy();
        });

    auto pasteHotKey = new QShortcut(QKeySequence(tr("Ctrl+Z", "Edit|Paste")), parent);
    connect(pasteHotKey, &QShortcut::activated,
        [&]
        {
            auto textEditor = static_cast<QPlainTextEdit*>(tabBar->currentWidget());
            textEditor->paste();
        });

    auto cutHotKey = new QShortcut(QKeySequence(tr("Ctrl+X", "Edit|Cut")), parent);
    connect(cutHotKey, &QShortcut::activated,
        [&]
        {
            auto textEditor = static_cast<QPlainTextEdit*>(tabBar->currentWidget());
            textEditor->cut();
        });
}


/************************************************************************************************/


void EditorCodeEditor::LoadDocument()
{
    textEditor->document()->clear();

    const auto importText   = std::string{ "Load Text File" };
    const auto fileMenuText = std::string{ "Files (*.*)" };
    const auto fileDir      = QFileDialog::getOpenFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());

    QFile file{ fileDir };
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        textEditor->setPlainText(file.readAll());
        file.close();
    }
}


/************************************************************************************************/


void EditorCodeEditor::SaveDocument()
{
    if (currentResource)
    {
        auto document = textEditor->document();
        currentResource->source = document->toPlainText().toStdString();
        std::cout << currentResource->source << "\n";
    }
    else
    {
        if(fileDir.empty())
        {
            const auto importText   = std::string{ "Save Text File" };
            const auto fileMenuText = std::string{ "Files (*.*)" };
            const auto qfileDir     = QFileDialog::getSaveFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());

            fileDir = qfileDir.toStdString();
        }

        QTextDocumentWriter writer{ QString(fileDir.c_str()) };
        writer.write(textEditor->document());
    }
}


/************************************************************************************************/


void EditorCodeEditor::SetResource(ScriptResource_ptr resource)
{
    textEditor->document()->clear();
    textEditor->setPlainText(resource->source.c_str());

    currentResource = resource;
}


/************************************************************************************************/


void EditorCodeEditor::SaveDocumentCopy()
{
    const auto importText   = std::string{ "Save Text File" };
    const auto fileMenuText = std::string{ "Files (*.*)" };
    const auto qfileDir     = QFileDialog::getSaveFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());

    fileDir = qfileDir.toStdString();

    QTextDocumentWriter writer{ qfileDir };
    writer.write(textEditor->document());
}


/************************************************************************************************/


QMenuBar* EditorCodeEditor::GetMenuBar() noexcept
{
    return menuBar;
}


/************************************************************************************************/


QTabWidget* EditorCodeEditor::GetTabs() noexcept
{
    return tabBar;
}


/************************************************************************************************/


EditorCodeEditor::~EditorCodeEditor()
{
    scriptContext->Release();
}


/************************************************************************************************/


void EditorCodeEditor::Resume()
{
    if (scriptContext->GetState() == asEContextState::asEXECUTION_SUSPENDED)
    {
        begin = std::chrono::system_clock::now();
        scriptContext->SetLineCallback(asMETHOD(EditorCodeEditor, LineCallback), this, asCALL_THISCALL);

        scriptContext->Execute();
        callStackWidget->clear();
        variableListWidget->clear();

        scriptContext->ClearLineCallback();
    }
}


/************************************************************************************************/


void EditorCodeEditor::Stop()
{
    if (scriptContext->GetState() == asEContextState::asEXECUTION_SUSPENDED)
    {
        scriptContext->Abort();
        callStackWidget->clear();
        variableListWidget->clear();
    }
}


/************************************************************************************************/


void EditorCodeEditor::Compile()
{
    if (scriptContext->GetState() == asEContextState::asEXECUTION_SUSPENDED)
        return;

    auto code       = textEditor->toPlainText().toStdString();

    scriptContext->SetLineCallback(asMETHOD(EditorCodeEditor, LineCallback), this, asCALL_THISCALL);

    begin = std::chrono::system_clock::now();

    errorListWidget->clear();
    QList<QTextEdit::ExtraSelection> extraSelections;
    bool errorFound     = false;
    auto errorHandler   =
        [&](int row, int col, const char* msg, const char* segment, int errorType)
        {
            if (!textEditor->isReadOnly()) {
                errorFound = true;

                QTextEdit::ExtraSelection selection;

                QColor lineColor = QColor(Qt::darkRed);

                selection.format.setBackground(lineColor);
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                selection.cursor = textEditor->textCursor();
                selection.cursor.movePosition(QTextCursor::Start);
                selection.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row - 1);
                selection.cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 0);
                selection.cursor.clearSelection();
                extraSelections.append(selection);

                errorListWidget->addItem(new QListWidgetItem(fmt::format("{}, {}: {}", row, col, msg).c_str()));
            }
        };

    scriptEngine.CompileString(code, scriptContext, errorHandler);

    if (errorFound)
        textEditor->setExtraSelections(extraSelections);

    scriptContext->ClearLineCallback();
}


/************************************************************************************************/


void EditorCodeEditor::Run()
{
    if (scriptContext->GetState() == asEContextState::asEXECUTION_SUSPENDED)
        return;

    auto code       = textEditor->toPlainText().toStdString();

    scriptContext->SetLineCallback(asMETHOD(EditorCodeEditor, LineCallback), this, asCALL_THISCALL);

    begin = std::chrono::system_clock::now();

    errorListWidget->clear();
    QList<QTextEdit::ExtraSelection> extraSelections;
    bool errorFound     = false;
    auto errorHandler   =
        [&](int row, int col, const char* msg, const char* segment, int errorType)
        {
            if (!textEditor->isReadOnly()) {
                errorFound = true;

                QTextEdit::ExtraSelection selection;

                QColor lineColor = QColor(Qt::darkRed);

                selection.format.setBackground(lineColor);
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                selection.cursor = textEditor->textCursor();
                selection.cursor.movePosition(QTextCursor::Start);
                selection.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row - 1);
                selection.cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 0);
                selection.cursor.clearSelection();
                extraSelections.append(selection);

                errorListWidget->addItem(new QListWidgetItem(fmt::format("{}, {}: {}", row, col, msg).c_str()));
            }
        };

    scriptEngine.RunStdString(code, scriptContext, errorHandler);

    if(errorFound)
        textEditor->setExtraSelections(extraSelections);

    scriptContext->ClearLineCallback();
}


/************************************************************************************************/


void EditorCodeEditor::LineCallback(asIScriptContext* ctx)
{
    auto& breakPoints   = textEditor->lineNumberArea->GetBreakponts();

    const char* scriptSection;
    int line = ctx->GetLineNumber(0, 0, &scriptSection);

    auto now = std::chrono::system_clock::now();

    if ((now - begin) > std::chrono::seconds(1))
    {
        textEditor->AddWarningLine("Infinite Loop Detected!", line);

        QList<QTextEdit::ExtraSelection> extraSelections;

        if (!textEditor->isReadOnly()) {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(Qt::darkRed);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textEditor->textCursor();
            selection.cursor.movePosition(QTextCursor::Start);
            selection.cursor.movePosition(QTextCursor::Down,    QTextCursor::MoveAnchor, line - 1);
            selection.cursor.movePosition(QTextCursor::Right,   QTextCursor::MoveAnchor, 0);
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }

        textEditor->setExtraSelections(extraSelections);

        ctx->Suspend();
        return;
    }


    if (strncmp(scriptSection, "Memory", 7) == 0 &&
        std::find_if(breakPoints.begin(), breakPoints.end(), [&](auto item) { return item.line + 1 == line; }) != breakPoints.end())
    {
        ctx->Suspend();

        callStackWidget->clear();
        variableListWidget->clear();

        for (asUINT stackLevel = 0; stackLevel < ctx->GetCallstackSize(); stackLevel++)
        {
            const char* scriptSection;

            asIScriptFunction*  func = ctx->GetFunction(stackLevel);
            int                 line = ctx->GetLineNumber(stackLevel, nullptr, &scriptSection);

            QListWidgetItem* item = new QListWidgetItem(fmt::format("{}:{}:{}", scriptSection, func->GetDeclaration(), line).c_str());
            callStackWidget->addItem(item);

            int numVars = ctx->GetVarCount(stackLevel);
            for (int variableIdx = 0; variableIdx < numVars; variableIdx++)
            {
                int typeId          = ctx->GetVar(variableIdx, stackLevel, nullptr);
                auto variableDecl   = ctx->GetVarDeclaration(variableIdx, stackLevel);


                switch (typeId)
                {
                case asTYPEID_FLOAT:
                {
                    auto* var = (float*)ctx->GetAddressOfVar(variableIdx, stackLevel);
                    QListWidgetItem* variable = new QListWidgetItem(fmt::format("{} : {}", variableDecl, *var).c_str());
                    variableListWidget->addItem(variable);
                }   break;
                case asTYPEID_INT32:
                {
                    auto* var = (uint32_t*)ctx->GetAddressOfVar(variableIdx, stackLevel);
                    QListWidgetItem* variable = new QListWidgetItem(fmt::format("{} : {}", variableDecl, *var).c_str());
                    variableListWidget->addItem(variable);
                }   break;
                default:
                {
                    if (ctx->GetEngine()->GetTypeInfoByDecl("string")->GetTypeId() == typeId)
                    {
                        auto* var = (std::string*)ctx->GetAddressOfVar(variableIdx, stackLevel);
                        QListWidgetItem* variable = new QListWidgetItem(fmt::format("{} : {}", variableDecl, *var).c_str());
                        variableListWidget->addItem(variable);
                    }
                    else if (ctx->GetEngine()->GetTypeInfoByDecl("float3")->GetTypeId() == typeId)
                    {
                        auto* var = (FlexKit::float3*)ctx->GetAddressOfVar(variableIdx, stackLevel);
                        QListWidgetItem* variable = new QListWidgetItem(fmt::format("{} : {}, {} , {}", variableDecl, var->x, var->y, var->z).c_str());
                        variableListWidget->addItem(variable);
                    }
                    else if (ctx->GetEngine()->GetTypeInfoByDecl("float4")->GetTypeId() == typeId)
                    {
                        auto& var = *((FlexKit::float4*)ctx->GetAddressOfVar(variableIdx, stackLevel));
                        QListWidgetItem* variable = new QListWidgetItem(fmt::format("{} : [ {}, {} , {}, {} ]", variableDecl, var[0], var[1], var[2], var[2]).c_str());
                        variableListWidget->addItem(variable);
                    }
                    else if (ctx->GetEngine()->GetTypeInfoByDecl("float4x4")->GetTypeId() == typeId)
                    {
                        auto& var = *(FlexKit::float4x4*)ctx->GetAddressOfVar(variableIdx, stackLevel);
                        QListWidgetItem* variable = new QListWidgetItem(
                            fmt::format(
                                "{} [{}, {}, {}, {}]\n[{}, {}, {}, {}]\n[{}, {}, {}, {}]\n[{}, {}, {}, {}]\n",
                                variableDecl,
                                var[0][0], var[0][1], var[0][2], var[0][3],
                                var[1][0], var[1][1], var[1][2], var[1][3],
                                var[2][0], var[2][1], var[2][2], var[2][3],
                                var[3][0], var[3][1], var[3][2], var[3][3]
                            ).c_str());

                        variableListWidget->addItem(variable);
                    }
                    else if (ctx->GetEngine()->GetTypeInfoByDecl("Quaternion")->GetTypeId() == typeId)
                    {
                        auto& var = *(FlexKit::Quaternion*)ctx->GetAddressOfVar(variableIdx, stackLevel);
                        QListWidgetItem* variable = new QListWidgetItem(fmt::format("{} : [ I*{}, J*{} , K*{}, {} ]", variableDecl, var[0], var[1], var[2], var[2]).c_str());
                        variableListWidget->addItem(variable);
                    }
                }   break;
                }


            }
        }
    }
}


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
