#include "EditorInspectorView.h"
#include "ViewportScene.h"
#include <qtimer>
#include <QtWidgets\qtextedit.h>
#include <QtWidgets\qboxlayout.h>
#include <QtWidgets\qlabel.h>
#include <QtWidgets\qpushbutton.h>
#include <QtWidgets\qmenubar.h>
#include <QtWidgets\qgroupbox.h>
#include <QtWidgets\qscrollarea.h>


/************************************************************************************************/


ComponentViewPanelContext::ComponentViewPanelContext(QBoxLayout* panel, std::vector<QWidget*>& items_out, std::vector<QBoxLayout*>& layouts) :
    propertyItems   { items_out },
    subLayouts      { layouts }
{
    layoutStack.push_back(panel);
}


/************************************************************************************************/


QLabel* ComponentViewPanelContext::AddHeader(std::string txt)
{
    auto label = new QLabel{ txt.c_str() };
    layoutStack.back()->addWidget(label, 0, Qt::AlignHCenter);

    propertyItems.push_back(label);

    return label;
}


/************************************************************************************************/


QLabel* ComponentViewPanelContext::AddText(std::string txt)
{
    auto label = new QLabel{ txt.c_str() };
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layoutStack.back()->addWidget(label);

    propertyItems.push_back(label);

    return label;
}

/************************************************************************************************/


void ComponentViewPanelContext::AddInputBox(std::string txt, FieldUpdateCallback update, FieldChangeCallback change)
{
    std::string initial;
    update(initial);

    auto label      = new QLabel{ txt.c_str() };
    auto inputBox   = new QTextEdit{ initial.c_str() };

    label->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    inputBox->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    inputBox->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);

    inputBox->connect(
        inputBox,
        &QTextEdit::textChanged,
        [=] { change(inputBox->toPlainText().toStdString()); });

    auto timer = new QTimer{ inputBox };
    timer->start(250);
    timer->connect(timer, &QTimer::timeout,
        [=] {
                std::string newText;
                update(newText);

                inputBox->setPlainText(newText.c_str());
            });

    PushHorizontalLayout();

    layoutStack.back()->addWidget(label);
    layoutStack.back()->addWidget(inputBox);

    Pop();

    propertyItems.push_back(label);
    propertyItems.push_back(inputBox);
}


/************************************************************************************************/


void ComponentViewPanelContext::AddButton(std::string label, ButtonCallback callback)
{
    auto button = new QPushButton();

    button->connect(
        button,
        &QPushButton::pressed,
        callback);

    //button->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);

    layoutStack.back()->addWidget(button);
}


/************************************************************************************************/


void ComponentViewPanelContext::PushVerticalLayout(std::string groupName, bool pushGroup)
{
    auto layout = new QBoxLayout{ QBoxLayout::Down };
    layout->setAlignment(Qt::AlignTop);

    if(pushGroup)
    {
        auto group  = new QGroupBox{};

        if (groupName.size())
            group->setTitle("Hello");

        group->setLayout(layout);

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
        layoutStack.back()->addWidget(group);
    }
    else
        layoutStack.back()->addLayout(layout);

    layoutStack.push_back(layout);
}


/************************************************************************************************/


void ComponentViewPanelContext::Pop()
{
    //layoutStack.back()->addStretch();
    layoutStack.pop_back();
}


/************************************************************************************************/



EditorInspectorView::EditorInspectorView(SelectionContext& IN_selectionContext, QWidget *parent) :
    QWidget             { parent },
    selectionContext    { IN_selectionContext },
    menu                { new QMenuBar{} },
    outerLayout         { new QVBoxLayout{} },
    scrollArea          { new QScrollArea{} },
    contentLayout       { new QVBoxLayout{} },
    contentWidget       { new QWidget{} }
{
    menu->setEnabled(false);

    auto addComponentMenu   = menu->addMenu("Add");

    for (auto& factory : availableComponents)
    {
        auto action = addComponentMenu->addAction(factory->ComponentName().c_str());
        action->connect(action, &QAction::triggered,
            [&]()
            {
                if (selectionContext.GetSelectionType() == ViewportObjectList_ID)
                    factory->Construct(*selectionContext.GetSelection<ViewportObjectList>().front());
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
    
    ComponentViewPanelContext context{ contentLayout, propertyItems, properties };

    timer->start(100);

    connect(timer, &QTimer::timeout, this, &EditorInspectorView::OnUpdate);
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


void EditorInspectorView::OnUpdate()
{
    if (selectionContext.GetSelectionType() == ViewportObjectList_ID)
    {
        auto selection      = selectionContext.GetSelection<ViewportObjectList>().front();
        uint64_t objectID   = selection->objectID;
        auto& gameObject    = selection->gameObject;

        
        if (auto res = findChild<QLabel*>("Nothing Selected"); res)
        {
            res->setParent(nullptr);
            menu->setEnabled(true);

            delete res;
        }

        if(objectID != selectedObject)
        {
            selectedObject = objectID;

            for (auto& layout : properties)
            {
                layout->setParent(nullptr);
                delete layout;
            }

            for (auto& items : propertyItems)
            {
                items->setParent(nullptr);
                delete items;
            }

            properties.clear();
            propertyItems.clear();

            const char* ID      = FlexKit::GetStringID(gameObject);
            const size_t ID_len = strnlen_s(ID, 32);

            QLabel* header = (ID_len) ?
                new QLabel{ QString{"GameObject: %1"}.arg(ID) } :
                new QLabel{ QString{"GameObject#%1"}.arg(selectedObject) };

            header->setObjectName("PropertyItem");

            contentLayout->addWidget(header);
            propertyItems.push_back(header);


            for (auto& componentView : gameObject)
            {
                auto layout = new QBoxLayout{ QBoxLayout::Down };

                properties.push_back(layout);
                layout->setObjectName("Properties");
                contentLayout->addLayout(layout);

                ComponentViewPanelContext context{ layout, propertyItems, properties };

                if (auto res = componentInspectors.find(componentView.ID); res != componentInspectors.end())
                {
                    res->second->Inspect(context, componentView.Get_ref());
                }
                else
                {
                    auto componentLabel = new QLabel{ QString{"Component ID#%1: "}.arg(componentView.ID) };
                    auto label          = new QLabel{};

                    label->setObjectName("PropertyItem");
                    label->setText("No Component Inspector Available");
                    layout->addWidget(componentLabel, 0, Qt::AlignHCenter);
                    layout->addWidget(label);

                    propertyItems.push_back(label);
                    propertyItems.push_back(componentLabel);
                }
            }
        }
        else
        {
            for (auto& property : propertyItems)
            {

            }
        }
    }
    else
    {
        auto res = findChild<QLabel*>("Nothing Selected");

        auto childCount = contentLayout->children().size();

        if (!res && contentLayout->children().size() > 2)
        {
            for(auto child : contentLayout->children())
            {
                if (child->isWidgetType())
                {
                    child->setParent(nullptr);
                    delete child;
                }
            }
        }

        if (!res)
        {
            QLabel* label = new QLabel{};
            label->setText("Nothing Selected");
            label->setObjectName("Nothing Selected");
            label->setAccessibleName("Nothing Selected");
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            contentLayout->addWidget(label);
            //contentLayout->addStretch();
        }
    }

    timer->start(100);
}


/************************************************************************************************/
