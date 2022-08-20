#pragma once

#include "SelectionContext.h"
#include <map>
#include <qtimer.h>
#include <functional>
#include <QtWidgets/qwidget.h>
#include <QObject>

class QObject;
class QLabel;
class QBoxLayout;
class QScrollArea;
class QWidget;
class QMenuBar;
class QListWidget;

class EditorInspectorView;
class ListUpdateCtx;
class ListEvt;
class QListWidgetItem;

using FieldUpdateCallback	= std::function<void (std::string& string)>;
using FieldChangeCallback	= std::function<void (const std::string& string)>;
using ButtonCallback		= std::function<void ()>;


using ListSizeUpdateCallback	= std::function<size_t()>;
using ListContentUpdateCallback	= std::function<void (size_t, QListWidgetItem*)>;
using ListEventCallback			= std::function<void (QListWidget*)>;


namespace FlexKit
{
	class GameObject;
	class ComponentViewBase;
}

/************************************************************************************************/


class ComponentViewPanelContext
{
public:
	ComponentViewPanelContext(QBoxLayout* panel, std::vector<QObject*>& items_out, std::vector<QBoxLayout*>&, EditorInspectorView*);

	QLabel* AddHeader			(const std::string txt);
	QLabel* AddText				(const std::string txt);
	void    AddInputBox			(const std::string txt, FieldUpdateCallback update, FieldChangeCallback change);

	void			AddButton	(std::string label, ButtonCallback);
	QListWidget*	AddList		(ListSizeUpdateCallback, ListContentUpdateCallback, ListEventCallback);

	void PushVerticalLayout		(std::string groupName = {}, bool goup = false);
	void PushHorizontalLayout	(std::string groupName = {}, bool goup = false);
	void Pop();

	void Refresh();

	EditorInspectorView*		inspector;
	std::vector<QBoxLayout*>	layoutStack;
	std::vector<QObject*>&		propertyItems;
};


/************************************************************************************************/


class IComponentInspector
{
public:
	virtual ~IComponentInspector() {}
	virtual FlexKit::ComponentID ComponentID() = 0;

	virtual std::string ComponentName() const noexcept = 0;
	virtual void Inspect(ComponentViewPanelContext& layout, FlexKit::GameObject&, FlexKit::ComponentViewBase& component) = 0;
};

class ViewportGameObject;
class ViewportScene;

class ComponentConstructionContext
{
public:
	virtual void					AddToScene(FlexKit::GameObject&)	{}
	virtual FlexKit::LayerHandle	GetSceneLayer()						{ return FlexKit::InvalidHandle; }
	virtual uint64_t				GetEditorIdentifier() = 0;
};

class IComponentFactory
{
public:
	virtual ~IComponentFactory() {}

	virtual FlexKit::ComponentViewBase&	Construct(FlexKit::GameObject&, ComponentConstructionContext& scene) = 0;
	virtual const std::string&			ComponentName() const noexcept = 0;
	virtual FlexKit::ComponentID		ComponentID() const noexcept = 0;
};


/************************************************************************************************/


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


	static void AddComponentFactory(std::unique_ptr<IComponentFactory>&& factory)
	{
		availableComponents.emplace_back(std::move(factory));
	}

	static FlexKit::ComponentViewBase& ConstructComponent(uint32_t ComponentID, ViewportGameObject& gameObject, ComponentConstructionContext& scene);

	 SelectionContext* GetSelectionContext() { return &selectionContext; }

	 void ClearPanel();

private:

	void UpdatePropertiesViewportObjectInspector();
	void UpdateAnimatorObjectInspector();
	void UpdateUI(FlexKit::GameObject&);

	void timerEvent(QTimerEvent*) override;
	void OnUpdate();

	inline static std::map<FlexKit::ComponentID, std::unique_ptr<IComponentInspector>>  componentInspectors = {};
	inline static std::vector<std::unique_ptr<IComponentFactory>>                       availableComponents = {};

	QTimer*					timer = new QTimer{ this };
	QMenuBar*				menu;

	SelectionContext&		selectionContext;
	uint64_t				selectedObject  = -1;
	uint64_t				propertyCount   = 0;

	QBoxLayout*		outerLayout;
	QBoxLayout*		contentLayout;
	QScrollArea*	scrollArea;
	QWidget*		contentWidget;

	FlexKit::Signal<void()>::Slots slot;

	std::vector<QBoxLayout*>	properties;
	std::vector<QObject*>		propertyItems;
};


/************************************************************************************************/
