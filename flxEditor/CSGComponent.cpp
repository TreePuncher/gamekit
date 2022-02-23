#include "PCH.h"
#include "CSGComponent.h"
#include "ViewportScene.h"
#include "EditorInspectorView.h"
#include "qlistwidget.h"


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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        panelCtx.PushVerticalLayout("CSG", true);

        panelCtx.AddList(
            [&]() -> size_t
            {    // Update Size
                return 1;
            }, 
            [&](auto idx, QListWidgetItem* item)
            {   // Update Contents
                item->setData(Qt::DisplayRole, "Testing");
            },             
            [&](auto& evt)
            {   // On Event
            }              
        );

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
