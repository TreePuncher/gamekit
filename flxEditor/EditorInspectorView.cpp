#include "PCH.h"
#include "EditorInspectorView.h"
#include "EditorSelectedPrefabObject.h"
#include "ViewportScene.h"
#include "qtoolbox.h"

/************************************************************************************************/


ComponentViewPanelContext::ComponentViewPanelContext(QBoxLayout* panel, std::vector<QObject*>& items_out, std::vector<QBoxLayout*>& layouts, EditorInspectorView* IN_inspector)
	: inspector		{ IN_inspector }
	, propertyItems	{ items_out }
{
	layoutStack.push_back(panel);
}


/************************************************************************************************/


QLabel* ComponentViewPanelContext::AddHeader(const std::string txt)
{
	auto label = new QLabel{ txt.c_str() };
	layoutStack.back()->addWidget(label, 0, Qt::AlignHCenter);

	propertyItems.push_back(label);

	return label;
}


/************************************************************************************************/


QLabel* ComponentViewPanelContext::AddText(const std::string txt)
{
	auto label = new QLabel{ txt.c_str() };
	label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	layoutStack.back()->addWidget(label);

	propertyItems.push_back(label);

	return label;
}


/************************************************************************************************/


void ComponentViewPanelContext::AddInputBox(const std::string txt, FieldUpdateCallback update, FieldChangeCallback change)
{
	std::string initial;
	update(initial);

	auto label      = new QLabel{ txt.c_str() };
	auto inputBox   = new QTextEdit{ initial.c_str() };

	label->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
	inputBox->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Fixed);
	inputBox->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
	inputBox->setBaseSize(QSize{ 10, 20 });
	inputBox->setFixedHeight(28);

	inputBox->connect(
		inputBox,
		&QTextEdit::textChanged,
		[=] { change(inputBox->toPlainText().toStdString()); });

	auto timer = new QTimer{ inputBox };

	timer->start(250);
	timer->connect(timer, &QTimer::timeout,
		[=]
		{
			std::string newText;
			update(newText);

			if(newText != inputBox->toPlainText().toStdString())
				inputBox->setPlainText(newText.c_str());
		});

	PushHorizontalLayout();

	layoutStack.back()->addWidget(label);
	layoutStack.back()->addWidget(inputBox);

	Pop();

	propertyItems.push_back(label);
	propertyItems.push_back(inputBox);
	propertyItems.push_back(timer);
}


/************************************************************************************************/


void ComponentViewPanelContext::AddButton(std::string label, ButtonCallback callback)
{
	auto button = new QPushButton(label.c_str());

	button->connect(
		button,
		&QPushButton::pressed,
		callback);

	layoutStack.back()->addWidget(button);
	propertyItems.push_back(button);
}


/************************************************************************************************/


void ComponentViewPanelContext::PushVerticalLayout(std::string groupName, bool pushGroup)
{
	auto layout = new QBoxLayout{ QBoxLayout::Down };
	layout->setAlignment(Qt::AlignTop);
	layout->setSizeConstraint(QLayout::SetFixedSize);

	if(pushGroup)
	{
		auto group  = new QGroupBox{};

		if (groupName.size())
			group->setTitle(groupName.c_str());

		group->setLayout(layout);

		propertyItems.push_back(group);
		layoutStack.back()->addWidget(group);
	}
	else
		layoutStack.back()->addLayout(layout);

	layoutStack.push_back(layout);
}


/************************************************************************************************/


void ComponentViewPanelContext::PushHorizontalLayout(std::string groupName, bool pushGroup)
{
	auto layout = new QBoxLayout{ QBoxLayout::LeftToRight };
	layout->setAlignment(Qt::AlignLeft);

	if(pushGroup)
	{
		auto group  = new QGroupBox{};

		if(groupName.size())
			group->setTitle(groupName.c_str());

		group->setLayout(layout);

		propertyItems.push_back(group);
		layoutStack.back()->addWidget(group);
	}
	else
		layoutStack.back()->addLayout(layout);

	layoutStack.push_back(layout);
}


/************************************************************************************************/


void ComponentViewPanelContext::Pop()
{
	layoutStack.pop_back();
}


/************************************************************************************************/


void ComponentViewPanelContext::Refresh()
{
	inspector->ClearPanel();
}


/************************************************************************************************/


