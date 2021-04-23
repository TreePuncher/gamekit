#pragma once

#include "ui_EditorInspectorView.h"
#include "SelectionContext.h"

#include <map>
#include <qtimer.h>
#include <QtWidgets/qwidget.h>
#include <functional>

class EditorInspectorView;
class QLabel;

using FieldChangeCallback = std::function<void (const std::string& string)>;

class ComponentViewPanelContext
{
public:
    ComponentViewPanelContext(QBoxLayout* panel, std::vector<QWidget*>& items_out, std::vector<QBoxLayout*>&);

    QLabel* AddHeader      (std::string txt);
    QLabel* AddText        (std::string txt);
    void    AddInputBox    (std::string label, std::string initial, FieldChangeCallback);

    void PushVerticalLayout();
    void PushHorizontalLayout();
    void Pop();

    std::vector<QBoxLayout*>    layoutStack;
    std::vector<QWidget*>&      propertyItems;
    std::vector<QBoxLayout*>&   subLayouts;
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

    template<typename TY_inspector, typename ... TY_args>
    static void AddComponentInspector(TY_args&& ... args)
    {
        auto componentInspector = std::make_unique<TY_inspector>(std::forward<TY_args>(args)...);
        componentInspectors[componentInspector->ComponentID()] = std::move(componentInspector);
    }

private:

    void    timerEvent(QTimerEvent*) override;
    void    OnUpdate();

    inline static std::map<FlexKit::ComponentID, std::unique_ptr<IComponentInspector>> componentInspectors;

    QTimer*                 timer = new QTimer{ this };
	Ui::EditorInspectorView ui;
    SelectionContext&       selectionContext;
    uint64_t                selectedObject = -1;

    std::vector<QBoxLayout*>    properties;
    std::vector<QWidget*>       propertyItems;
};
