#pragma once

#include "ui_EditorInspectorView.h"
#include "SelectionContext.h"

#include <map>
#include <qtimer.h>
#include <QtWidgets/qwidget.h>

class EditorInspectorView;

class ComponentViewPanelContext
{
public:
    void AddLabel       (std::string label){}
    void AddInputBox    (std::string label){}

    QBoxLayout*             layout;
    std::vector<QWidget*>&  propertyItems;
};

class IComponentInspector
{
public:
    virtual ~IComponentInspector() {}
    virtual FlexKit::ComponentID ComponentID() = 0;

    virtual void Inspect(ComponentViewPanelContext& layout, FlexKit::ComponentViewBase& component) = 0;
};

class EditorInspectorView : public QWidget
{
	Q_OBJECT

public:
	EditorInspectorView(SelectionContext& selectionContext, QWidget *parent = Q_NULLPTR);
	~EditorInspectorView();

private:

    void    timerEvent(QTimerEvent*) override;
    void    OnUpdate();

    inline static std::map<FlexKit::ComponentID, std::shared_ptr<IComponentInspector>> componentInspectors;

    QTimer*                 timer = new QTimer{ this };
	Ui::EditorInspectorView ui;
    SelectionContext&       selectionContext;
    uint64_t                selectedObject = -1;

    std::vector<QBoxLayout*>    properties;
    std::vector<QWidget*>       propertyItems;
};