QListWidget* ComponentViewPanelContext::AddList(ListSizeUpdateCallback size, ListContentUpdateCallback content, ListEventCallback evt)
{
	auto list = new QListWidget();

	const auto end = size();
	for (size_t itr = 0; itr < end; ++itr)
	{
		QListWidgetItem* item = new QListWidgetItem;
		content(itr, item);
		list->addItem(item);
	}

	list->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

	list->connect(list, &QListWidget::itemSelectionChanged, [=]() { evt(list); });
	list->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

	auto timer = new QTimer{ list };

	timer->connect(
		timer,
		&QTimer::timeout,
		[=, currentSize = 0]() mutable
		{

			const auto end = size();
			if (currentSize != size())
			{
				list->clear();

				for (size_t itr = 0; itr < end; ++itr)
				{
					QListWidgetItem* item = new QListWidgetItem;
					content(itr, item);
					list->addItem(item);
				}
				currentSize = size();
			}


			timer->start(250);
		});

	timer->start(250);
	layoutStack.back()->addWidget(list);

	propertyItems.push_back(timer);
	propertyItems.push_back(list);


	return list;
}


/************************************************************************************************/


EditorInspectorView::EditorInspectorView(SelectionContext& IN_selectionContext, QWidget *parent) :
	QWidget				{ parent },
	selectionContext	{ IN_selectionContext },
	menu				{ new QMenuBar{} },
	outerLayout			{ new QVBoxLayout{} },
	scrollArea			{ new QScrollArea{} },
	contentLayout		{ new QVBoxLayout{} },
	contentWidget		{ new QWidget{} }
{
	auto addComponentMenu = menu->addMenu("Add");

	for (auto& factory : availableComponents)
	{
		auto action = addComponentMenu->addAction(factory->ComponentName().c_str());
		action->connect(action, &QAction::triggered,
			[&]()
			{
				switch(selectionContext.GetSelectionType())
				{
				case ViewportObjectList_ID:
				{
					ViewportSelection selection = selectionContext.GetSelection<ViewportSelection>();
					struct ViewportPanelContext : public ComponentConstructionContext
					{
						ViewportPanelContext(ViewportSelection& IN_selection) : selection{ IN_selection } {}

						void AddToScene(FlexKit::GameObject& go) final
						{
							selection.scene->scene.AddGameObject(go);
						}

						FlexKit::LayerHandle GetSceneLayer() final
						{
							return selection.scene->physicsLayer;
						}

						uint64_t GetEditorIdentifier() final
						{
							return selection.viewportObjects.front()->objectID;
						}

						ViewportSelection& selection;
					} ctx{ selection };

					if(selection.viewportObjects.size())
						factory->Construct(*selection.viewportObjects.front(), ctx);
				}	break;
				case AnimatorObject_ID:
				{
					auto selection = selectionContext.GetSelection<EditorSelectedPrefabObject*>();
					
					struct AnimatorPanelContext : public ComponentConstructionContext
					{
						AnimatorPanelContext(EditorSelectedPrefabObject* IN_obj) : selection{ IN_obj } {}

						void AddToScene(FlexKit::GameObject& gameObject) {}

						FlexKit::LayerHandle GetSceneLayer()
						{
							return selection->layer;
						}

						uint64_t GetEditorIdentifier() final
						{
							return selection->ID;
						}

						EditorSelectedPrefabObject* selection;
					} ctx{ selection };

					if(selection)
						factory->Construct(selection->gameObject, ctx);
				}	break;
				default:
					break;
				}
			});
	}

	setLayout(outerLayout);
	outerLayout->addWidget(scrollArea);
	outerLayout->setMenuBar(menu);

	scrollArea->setWidgetResizable(true);
	scrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	scrollArea->setWidget(contentWidget);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);

	contentLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
	contentLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	contentWidget->setLayout(contentLayout);
	
	ComponentViewPanelContext context{ contentLayout, propertyItems, properties, this };

	timer->start(100);

	connect(timer, &QTimer::timeout, this, &EditorInspectorView::OnUpdate);

	selectionContext.OnChange.Connect(
		slot,
		[&]()
		{
			ClearPanel();
		});
}


FlexKit::ComponentViewBase& EditorInspectorView::ConstructComponent(uint32_t componentID, ViewportGameObject& gameObject, ComponentConstructionContext& ctx)
{
	for (auto& component : availableComponents)
	{
		if (component->ComponentID() == componentID)
			return component->Construct(gameObject, ctx);
	 }

	throw std::runtime_error("Unknown Component");
}


/************************************************************************************************/


EditorInspectorView::~EditorInspectorView()
{
}


/************************************************************************************************/


void EditorInspectorView::timerEvent(QTimerEvent*)
{
	OnUpdate();
}


/************************************************************************************************/


void EditorInspectorView::UpdatePropertiesViewportObjectInspector()
{

	auto selection					= selectionContext.GetSelection<ViewportSelection>().viewportObjects.front();
	uint64_t objectID				= selection->objectID;

	auto& gameObject				= selection->gameObject;
	auto gameObjectPropertyCount	= std::distance(gameObject.begin(), gameObject.end());

	if( objectID        != selectedObject ||
		propertyCount   != gameObjectPropertyCount)
	{
		menu->setEnabled(true);

		selectedObject  = objectID;
		propertyCount   = gameObjectPropertyCount;

		UpdateUI(gameObject);
	}
}


/************************************************************************************************/


