#include "PCH.h"
#include "EditorApplication.h"
#include "EditorColliderComponent.h"
#include "EditorInspectorView.h"
#include "EditorViewport.h"
#include "EditorProject.h"
#include "CSGComponent.h"
#include "physicsutilities.h"
#include "Serialization.hpp"
#include "ResourceIDs.h"
#include "EditorResourcePickerDialog.h"
#include "Scene.h"
#include "KeyValueIds.h"
#include "EditorPrefabObject.h"


/************************************************************************************************/


class StaticColliderResource :
	public FlexKit::Serializable<StaticColliderResource, FlexKit::iResource, TriMeshColliderTypeID>
{
public:
			ResourceID_t	GetResourceTypeID()	const noexcept override { return TriMeshColliderTypeID; }
	const	std::string&	GetResourceID()		const noexcept override { return stringID; }
			uint64_t		GetResourceGUID()		const noexcept override { return guid; }

	void SetResourceID(const std::string& newID)	noexcept final { stringID	= newID; }
	void SetResourceGUID(uint64_t newGUID)			noexcept final { guid		= newGUID; }

	FlexKit::ResourceBlob CreateBlob() const
	{
		FlexKit::ColliderResourceBlob header;
		FlexKit::Blob blob;

		header.GUID			= guid;
		header.ResourceSize	= sizeof(header) + colliderBlob.size();
		header.Type			= EResourceType::EResource_Collider;
		
		blob += header;
		blob += colliderBlob;

		auto [data, size] = blob.Release();

		FlexKit::ResourceBlob out;
		out.buffer			= (char*)data;
		out.bufferSize		= size;
		out.GUID			= guid;
		out.ID				= stringID;
		out.resourceType	= EResourceType::EResource_Collider;

		return out;
	}

	void Serialize(auto& archive)
	{
		archive& stringID;
		archive& guid;
		archive& colliderBlob;
	}

	FlexKit::Blob	colliderBlob;
	std::string		stringID;
	uint64_t		guid;
};


/************************************************************************************************/


class ColliderEditorComponent final : public IEditorComponent
{
public:
	ColliderEditorComponent(EditorViewport& IN_viewport, EditorProject& IN_project) :
		project		{ IN_project },
		viewport	{ IN_viewport } {}


	inline static const std::string name = "Static Collider";


