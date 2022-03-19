#include "PCH.h"
#include "EditorColliderComponent.h"
#include "EditorInspectorView.h"
#include "EditorViewport.h"
#include "EditorProject.h"
#include "CSGComponent.h"
#include "physicsutilities.h"
#include "Serialization.hpp"
#include "..\FlexKitResourceCompiler\ResourceIDs.h"


/************************************************************************************************/


class StaticColliderResource :
    public FlexKit::Serializable<StaticColliderResource, FlexKit::iResource, GetTypeGUID(MeshResource)>
{
public:

    const ResourceID_t GetResourceTypeID()  const noexcept override { return TriMeshColliderTypeID; }
    const std::string& GetResourceID()      const noexcept override { return stringID; }
    const uint64_t     GetResourceGUID()    const noexcept override { return resourceID; }

    FlexKit::ResourceBlob CreateBlob()      const override
    {
        return {};
    }


    void Serialize(auto& archive)
    {
        archive& stringID;
        archive& resourceID;
    }

    FlexKit::Blob   colliderBlob;
    std::string     stringID;
    uint64_t        resourceID;
};


class ColliderInspector : public IComponentInspector
{
public:
    ColliderInspector(EditorViewport& IN_viewport, EditorProject& IN_project) :
        project     { IN_project },
        viewport    { IN_viewport } {}

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::StaticBodyComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& view) override
    {
        panelCtx.PushVerticalLayout("Static Collider", true);

        panelCtx.AddButton(
            "Create Collider from CSG Brush",
            [&]()
            {
                auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
                auto& gameObject = staticBody.GetGameObject();

                FlexKit::Apply(gameObject,
                    [&](CSGView& csg)
                    {
                        auto& physx     = FlexKit::PhysXComponent::GetComponent();
                        auto geometry   = csg->brushes.front().shape.CreateIndexedMesh();

                        auto newShape = physx.CookMesh2(
                            geometry.points.data(), geometry.points.size(),
                            geometry.indices.data(), geometry.indices.size());

                        auto resource = std::make_shared<StaticColliderResource>();
                        resource->stringID      = "Static Collider";
                        resource->resourceID    = rand();
                        resource->colliderBlob  = newShape;

                        auto shape = physx.LoadTriMeshShape(newShape);

                        if(shape != FlexKit::InvalidHandle_t)
                            staticBody.SetShape(shape);

                        project.AddResource(resource);
                    });
            });

        panelCtx.AddButton(
            "Select Collider",
            [&]()
            {
            });

        panelCtx.AddButton(
            "Add Cylinder Collider",
            [&]()
            {
            });

        panelCtx.AddButton(
            "Add Box Collider",
            [&]()
            {
            });

        panelCtx.Pop();
    }

    EditorProject&  project;
    EditorViewport& viewport;
};


/************************************************************************************************/


class RigidBodyInspector : public IComponentInspector
{
public:
    RigidBodyInspector(EditorViewport& IN_viewport, EditorProject& IN_project) :
        viewport    { IN_viewport },
        project     { IN_project } {}

    FlexKit::ComponentID ComponentID() override
    {
        return FlexKit::RigidBodyComponentID;
    }

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::ComponentViewBase& view) override
    {
        panelCtx.PushVerticalLayout("Rigid Body Collider", true);
        panelCtx.Pop();
    }

    EditorProject&  project;
    EditorViewport& viewport;
};


/************************************************************************************************/


struct ColliderComponentFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        auto& physx = FlexKit::PhysXComponent::GetComponent();
        auto layer  = scene.GetLayer();

        const static auto defaultCube = physx.CreateCubeShape(float3{ 1, 1, 1 });

        viewportObject.gameObject.AddView<FlexKit::StaticBodyView>(layer, defaultCube);
    }

    inline static const std::string name = "Static Collider";

    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<ColliderComponentFactory>());

        return true;
    }

    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& colliderComponent = static_cast<EditorColliderComponent&>(component);

    }

    inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<ColliderComponentFactory, FlexKit::StaticBodyComponentID> _register;

    inline static bool _registered = Register();
};


/************************************************************************************************/


struct RigidBodyComponentFactory : public IComponentFactory
{
    void Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        auto& physx = FlexKit::PhysXComponent::GetComponent();
        auto layer  = scene.GetLayer();

        const static auto defaultCube = physx.CreateCubeShape(float3{ 1, 1, 1 });

        viewportObject.gameObject.AddView<FlexKit::RigidBodyView>(layer, defaultCube);
    }

    inline static const std::string name = "Rigid Body Collider";

    const std::string& ComponentName() const noexcept { return name; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<RigidBodyComponentFactory>());

        return true;
    }

    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& editorCSGComponent = static_cast<EditorComponentCSG&>(component);
    }

    inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<ColliderComponentFactory, FlexKit::StaticBodyComponentID> _register;

    inline static bool _registered = Register();
};


/************************************************************************************************/


void RegisterColliderInspector(EditorViewport& viewport, EditorProject& project)
{
    EditorInspectorView::AddComponentInspector<ColliderInspector>(viewport, project);
}


/************************************************************************************************/


void RegisterRigidBodyInspector(EditorViewport& viewport, EditorProject& project)
{
    EditorInspectorView::AddComponentInspector<RigidBodyInspector>(viewport, project);
}


/************************************************************************************************/


FlexKit::Blob EditorColliderComponent::GetBlob()
{
    FlexKit::SaveArchiveContext archive;
    archive& ColliderResourceID;

    return archive.GetBlob();
}

FlexKit::Blob EditorRigidBodyComponent::GetBlob()
{
    FlexKit::SaveArchiveContext archive;
    archive& ColliderResourceID;

    return archive.GetBlob();
}


/**********************************************************************

Copyright (c) 2022 Robert May

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