void EditorInspectorView::UpdateAnimatorObjectInspector()
{
	auto selection      = selectionContext.GetSelection<EditorSelectedPrefabObject*>();
	uint64_t objectID   = selection->ID;
	auto& gameObject    = selection->gameObject;

	auto gameObjectPropertyCount = std::distance(gameObject.begin(), gameObject.end());

	if (objectID        != selectedObject ||
		propertyCount   != gameObjectPropertyCount)
	{
		selectedObject  = objectID;
		propertyCount   = gameObjectPropertyCount;

		UpdateUI(gameObject);
	}
}


/************************************************************************************************/


void clearLayout(QLayout* layout) {
	if (layout == NULL)
		return;
	QLayoutItem* item;
	while ((item = layout->takeAt(0))) {
		if (item->layout()) {
			clearLayout(item->layout());
			item->layout()->setParent(nullptr);
			delete item->layout();
		}
		else if (item->widget()) {
			item->widget()->hide();
			item->widget()->setParent(nullptr);
			delete item->widget();
		}
		else
			delete item;
	}
}

void EditorInspectorView::ClearPanel()
{
	if (selectedObject == -1)
		return;

	auto res = findChild<QLabel*>("Nothing Selected");

	auto childCount = contentLayout->children().size();

	propertyItems.clear();

	clearLayout(contentLayout);

	QLabel* label = new QLabel{};
	label->setText("Nothing Selected");
	label->setObjectName("Nothing Selected");
	label->setAccessibleName("Nothing Selected");
	label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	contentLayout->addWidget(label);

	propertyItems.push_back(label);

	selectedObject  = -1;
	propertyCount   = 0;
}


/************************************************************************************************/


void EditorInspectorView::UpdateUI(FlexKit::GameObject& gameObject)
{
	auto children = contentWidget->children();
	for (auto child : children)
		if (child != contentLayout)
			delete child;

	auto children2 = contentWidget->children();

	properties.clear();
	propertyItems.clear();

	const char* ID		= FlexKit::GetStringID(gameObject);
	const size_t ID_len = strnlen_s(ID, 32);

	QLabel* header = (ID_len) ?
		new QLabel{ QString{"GameObject: %1"}.arg(ID) } :
		new QLabel{ QString{"GameObject#%1"}.arg(selectedObject) };

	header->setObjectName("PropertyItem");

	contentLayout->addWidget(header);
	propertyItems.push_back(header);

	auto* toolBox = new QToolBox{};
	contentLayout->addWidget(toolBox);

	for (auto& componentView : gameObject)
	{
		auto layout		= new QBoxLayout{ QBoxLayout::Down };
		auto scrollable	= new QScrollArea{};

		scrollable->setLayout(layout);
		scrollable->setContextMenuPolicy(Qt::CustomContextMenu);
		
		properties.push_back(layout);
		layout->setObjectName("Properties");
		
		ComponentViewPanelContext context{ layout, propertyItems, properties, this };

		if (auto res = componentInspectors.find(componentView.GetID()); res != componentInspectors.end())
		{
			toolBox->addItem(scrollable, fmt::format("{}", res->second->ComponentName()).c_str());

			res->second->Inspect(context, gameObject, componentView.Get_ref());
		}
		else
		{
			toolBox->addItem(scrollable, fmt::format("Unknown Component: {}", componentView.GetID()).c_str());

			context.PushVerticalLayout(QString{ "Component ID#%1: " }.arg(componentView.GetID()).toStdString(), true);
			context.AddText("No Component Inspector Available");
			context.Pop();
		}

		scrollable->connect(scrollable, &QScrollArea::customContextMenuRequested,
			[=, inspector = context.inspector, &gameObject, component = componentView.Get()](const QPoint& point)
			{
				auto menu			= new QMenu{ tr("Context menu"), scrollable };
				auto removeAction	= new QAction{ "Remove", this };

				menu->connect(removeAction, &QAction::triggered,
					[&, removeAction]()
					{
						gameObject.RemoveView(component);
						menu->removeAction(removeAction);
					});

				menu->addAction(removeAction);
				menu->exec(scrollable->mapToGlobal(point));
			});

		scrollable->setSizePolicy(QSizePolicy{ QSizePolicy::MinimumExpanding,  QSizePolicy::Minimum });
		layout->setSizeConstraint(QLayout::SetMinimumSize);
		layout->setAlignment(Qt::AlignTop);
	}
}


/************************************************************************************************/


void EditorInspectorView::OnUpdate()
{
	switch(selectionContext.GetSelectionType())
	{
	case ViewportObjectList_ID:
		UpdatePropertiesViewportObjectInspector();
		break;
	case AnimatorObject_ID:
		UpdateAnimatorObjectInspector();
		break;
	default:
	{
		ClearPanel();
	}   break;
	};

	timer->start(100);
}


/************************************************************************************************/
