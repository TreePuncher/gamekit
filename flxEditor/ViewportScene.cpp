#include "PCH.h"
#include "ViewportScene.h"
#include "AnimationComponents.h"
#include "CSGComponent.h"

/************************************************************************************************/


struct EntityStringIDComponent
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& stringID  = static_cast<FlexKit::EntityStringIDComponent&>(component);
        auto& runtime   = static_cast<FlexKit::StringIDView&>(base);

        stringID.stringID = runtime.GetString();
    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<EntityStringIDComponent, FlexKit::StringComponentID> _register;
};


struct TransformComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& sceneNode = static_cast<FlexKit::EntitySceneNodeComponent&>(component);
        auto& runtime   = static_cast<FlexKit::SceneNodeView<>&>(base);

        sceneNode.nodeIdx = scene.MapNode(runtime.node);

    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<TransformComponentUpdate, FlexKit::TransformComponentID> _register;
};


struct BrushComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& brush         = static_cast<FlexKit::EntityBrushComponent&>(component);
        auto& runtime       = static_cast<FlexKit::BrushView&>(base);
        auto& runtimeBrush  = runtime.GetBrush();

        //brush.material = runtimeBrush.material; // TODO: update material component data
        brush.MeshGuid  = FlexKit::GetMeshResource(runtimeBrush.MeshHandle)->assetHandle;
    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<BrushComponentUpdate, FlexKit::BrushComponentID> _register;
};


struct MaterialComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& material  = static_cast<FlexKit::EntityMaterialComponent&>(component);
        auto& runtime   = static_cast<FlexKit::MaterialComponentView&>(base);
    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<MaterialComponentUpdate, FlexKit::MaterialComponentID> _register;
};


struct SkeletonComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& skeleton  = static_cast<FlexKit::EntitySkeletonComponent&>(component);
        auto& runtime   = static_cast<FlexKit::SkeletonView&>(base);

        skeleton.skeletonResourceID = runtime.GetSkeleton()->guid;
    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<SkeletonComponentUpdate, FlexKit::SkeletonComponentID> _register;
};


struct PointLightComponentUpdate
{
    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& pointlight    = static_cast<FlexKit::EntityPointLightComponent&>(component);
        auto& runtime       = static_cast<FlexKit::PointLightView&>(base);

        pointlight.I = runtime.GetIntensity();
        pointlight.K = runtime.GetK();
        pointlight.R = runtime.GetRadius();
    }

    IEntityComponentRuntimeUpdater::RegisterConstructorHelper<PointLightComponentUpdate, FlexKit::PointLightComponentID> _register;
};


/************************************************************************************************/


ViewportObjectList ViewportScene::RayCast(FlexKit::Ray v) const
{
    auto&       visables        = FlexKit::SceneVisibilityComponent::GetComponent();
    const auto  rayCastResults  = scene.RayCast(v, FlexKit::SystemAllocator);

    ViewportObjectList results;
    for (auto& result : rayCastResults)
    {
        auto res = std::find_if(
            sceneObjects.begin(), sceneObjects.end(),
            [&](auto& object)
            {
                return &object->gameObject == visables[result.visibileObject].entity;
            });

        if (res != sceneObjects.end())
            results.emplace_back(*res);
    }

    return results;
}


/************************************************************************************************/


void ViewportScene::Update()
{
    auto& entities = sceneResource->sceneResource->entities; // TODO: make this not dumb
    ViewportSceneContext ctx{ *this };

    for (auto& object : sceneObjects)
    {
        auto res = std::find_if(entities.begin(), entities.end(),
            [&](FlexKit::SceneEntity& entity)
            {
                return (entity.objectID == object->objectID);
            });

        if (res != entities.end())
        {   // Update Data
            for (auto& component : object->gameObject)
            {
                const auto id = component->ID;

                auto res2 = std::find_if(
                    std::begin(res->components),
                    std::end(res->components),
                    [&](FlexKit::EntityComponent_ptr& component)
                    {
                        return component->id == id;
                    });

                if(res2 != res->components.end())
                    IEntityComponentRuntimeUpdater::Update((**res2), component.Get_ref(), ctx);
            }
        }
        else
        {   // Add GameObject
            auto& entity = entities.emplace_back();

            for (auto& component : object->gameObject)
            {
                if (auto entityComponent = FlexKit::EntityComponent::CreateComponent(component.ID); entityComponent)
                {
                    IEntityComponentRuntimeUpdater::Update(*entityComponent, component.Get_ref(), ctx);
                    entity.components.emplace_back(entityComponent);
                }
                //else// TODO: ignore certain components that are runtime only
                //    FK_LOG_WARNING("Failed to serialize component data! Component ID: %u", component.ID);

            }
        }

    }

    auto& nodes = sceneResource->sceneResource->nodes;
    nodes.clear();

    for (const auto& node : ctx.nodes)
    {
        FlexKit::SceneNode sceneNode;
        sceneNode.scale         = node.scale;
        sceneNode.orientation   = node.orientation;
        sceneNode.position      = node.position;
        sceneNode.parent        = node.parent;

        nodes.emplace_back(sceneNode);
    }
}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