	FlexKit::ComponentID ComponentID()	const noexcept { return FlexKit::StaticBodyComponentID; }
	const std::string& ComponentName()	const noexcept { return name; }


	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& view) override
	{
		panelCtx.PushVerticalLayout("Static Collider", true);

		panelCtx.PushHorizontalLayout("Create", true);

		panelCtx.AddButton(
			"Create Collider from TriMesh",
			[&]()
			{
				auto& staticBody = static_cast<FlexKit::StaticBodyView&>(view);
				auto* editorData = (StaticColliderEditorData*)staticBody.GetUserData();
				auto& gameObject = staticBody.GetGameObject();

				FlexKit::Apply(gameObject,
					[&](FlexKit::BrushView& brush)
					{
						auto meshes = brush.GetMeshes();

						if (!meshes.empty())
						{
							auto meshResource	= FlexKit::GetMeshResource(meshes.front());
							auto& physx			= FlexKit::PhysXComponent::GetComponent();

							auto lod			= meshResource->GetLowestLoadedLod();
							auto indexBufferIdx = lod.GetIndexBufferIndex();

							auto indexBuffer = lod.GetIndices();
							auto pointBuffer = lod.GetPoints();

							if (pointBuffer && indexBuffer)
							{
								auto newShape = physx.CookMesh2(
									(float3*)pointBuffer->GetBuffer(), pointBuffer->GetBufferSize(),
									(uint32_t*)indexBuffer->GetBuffer(), indexBuffer->GetBufferSize());

								auto resource = std::make_shared<StaticColliderResource>();
								resource->stringID		= "Static Collider";
								resource->guid			= rand();
								resource->colliderBlob	= newShape;

								auto shape = physx.LoadTriMeshShape(newShape);
								
								Collider colliderMeta;
								colliderMeta.shape.type				= FlexKit::StaticBodyType::TriangleMesh;
								colliderMeta.shape.triMeshResource	= resource->guid;
								colliderMeta.shape.orientation		= FlexKit::Quaternion{ 0, 0, 0, 1 };
								colliderMeta.shape.position			= FlexKit::float3(0);
								colliderMeta.shapeName				= std::string(meshResource->ID) + "_Collider";

								editorData->colliders.push_back({ colliderMeta });

								if(shape)
									staticBody.AddShape(shape);

								auto projectResource = project.AddResource(resource);

								if(viewport.GetScene())
									viewport.GetScene()->sceneResource->sceneResources.push_back(projectResource);
							}
							else
								return;

						}
					});
			});


		if (viewport.GetScene())
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
							resource->guid          = rand();
							resource->colliderBlob  = newShape;

							auto shape = physx.LoadTriMeshShape(newShape);

							Collider colliderMeta;
							colliderMeta.shape.type             = FlexKit::StaticBodyType::TriangleMesh;
							colliderMeta.shape.triMeshResource  = resource->guid;
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

		panelCtx.Pop();

		panelCtx.AddButton(
			"Add TriMesh Collider",
			[&]()
			{
				auto resourcePicker = new EditorResourcePickerDialog(TriMeshColliderTypeID, project);

				resourcePicker->OnSelection(
					[&](ProjectResource_ptr resource)
					{
						auto colliderResource = std::static_pointer_cast<StaticColliderResource>(resource->resource);

						auto& physx			= FlexKit::PhysXComponent::GetComponent();
						auto shape			= physx.LoadTriMeshShape(colliderResource->colliderBlob);
						auto& staticBody	= static_cast<FlexKit::StaticBodyView&>(view);

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
				auto& staticBody	= static_cast<FlexKit::StaticBodyView&>(view);
				auto* editorData	= (StaticColliderEditorData*)staticBody.GetUserData();
				auto& gameObject	= staticBody.GetGameObject();
				auto& physx			= FlexKit::PhysXComponent::GetComponent();
				auto cubeShape		= physx.CreateCubeShape(FlexKit::float3{ 1, 1, 1 });

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

				if(item->text() != label.c_str())
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
				auto* editorData    = (StaticColliderEditorData*)staticBody.GetUserData();

				if (idx < 0 || idx > editorData->colliders.size())
					return;

				staticBody.RemoveShape(idx);

				editorData->colliders.erase(editorData->colliders.begin() + idx);
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

		panelCtx.AddButton(
			"Transform to Node",
			[&]()
			{
				auto& staticBody	= static_cast<FlexKit::StaticBodyView&>(view);
				auto& gameObject	= staticBody.GetGameObject();

				const auto p = FlexKit::GetWorldPosition(gameObject);
				const auto q = FlexKit::GetOrientation(gameObject);

				FlexKit::StaticBodySetWorldPosition(gameObject,		p);
				FlexKit::StaticBodySetWorldOrientation(gameObject,	q);
			});


		panelCtx.Pop();
	}


	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		auto layerHandle = ctx.GetSceneLayer();
		FK_ASSERT(layerHandle != FlexKit::InvalidHandle);

		auto& physx = FlexKit::PhysXComponent::GetComponent();

		const static auto defaultCube	= physx.CreateCubeShape(float3{ 1, 1, 1 });
		auto editorData					= new StaticColliderEditorData{};
		editorData->editorId			= ctx.GetEditorIdentifier();
		auto& staticBody				= gameObject.AddView<FlexKit::StaticBodyView>(layerHandle);

		staticBody.SetUserData(editorData);

		return staticBody;
	}


	static void AddEditorColliderData(FlexKit::GameObject& gameObject, const char* userData_ptr, size_t userData_size, FlexKit::ValueMap map)
	{
		auto ctx = FlexKit::FindValue<LoadEntityContextInterface*>(map, FlexKit::LoadEntityContextInterfaceKID);

		if (userData_ptr && ctx)
		{   // Loading collider from disk
			auto entity	= FlexKit::FindValue<FlexKit::SceneEntity*>(map, FlexKit::SceneEntityKID).value_or(ctx.value()->Resource());

			FlexKit::StaticBodyHeaderBlob header;
			memcpy(&header, (void*)userData_ptr, sizeof(header));

			auto& _ptr		= ctx.value();
			auto shapes_ptr	= ((FlexKit::StaticBodyShape*)(userData_ptr + sizeof(header)));
			auto shapes		= std::span{ shapes_ptr, header.shapeCount };

			if (entity)
			{
				auto staticBodyComponent = std::static_pointer_cast<EditorColliderComponent>(entity->FindComponent(FlexKit::StaticBodyComponentID));

				if (staticBodyComponent)
				{
					FlexKit::Apply(gameObject,
						[&](FlexKit::StaticBodyView&	view)
						{
							auto editorData			= new StaticColliderEditorData{};
							editorData->colliders	= staticBodyComponent->colliders;

							view.SetUserData(editorData);
						});
				}
			}
			else
			{
				FK_LOG_INFO("Failed to create collider component");
			}
		}
	}


	static void Register(EditorViewport& viewport, EditorProject& project)
	{
		static bool once = []
		{
			FlexKit::StaticBodyComponent::SetOnConstructionHandler(
				[](FlexKit::GameObject& gameObject, const char* userData_ptr, size_t userData_size, FlexKit::ValueMap values)
				{
					AddEditorColliderData(gameObject, userData_ptr, userData_size, values);
				});

			return true;
		}();

		static ColliderEditorComponent component(viewport, project);
		EditorInspectorView::AddComponent(component);
	}


	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& colliderView		= static_cast<FlexKit::StaticBodyView&>(base);
		auto& colliderComponent	= static_cast<EditorColliderComponent&>(component);
		auto* editorData		= static_cast<StaticColliderEditorData*>(colliderView.GetUserData());

		FK_ASSERT(editorData != nullptr);

		if (editorData)
		{
			colliderComponent.colliders = editorData->colliders;
			colliderComponent.editorId  = editorData->editorId;
		}
	}


	inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<ColliderEditorComponent, FlexKit::StaticBodyComponentID> _register;

	EditorProject&	project;
	EditorViewport&	viewport;
};


/************************************************************************************************/


class RigidBodyEditorComponent final : public IEditorComponent
{
public:
	RigidBodyEditorComponent(EditorViewport& IN_viewport, EditorProject& IN_project) :
		viewport	{ IN_viewport },
		project		{ IN_project } {}


	FlexKit::ComponentID	ComponentID()	const noexcept { return FlexKit::RigidBodyComponentID; }
	const std::string&		ComponentName() const noexcept { return name; }


	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject&, FlexKit::ComponentViewBase& view) override
	{
		panelCtx.PushVerticalLayout("Rigid Body Collider", true);

		panelCtx.AddButton(
			"Add Cube Rigid Body",
			[&]()
			{
				auto& physx		= FlexKit::PhysXComponent::GetComponent();
				auto cubeShape	= physx.CreateCubeShape({ 1, 1, 1 });
			});

		panelCtx.Pop();
	}


	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		auto& physx = FlexKit::PhysXComponent::GetComponent();
		auto layer  = ctx.GetSceneLayer();

		FK_ASSERT(layer != FlexKit::InvalidHandle);

		const static auto defaultCube = physx.CreateCubeShape(float3{ 1, 1, 1 });

		auto& rigidBodyView = gameObject.AddView<FlexKit::RigidBodyView>(layer);
		rigidBodyView.AddShape(defaultCube);

		return rigidBodyView;
	}


	inline static const std::string name = "Rigid Body Collider";


	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& colliderComponent = static_cast<EditorRigidBodyComponent&>(component);
	}


	inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<RigidBodyEditorComponent, FlexKit::RigidBodyComponentID> _register;


	EditorProject&  project;
	EditorViewport& viewport;
};


/************************************************************************************************/


void RegisterColliderInspector(EditorViewport& viewport, EditorProject& project)
{
	ColliderEditorComponent::Register(viewport, project);
}


/************************************************************************************************/


void RegisterRigidBodyInspector(EditorViewport& viewport, EditorProject& project)
{
	static RigidBodyEditorComponent component{ viewport, project };
	EditorInspectorView::AddComponent(component);
}


/************************************************************************************************/


FlexKit::Blob EditorColliderComponent::GetBlob()
{
	FlexKit::Blob body;
	for (auto& collider : colliders)
		body += collider.shape;

	FlexKit::StaticBodyHeaderBlob headerDesc{
		.shapeCount = (uint32_t)colliders.size(),
	};

	FlexKit::Blob header;
	headerDesc.componentHeader.blockSize = sizeof(headerDesc) + body.size();

	header += headerDesc;

	return header + body;
}


FlexKit::Blob EditorRigidBodyComponent::GetBlob()
{
	FlexKit::SaveArchiveContext archive;
	archive& ColliderResourceID;

	return archive.GetBlob();
}


/************************************************************************************************/


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
