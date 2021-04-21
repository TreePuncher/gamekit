#include "EditorInspectorView.h"
#include "ViewportScene.h"
#include <qtimer>
#include <QtWidgets\qtextedit.h>
#include <QtWidgets\qlabel.h>
#include <QtWidgets\qboxlayout.h>


/************************************************************************************************/


EditorInspectorView::EditorInspectorView(SelectionContext& IN_selectionContext, QWidget *parent) :
    QWidget             { parent },
    selectionContext    { IN_selectionContext }
{
	ui.setupUi(this);

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

            ui.verticalLayout->addWidget(header);
            propertyItems.push_back(header);


            for (auto& componentView : gameObject)
            {
                auto layout = new QBoxLayout{ QBoxLayout::LeftToRight };
                properties.push_back(layout);
                layout->setObjectName("Properties");
                ui.verticalLayout->addLayout(layout);

                ComponentViewPanelContext context{ layout, propertyItems };

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
                    layout->addWidget(componentLabel);
                    layout->addWidget(label);

                    propertyItems.push_back(label);
                    propertyItems.push_back(componentLabel);
                }
            }
        }
    }
    else
    {
        auto res = findChild<QLabel*>("Nothing Selected");

        if (!res && children().size() > 2)
        {
            while (children().size() > 2)
            {
                auto child = children().at(0);
                if (child->isWidgetType())
                {
                    child->setParent(nullptr);
                    delete child;
                }
                auto temp = children().size();
                int x = 0;
            }
        }

        if (!res)
        {
            QLabel* label = new QLabel{};
            label->setText("Nothing Selected");
            label->setObjectName("Nothing Selected");
            label->setAccessibleName("Nothing Selected");

            ui.verticalLayout->addWidget(label);
        }
    }

    timer->start(100);
}


/************************************************************************************************/
