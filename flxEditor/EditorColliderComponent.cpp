#include "PCH.h"
#include "EditorColliderComponent.h"
#include "EditorInspectorView.h"
#include "EditorViewport.h"
#include "EditorProject.h"
#include "CSGComponent.h"
#include "physicsutilities.h"
#include "Serialization.hpp"
#include "ResourceIDs.h"
#include "EditorResourcePickerDialog.h"


/************************************************************************************************/

class StaticColliderResource :
    public FlexKit::Serializable<StaticColliderResource, FlexKit::iResource, GetTypeGUID(MeshResource)>
{
public:
    const ResourceID_t GetResourceTypeID()  const noexcept override { return TriMeshColliderTypeID; }
    const std::string& GetResourceID()      const noexcept override { return stringID; }
    const uint64_t     GetResourceGUID()    const noexcept override { return resourceID; }


    FlexKit::ResourceBlob CreateBlob() const
    {
        FlexKit::ColliderResourceBlob header;
        FlexKit::Blob blob;

        header.GUID         = resourceID;
        header.ResourceSize = sizeof(header) + colliderBlob.size();
        header.Type         = EResourceType::EResource_Collider;

        blob += header;
        blob += colliderBlob;

        auto [data, size] = blob.Release();

        FlexKit::ResourceBlob out;
        out.buffer          = (char*)data;
        out.bufferSize      = size;
        out.GUID            = resourceID;
        out.ID              = stringID;
        out.resourceType    = EResourceType::EResource_Collider;
        return out;
    }

    void Serialize(auto& archive)
    {
        archive& stringID;
        archive& resourceID;
        archive& colliderBlob;
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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& view) override
    {
        panelCtx.PushVerticalLayout("Static Collider", true);

        panelCtx.AddButton(
            "Create Collider from CSG Brush",
            [&]()
            {
                auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
                auto* editorData = (StaticColliderEditorData*)staticBody.GetUserData();
                auto& gameObject = staticBody.GetGameObject();

                FlexKit::Apply(gameObject,
                    [&](CSGView& csg)
                    {
                        if (!csg->brushes.size())
                            return;

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

                        Collider colliderMeta;
                        colliderMeta.shape.type             = FlexKit::StaticBodyType::TriangleMesh;
                        colliderMeta.shape.triMeshResource  = resource->resourceID;
                        colliderMeta.shape.orientation      = FlexKit::Quaternion{ 0, 0, 0, 1 };
                        colliderMeta.shape.position         = FlexKit::float3(0);
                        colliderMeta.shapeName              = "CSG generated Collider";

                        editorData->colliders.push_back({ colliderMeta });

                        if(shape)
                            staticBody.AddShape(shape);

                        auto projectResource = project.AddResource(resource);
                        viewport.GetScene()->sceneResource->sceneResources.push_back(projectResource);
                    });
            });

        panelCtx.AddButton(
            "Add TriMesh Collider",
            [&]()
            {
                auto resourcePicker = new EditorResourcePickerDialog(TriMeshColliderTypeID, project);

                resourcePicker->OnSelection(
                    [&](ProjectResource_ptr resource)
                    {
                        auto colliderResource = std::static_pointer_cast<StaticColliderResource>(resource->resource);

                        auto& physx         = FlexKit::PhysXComponent::GetComponent();
                        auto shape          = physx.LoadTriMeshShape(colliderResource->colliderBlob);
                        auto& staticBody    = static_cast<FlexKit::StaticBodyView&>(view);

                        staticBody.AddShape(shape);
                    });

                resourcePicker->show();
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
                auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
                auto* editorData = (StaticColliderEditorData*)staticBody.GetUserData();
                auto& gameObject = staticBody.GetGameObject();
                auto& physx      = FlexKit::PhysXComponent::GetComponent();
                auto cubeShape   = physx.CreateCubeShape(FlexKit::float3{ 1, 1, 1 });

                staticBody.AddShape(cubeShape);

                Collider collider;
                collider.shape.type = FlexKit::StaticBodyType::Cube;
                collider.shape.cube.dimensions[0] = 1.0f;
                collider.shape.cube.dimensions[1] = 1.0f;
                collider.shape.cube.dimensions[2] = 1.0f;

                collider.shapeName = "CubeShape 1x1x1";
                editorData->colliders.push_back(collider);
            });



        auto& list = *panelCtx.AddList(
            [&]() -> size_t 
            {   // Size update
                auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
                auto* editorData = (StaticColliderEditorData*)staticBody.GetUserData();

                if (editorData)
                    return editorData->colliders.size();
                else return 0;
            },
            [&](auto idx, QListWidgetItem* item)
            {   // Update Contents
                auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
                auto* editorData = (StaticColliderEditorData*)staticBody.GetUserData();

                auto resourceID = editorData->colliders[idx].shape.triMeshResource;
                std::string label{ std::format("{}", resourceID)};

                item->setData(Qt::DisplayRole, label.c_str());
            },
            [&](QListWidget* listWidget)
            {   // On Event
                auto item   = listWidget->currentItem();
                auto idx    = listWidget->indexFromItem(item);

                listWidget->setCurrentIndex(idx);
            }
        );

        panelCtx.AddButton(
            "Remove Collider",
            [&]()
            {
                auto item           = list.currentItem();
                auto idx            = list.indexFromItem(item).row();
                auto& staticBody    = static_cast<FlexKit::StaticBodyView&>(view);

                staticBody.RemoveShape(idx);
            });

        panelCtx.AddButton(
            "Update Collider From CSG",
            [&]()
            {
                auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
                auto& gameObject = staticBody.GetGameObject();

                FlexKit::Apply(gameObject,
                    [&](CSGView& csg)
                    {
                        if (!csg->brushes.size())
                            return;

                        auto* editorData    = (StaticColliderEditorData*)staticBody.GetUserData();
                        auto item           = list.currentItem();
                        auto idx            = list.indexFromItem(item).row();

                        auto& physx         = FlexKit::PhysXComponent::GetComponent();
                        auto geometry       = csg->brushes.front().shape.CreateIndexedMesh();

                        auto newShape = physx.CookMesh2(
                            geometry.points.data(), geometry.points.size(),
                            geometry.indices.data(), geometry.indices.size());

                        auto shape = physx.LoadTriMeshShape(newShape);
                        if (shape)
                        {
                            auto& staticBody    = static_cast<FlexKit::StaticBodyView&>(view);
                            auto resource       = std::static_pointer_cast<StaticColliderResource>(project.FindProjectResource(editorData->colliders[idx].shape.triMeshResource)->resource);

                            resource->colliderBlob = newShape;

                            staticBody.RemoveShape(idx);
                            staticBody.AddShape(shape);
                        }
                    });
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

    void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& view) override
    {
        panelCtx.PushVerticalLayout("Rigid Body Collider", true);

        panelCtx.AddButton(
            "Add Cube Rigid Body",
            [&]()
            {
                auto& physx     = FlexKit::PhysXComponent::GetComponent();
                auto cubeShape  = physx.CreateCubeShape({ 1, 1, 1 });
            });

        panelCtx.Pop();
    }

    EditorProject&  project;
    EditorViewport& viewport;
};


/************************************************************************************************/


struct ColliderComponentFactory : public IComponentFactory
{
    FlexKit::ComponentViewBase& Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        auto& physx = FlexKit::PhysXComponent::GetComponent();
        auto layer  = scene.GetLayer();

        const static auto defaultCube   = physx.CreateCubeShape(float3{ 1, 1, 1 });
        auto editorData                 = new StaticColliderEditorData{};

        auto& staticBody = viewportObject.gameObject.AddView<FlexKit::StaticBodyView>(layer);

        staticBody.SetUserData(editorData);

        return staticBody;
    }

    inline static const std::string name = "Static Collider";

    const std::string&      ComponentName() const noexcept { return name; }
    FlexKit::ComponentID    ComponentID() const noexcept { return FlexKit::StaticBodyComponentID; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<ColliderComponentFactory>());

        return true;
    }

    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& colliderComponent = static_cast<EditorColliderComponent&>(component);
        auto& colliderView      = static_cast<FlexKit::StaticBodyView&>(base);
        auto* editorData        = static_cast<StaticColliderEditorData*>(colliderView.GetUserData());

        colliderComponent.colliders = editorData->colliders;
    }

    inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<ColliderComponentFactory, FlexKit::StaticBodyComponentID> _register;

    inline static bool _registered = Register();
};


