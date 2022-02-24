#include "PCH.h"
#include <SceneInspectors.h>


/************************************************************************************************/


void TransformInspector::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component)
{
    auto& sceneNodeView = static_cast<FlexKit::SceneNodeView<>&>(component);

    const auto initialPos       = sceneNodeView.GetPosition();
    const auto initialPosLcl    = sceneNodeView.GetPositionL();
    const auto scale            = sceneNodeView.GetScale();
    const auto orientation      = sceneNodeView.GetOrientation();

    panelCtx.PushHorizontalLayout("Transform", true);

    panelCtx.AddText(fmt::format("Node: {}", sceneNodeView.node.to_uint()));
    panelCtx.AddText(fmt::format("Parent: {}", sceneNodeView.GetParentNode().to_uint()));

    panelCtx.Pop();

    auto positionTxt = panelCtx.AddText(fmt::format("Global: [{}, {}, {}]", initialPos.x, initialPos.y, initialPos.z));

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
            const float x = strtof(txt.c_str(), &p);
            if (!*p)
            {
                auto position = sceneNodeView.GetPosition();

                if (position.x != x)
                {
                    position.x = x;
                    sceneNodeView.SetPosition(position);
                    positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
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
            const float y = strtof(txt.c_str(), &p);
            if (!*p)
            {
                auto position = sceneNodeView.GetPosition();

                if (position.y != y)
                {
                    position.y = y;
                    sceneNodeView.SetPosition(position);
                    positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
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
            const float z = strtof(txt.c_str(), &p);
            if (!*p)
            {
                auto position = sceneNodeView.GetPosition();

                if (position.z != z)
                {
                    position.z = z;
                    sceneNodeView.SetPosition(position);
                    positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            }
        });

    panelCtx.Pop();

    auto lclPosTxt = panelCtx.AddText(fmt::format("Local: [{}, {}, {}]", initialPosLcl.x, initialPosLcl.y, initialPosLcl.z));

    panelCtx.PushHorizontalLayout("", true);

    panelCtx.AddInputBox(
        "X",
        [&](std::string& txt)
        {
            auto pos = sceneNodeView.GetPositionL();
            txt = fmt::format("{}", pos.x);
        },
        [&, positionTxt = positionTxt](const std::string& txt)
        {
            char* p;
            const float x = strtof(txt.c_str(), &p);
            if (!*p)
            {
                auto position = sceneNodeView.GetPositionL();

                if (position.x != x)
                {
                    position.x = x;
                    sceneNodeView.SetPositionL(position);
                    positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            }
        });

    panelCtx.AddInputBox(
        "Y",
        [&](std::string& txt)
        {
            auto pos = sceneNodeView.GetPositionL();
            txt = fmt::format("{}", pos.y);
        },
        [&, positionTxt = positionTxt](const std::string& txt)
        {
            char* p;
            const float y = strtof(txt.c_str(), &p);
            if (!*p)
            {
                auto position = sceneNodeView.GetPositionL();

                if (position.y != y)
                {
                    position.y = y;
                    sceneNodeView.SetPositionL(position);
                    positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            }
        });

    panelCtx.AddInputBox(
        "Z",
        [&](std::string& txt)
        {
            auto pos = sceneNodeView.GetPositionL();
            txt = fmt::format("{}", pos.z);
        },
        [&, positionTxt = positionTxt](const std::string& txt)
        {
            char* p;
            const float z = strtof(txt.c_str(), &p);
            if (!*p)
            {
                auto position = sceneNodeView.GetPositionL();

                if (position.z != z)
                {
                    position.z = z;
                    sceneNodeView.SetPositionL(position);
                    positionTxt->setText(fmt::format("Position: [{}, {}, {}]", position.x, position.y, position.z).c_str());
                }
            }
        });

    panelCtx.Pop();

    auto scaleLabel = panelCtx.AddText(fmt::format("Scale: [{}, {}, {}]", scale.x, scale.y, scale.z));
    auto timer = new QTimer{ scaleLabel };

    timer->start(250);
    timer->connect(timer, &QTimer::timeout,
        [=, &sceneNodeView]
        {
            const auto scale = sceneNodeView.GetScale();
            scaleLabel->setText(fmt::format("Scale: [{}, {}, {}]", scale.x, scale.y, scale.z).c_str());
        });

    panelCtx.PushHorizontalLayout("Orientation", true);
    auto orientationLabel = panelCtx.AddText(fmt::format("Quaternion: [{}, {}, {}, {}]", orientation.x, orientation.y, orientation.z, orientation.w));

    timer->start(250);
    timer->connect(timer, &QTimer::timeout,
        [=, &sceneNodeView]
        {
            const auto orientation = sceneNodeView.GetOrientation();
            orientationLabel->setText(fmt::format("Quaternion: [{}, {}, {}, {}]", orientation.x, orientation.y, orientation.z, orientation.w).c_str());
        });
}


/************************************************************************************************/


void VisibilityInspector::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component)
{
    auto& visibility = static_cast<FlexKit::SceneVisibilityView&>(component);

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


/************************************************************************************************/


void PointLightInspector::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component)
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


/************************************************************************************************/


void SceneBrushInspector::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component)
{
    auto& brush = static_cast<FlexKit::BrushView&>(component);

    panelCtx.AddHeader("Brush");

    auto mesh_ptr = FlexKit::GetMeshResource(brush.GetTriMesh());

    if (mesh_ptr->ID)
        panelCtx.AddText(fmt::format("Tri Mesh Resource: {}", mesh_ptr->ID));

    panelCtx.AddText(fmt::format("Tri Mesh AssetHandle: {}", mesh_ptr->assetHandle));

    if (mesh_ptr->AnimationData)
        panelCtx.AddText("Brush Animated");
}


/************************************************************************************************/


void MaterialInspector::Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& component)
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


/************************************************************************************************/
