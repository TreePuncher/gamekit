#include "EditorOutputWindow.h"
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qtextbrowser.h>
#include <chrono>

using namespace std::literals::chrono_literals;

EditorOutputWindow::EditorOutputWindow(QWidget* parent)
    : QWidget{ parent }, timer{ new QTimer }
{
    ui.setupUi(this);

    auto comboBox = findChild<QComboBox*>("outputMode");
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int idx) { OnChanged(idx); });
    connect(timer, &QTimer::timeout, [&]() { UpdateText(); });

    timer->setInterval(100ms);
    timer->start();

}

EditorOutputWindow::~EditorOutputWindow()
{
}


void EditorOutputWindow::OnChanged(int idx)
{
    mode = idx;
}


void EditorOutputWindow::UpdateText()
{
    if (mode != -1 && sources[mode])
    {
        auto    textBrowser     = findChild<QTextBrowser*>("outputWindow");
        auto&   text            = sources[mode]();
        auto    outputText     = QString::fromStdString(text);
        textBrowser->setText(outputText);
    }
}