/************************************************************************************************/


struct RigidBodyComponentFactory : public IComponentFactory
{
    FlexKit::ComponentViewBase& Construct(ViewportGameObject& viewportObject, ViewportScene& scene)
    {
        auto& physx = FlexKit::PhysXComponent::GetComponent();
        auto layer  = scene.GetLayer();

        const static auto defaultCube = physx.CreateCubeShape(float3{ 1, 1, 1 });

        auto& rigidBodyView = viewportObject.gameObject.AddView<FlexKit::RigidBodyView>(layer);
        rigidBodyView.AddShape(defaultCube);

        return rigidBodyView;
    }

    inline static const std::string name = "Rigid Body Collider";

    const std::string&      ComponentName() const noexcept { return name; }
    FlexKit::ComponentID    ComponentID() const noexcept { return FlexKit::RigidBodyComponentID; }

    static bool Register()
    {
        EditorInspectorView::AddComponentFactory(std::make_unique<RigidBodyComponentFactory>());

        return true;
    }

    static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
    {
        auto& colliderComponent = static_cast<EditorRigidBodyComponent&>(component);
    }

    inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<ColliderComponentFactory, FlexKit::RigidBodyComponentID> _register;

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
    FlexKit::StaticBodyHeaderBlob header{
        .shapeCount = (uint32_t)colliders.size()
    };

    FlexKit::Blob blob;

    blob += header;

    for (auto& collider : colliders)
        blob += collider.shape;

    return blob;
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