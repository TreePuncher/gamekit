#pragma once

#include "EditorInspectorView.h"
#include <Transforms.h>
#include <fmt/format.h>
#include <QtWidgets\qlabel.h>
#include <format>


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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        auto& sceneNodeView = static_cast<FlexKit::SceneNodeView<>&>(component);

        const auto initialPos   = sceneNodeView.GetPosition();
        const auto scale        = sceneNodeView.GetScale();
        const auto orientation  = sceneNodeView.GetOrientation();

        panelCtx.PushHorizontalLayout("Transform", true);

        panelCtx.AddText(fmt::format("Node: {}", sceneNodeView.node.to_uint()));
        panelCtx.AddText(fmt::format("Parent: {}", sceneNodeView.GetParentNode().to_uint()));

        panelCtx.Pop();

        auto positionTxt = panelCtx.AddText(fmt::format("Position: \t[{}, {}, {}]", initialPos.x, initialPos.y, initialPos.z));

        panelCtx.PushHorizontalLayout("", true);

        panelCtx.AddInputBox(
            "X",
            [&](std::string& txt)
            {
                auto pos = sceneNodeView.GetPosition();
                txt = fmt::format("{}", pos.x);
            },
            [&, positionTxt = positionTxt](const std::string& txt)
            {
                char* p;
                float x = strtof(txt.c_str(), &p);
                if (!*p)
                {
                    auto position = sceneNodeView.GetPosition();
                    position.x = x;

                    sceneNodeView.SetPosition(position);
                    positionTxt->setText(fmt::format("Position: \t[{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            });

        panelCtx.AddInputBox(
            "Y",
            [&](std::string& txt)
            {
                auto pos = sceneNodeView.GetPosition();
                txt = fmt::format("{}", pos.y);
            },
            [&, positionTxt = positionTxt](const std::string& txt)
            {
                char* p;
                float y = strtof(txt.c_str(), &p);
                if (!*p)
                {
                    auto position = sceneNodeView.GetPosition();
                    position.y = y;

                    sceneNodeView.SetPosition(position);
                    positionTxt->setText(fmt::format("Position: \t[{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            });

        panelCtx.AddInputBox(
            "Z",
            [&](std::string& txt)
            {
                auto pos = sceneNodeView.GetPosition();
                txt = fmt::format("{}", pos.z);
            },
            [&, positionTxt = positionTxt](const std::string& txt)
            {
                char* p;
                float z = strtof(txt.c_str(), &p);
                if (!*p)
                {
                    auto position = sceneNodeView.GetPosition();
                    position.z = z;

                    sceneNodeView.SetPosition(position);
                    positionTxt->setText(fmt::format("Position: \t[{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            });

        panelCtx.Pop();

        panelCtx.AddText(fmt::format("Scale: \t[{}, {}, {}]", scale.x, scale.y, scale.z));
        panelCtx.AddText(fmt::format("Orientation: \t[{}, {}, {}]", orientation.x, orientation.y, orientation.z));
    }

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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        auto& visibility    = static_cast<FlexKit::SceneVisibilityView&>(component);

        panelCtx.AddHeader("Visibility");
        panelCtx.AddText("Bounding Volume type: Bounding Sphere");

        panelCtx.AddInputBox(
            "Bounding Sphere Radius",
            [&](std::string& str)
            {
                str = fmt::format("{}", visibility.GetBoundingSphere().w);
            },
            [&](const std::string& txt)
            {
                char* p;
                float r = strtof(txt.c_str(), &p);
                if (!*p)
                {
                    auto BS = visibility.GetBoundingSphere();
                    BS.w = r;
                    visibility.SetBoundingSphere(BS);
                }
            });
    }

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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        auto& pointLight = static_cast<FlexKit::PointLightView&>(component);

        panelCtx.AddHeader("Point Light");

        panelCtx.AddText(fmt::format("Node: {}", pointLight.GetNode().to_uint()));

        panelCtx.AddInputBox(
            "Radius",
            [&](std::string& txt)
            {
                txt = fmt::format("{}", pointLight.GetRadius());
            },
            [&](const std::string& txt)
            {
                char* p;
                float r = strtof(txt.c_str(), &p);
                if (!*p)
                {
                    auto radius = pointLight.GetRadius();
                    pointLight.SetRadius(r);
                }
            });

        panelCtx.AddInputBox(
            "Intensity",
            [&](std::string& string) {
                string = fmt::format("{}", pointLight.GetRadius());
            },
            [&](const std::string& txt)
            {
                char* p;
                float i = strtof(txt.c_str(), &p);
                if (!*p)
                {
                    auto radius = pointLight.GetIntensity();
                    pointLight.SetIntensity(i);
                }
            });
    }
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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        auto& brush = static_cast<FlexKit::BrushView&>(component);

        panelCtx.AddHeader("Brush");

        auto mesh_ptr = FlexKit::GetMeshResource(brush.GetTriMesh());

        if(mesh_ptr->ID)
            panelCtx.AddText(fmt::format("Tri Mesh Resource: {}", mesh_ptr->ID));

        panelCtx.AddText(fmt::format("Tri Mesh AssetHandle: {}", mesh_ptr->assetHandle));

        if (mesh_ptr->AnimationData)
            panelCtx.AddText("Brush Animated");
    }
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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component) override
    {
        auto& material = static_cast<FlexKit::MaterialComponentView&>(component);

        panelCtx.AddHeader("Material");

        auto passes = material.GetPasses();
        panelCtx.AddText("Passes");

        if (passes.size())
        {
            panelCtx.PushVerticalLayout();
            for (auto& pass : passes)
                panelCtx.AddText(std::string{} + std::format("{}", pass.to_uint()));

            panelCtx.Pop();
        }

        panelCtx.AddText("Pass Count" + std::format("{}", passes.size()));
        panelCtx.AddButton("Add Pass", [&]() {});
    }
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

