#pragma once

#include <QtWidgets/QWidget>

class EditorRenderThread : public QObject
{
	Q_OBJECT

public:
	EditorRenderThread(QObject *parent = Q_NULLPTR);
	~EditorRenderThread();

private:
};
