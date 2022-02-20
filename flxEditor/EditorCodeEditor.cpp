#include "PCH.h"
#include "EditorCodeEditor.h"
#include "QtWidgets/qabstractbutton.h"
#include "QtWidgets/qmenubar.h"

#include <qsyntaxhighlighter.h>
#include <qregularexpression.h>
#include <qfile.h>
#include <QtWidgets/qfiledialog.h>
#include <qtextdocumentwriter.h>
#include <qshortcut.h>


/************************************************************************************************/


class BasicHighlighter : public QSyntaxHighlighter
{
public:
    BasicHighlighter(QTextDocument* document) : QSyntaxHighlighter{document}
    {
        HighlightingRule rule;

        keywordFormat.setForeground(Qt::darkBlue);
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

        quotationFormat.setForeground(Qt::darkGreen);
        rule.pattern    = QRegularExpression(QStringLiteral("\".*\""));
        rule.format     = quotationFormat;
        highlightingRules.append(rule);

        functionFormat.setFontItalic(true);
        functionFormat.setForeground(Qt::blue);
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


EditorCodeEditor::EditorCodeEditor(EditorScriptEngine& IN_scriptEngine, QWidget *parent)
    : QWidget(parent), scriptEngine{ IN_scriptEngine }
{
	ui.setupUi(this);

    auto boxLayout  = findChild<QBoxLayout*>("verticalLayout");
    textEditor      = findChild<QPlainTextEdit*>("plainTextEdit");

    textEditor->setTabStopDistance(textEditor->tabStopDistance() / 2);
    highlighter = new BasicHighlighter{ textEditor->document() };

    // Setup Menu
    auto menuBar                = new QMenuBar();
    menuBar->setVisible(true);

    auto fileMenu               = menuBar->addMenu("File");

    auto fileLoad               = fileMenu->addAction("Load File");
    fileLoad->connect(fileLoad, &QAction::triggered, this, &EditorCodeEditor::LoadDocument);

    auto fileSave           = fileMenu->addAction("Save File");
    fileSave->connect(fileSave, &QAction::triggered, this, &EditorCodeEditor::SaveDocument);

    auto fileSaveCopy = fileMenu->addAction("Save File Copy");
    fileSave->connect(fileSave, &QAction::triggered, this, &EditorCodeEditor::SaveDocumentCopy);


    auto editMenu               = menuBar->addMenu("Edit");
    undo = editMenu->addAction("Undo");
    redo = editMenu->addAction("Redo");

    undo->setEnabled(textEditor->document()->availableUndoSteps() > 0);
    redo->setEnabled(textEditor->document()->availableRedoSteps() > 0);

    textEditor->document()->connect(textEditor->document(), &QTextDocument::redoAvailable, redo, &QAction::setEnabled);
    textEditor->document()->connect(textEditor->document(), &QTextDocument::undoAvailable, undo, &QAction::setEnabled);

    undo->connect(undo, &QAction::triggered, textEditor, &QPlainTextEdit::undo);
    redo->connect(redo, &QAction::triggered, textEditor, &QPlainTextEdit::redo);

    auto viewMenu               = menuBar->addMenu("View");
    auto syntaxHightlightToggle = viewMenu->addAction("Toggle Hightlighing");

    syntaxHightlightToggle->connect(syntaxHightlightToggle, &QAction::triggered,
        [this]()
        {
            if(highlighter)
                highlighter->enabled = !highlighter->enabled;
        });

    auto debugMenu = menuBar->addMenu("Debug");
    auto run        = debugMenu->addAction("Run");
    run->connect(run, &QAction::triggered, [&] { RunCode(); });

    boxLayout->setMenuBar(menuBar);


    // HotKeys
    auto loadHotKey = new QShortcut(QKeySequence(tr("Ctrl+O", "File|Open")), parent);
    loadHotKey->connect(loadHotKey, &QShortcut::activated, this, &EditorCodeEditor::LoadDocument);

    auto saveHotKey = new QShortcut(QKeySequence(tr("Ctrl+S", "File|Save")), parent);
    saveHotKey->connect(saveHotKey, &QShortcut::activated, this, &EditorCodeEditor::SaveDocument);

    auto saveCopyHotKey = new QShortcut(QKeySequence(tr("Shift+Ctrl+S", "File|SaveCopy")), parent);
    saveCopyHotKey->connect(saveCopyHotKey, &QShortcut::activated, this, &EditorCodeEditor::SaveDocumentCopy);

    auto undoHotKey = new QShortcut(QKeySequence(tr("Ctrl+U", "Edit|Undo")), parent);
    undoHotKey->connect(undoHotKey, &QShortcut::activated, textEditor, &QPlainTextEdit::undo);

    auto redoHotKey = new QShortcut(QKeySequence(tr("Ctrl+R", "Edit|Redo")), parent);
    redoHotKey->connect(redoHotKey, &QShortcut::activated, textEditor, &QPlainTextEdit::redo);

    auto copyHotKey = new QShortcut(QKeySequence(tr("Ctrl+C", "Edit|Copy")), parent);
    copyHotKey->connect(redoHotKey, &QShortcut::activated, textEditor, &QPlainTextEdit::copy);

    auto pasteHotKey = new QShortcut(QKeySequence(tr("Ctrl+Z", "Edit|Paste")), parent);
    pasteHotKey->connect(redoHotKey, &QShortcut::activated, textEditor, &QPlainTextEdit::paste);

    auto cutHotKey = new QShortcut(QKeySequence(tr("Ctrl+X", "Edit|Cut")), parent);
    cutHotKey->connect(redoHotKey, &QShortcut::activated, textEditor, &QPlainTextEdit::cut);

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


/************************************************************************************************/


void EditorCodeEditor::SaveDocumentCopy()
{
    const auto importText = std::string{ "Save Text File" };
    const auto fileMenuText = std::string{ "Files (*.*)" };
    const auto qfileDir = QFileDialog::getSaveFileName(this, tr(importText.c_str()), QDir::currentPath(), fileMenuText.c_str());

    fileDir = qfileDir.toStdString();

    QTextDocumentWriter writer{ qfileDir };
    writer.write(textEditor->document());
}


/************************************************************************************************/


EditorCodeEditor::~EditorCodeEditor()
{
}


/************************************************************************************************/


void EditorCodeEditor::RunCode()
{
    auto code = textEditor->toPlainText().toStdString();

    scriptEngine.RunStdString(code);
}


/************************************************************************************************/
