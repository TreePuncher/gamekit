#pragma once

#include "EditorInspectorView.h"
#include <Transforms.h>
#include <fmt/format.h>
#include <QtWidgets\qlabel.h>
#include <format>
#include <CSGComponent.h>
#include <ViewportScene.h>

/************************************************************************************************/


class TransformInspector : public IComponentInspector
{
public:

    TransformInspector() {}
    ~TransformInspector() {}

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::TransformComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override;
private:

};


class TransformComponentFactory : public IComponentFactory
{
public:
    ~TransformComponentFactory() {}

    void Construct(ViewportGameObject& gameObject, ViewportScene& scene)
    {
        gameObject.gameObject.AddView<FlexKit::SceneNodeView<>>(FlexKit::GetZeroedNode());
    }

    const std::string& ComponentName() const noexcept { return name; }

    inline static const std::string name = "Transform";

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<TransformComponentFactory>());
        return true;
    }

    inline static bool _registered = Register();
};


/************************************************************************************************/


class VisibilityInspector : public IComponentInspector
{
public:

    VisibilityInspector() {}
    ~VisibilityInspector() {}

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::SceneVisibilityComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override;

private:

};


/************************************************************************************************/


class PointLightInspector : public IComponentInspector
{
public:

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::PointLightComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override;
};


struct PointLightFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& gameObject, ViewportScene& scene)
    {
        gameObject.gameObject.AddView<FlexKit::PointLightView>();
    }

    inline static const std::string name = "PointLight";
    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<PointLightFactory>());
        return true;
    }

    inline static bool _registered = Register();
};


/************************************************************************************************/


class PointLightShadowInspector : public IComponentInspector
{
public:

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::PointLightShadowMapID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        auto& cubeShadowMapView = static_cast<FlexKit::PointLightShadowMapView&>(component);

        panelCtx.AddHeader("Point Light Shadow Map");
        panelCtx.AddText("No Fields!");
    }
};


struct CubicShadowMapFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        if(viewportObject.gameObject.GetView(FlexKit::PointLightComponent::GetComponentID()))
            viewportObject.gameObject.AddView<FlexKit::PointLightShadowMapView>(
                FlexKit::_PointLightShadowCaster{ FlexKit::GetPointLight(viewportObject.gameObject),
                FlexKit::GetSceneNode(viewportObject.gameObject) } );
    }

    inline static const std::string name = "Cubic Shadow Map";
    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<CubicShadowMapFactory>());
        return true;
    }

    inline static bool _registered = Register();
};


/************************************************************************************************/


class SceneBrushInspector : public IComponentInspector
{
public:

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::BrushComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override;
};


struct SceneBrushFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        viewportObject.gameObject.AddView<FlexKit::BrushView>(FlexKit::InvalidHandle_t, FlexKit::GetSceneNode(viewportObject.gameObject));
    }

    inline static const std::string name = "Brush";
    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<SceneBrushFactory>());
        return true;
    }

    inline static bool _registered = Register();
};


/************************************************************************************************/


class MaterialInspector : public IComponentInspector
{
public:

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::BrushComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override;
};


struct MaterialFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        viewportObject.gameObject.AddView<FlexKit::MaterialComponentView>(FlexKit::MaterialComponent::GetComponent().CreateMaterial());
        FlexKit::SetMaterialHandle(viewportObject.gameObject, FlexKit::GetMaterialHandle(viewportObject.gameObject));
    }

    inline static const std::string name = "Material";
    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<MaterialFactory>());
        return true;
    }

    inline static bool _registered = Register();
};


/************************************************************************************************/

