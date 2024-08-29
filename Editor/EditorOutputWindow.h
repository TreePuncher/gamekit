#pragma once

#include <QtWidgets/qwidget.h>
#include "ui_EditorOutputWindow.h"
#include <functional>
#include <qtimer.h>

using FNUpdateOutputText = std::function<const std::string& ()>;

class EditorOutputWindow : public QWidget
{
	Q_OBJECT

public:
	EditorOutputWindow(QWidget *parent = Q_NULLPTR);
	~EditorOutputWindow();

    void SetInputSource(FNUpdateOutputText fnUpdateText, size_t idx)
    {
        sources[idx] = fnUpdateText;
    }

public slots:

    void OnChanged(int);
    void UpdateText();
private:
	Ui::EditorOutputWindow  ui;
    size_t                  mode = 0;
    FNUpdateOutputText      sources[3];
    QTimer*                 timer;
};
