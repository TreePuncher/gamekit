#include "PCH.h"
#include "CSGComponent.h"
#include "ViewportScene.h"
#include "EditorInspectorView.h"
#include "qlistwidget.h"
#include <QtWidgets\qlistwidget.h>


/************************************************************************************************/


struct CSGComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& editorCSGComponent = static_cast<EditorComponentCSG&>(component);

    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<CSGComponentUpdate, CSGComponentID> _register;
};


/************************************************************************************************/


class CSGInspector : public IComponentInspector
{
public:

    FlexKit::ComponentID ComponentID() override
    {
        return CSGComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& view) override
    {
        CSGView& csgView = static_cast<CSGView&>(view);

        panelCtx.PushVerticalLayout("CSG", true);

        panelCtx.AddButton("Add",
            [&]()
            {
                CSGNode newNode;
                newNode.op = CSG_OP::CSG_ADD;
                csgView.GetData().nodes.emplace_back(newNode);
            });

        panelCtx.AddList(
            [&]() -> size_t
            {    // Update Size
                return csgView.GetData().nodes.size();
            }, 
            [&](auto idx, QListWidgetItem* item)
            {   // Update Contents
                auto op = csgView.GetData().nodes[idx].op;

                std::string label = op == CSG_OP::CSG_ADD ? "Add" :
                                    op == CSG_OP::CSG_SUB ? "Sub" : "";

                item->setData(Qt::DisplayRole, label.c_str());
            },             
            [&](QListWidget* listWidget)
            {   // On Event
                listWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
                std::cout << "Changed\n";
            }              
        );

        panelCtx.PushHorizontalLayout();

        auto* label1 = panelCtx.AddText("Hello");
        auto* label2 = panelCtx.AddText("Hello");
        auto* label3 = panelCtx.AddText("Hello");

        panelCtx.Pop();
        panelCtx.Pop();
    }
};


/************************************************************************************************/


struct CSGComponentFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        viewportObject.gameObject.AddView<CSGView>();
    }

    inline static const std::string name = "CSG";

    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentInspector<CSGInspector>();
        EditorInspectorView::AddComponentFactory(std::make_unique<CSGComponentFactory>());
        return true;
    }

    inline static bool _registered = Register();
};


/************************************************************************************************/


std::vector<float3> CSGShape::GetTris() const
{
    return {};
}

FlexKit::BoundingSphere CSGShape::GetBoundingVolume() const
{
    return { 0, 0, 0, 1 };
}


/************************************************************************************************/
