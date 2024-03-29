#include "PCH.h"
#include "AnimationComponents.h"
#include "CSGComponent.h"
#include "EditorSceneEntityComponents.h"
#include "ViewportScene.h"


/************************************************************************************************/


int ViewportSceneContext::MapNode(FlexKit::NodeHandle node) noexcept
{
	if (node == FlexKit::InvalidHandle || node == FlexKit::NodeHandle{ 0 })
		return -1;

	if (auto res = nodeMap.find(node); res != nodeMap.end())
		return res->second;
	else
	{
		auto parent				= MapNode(FlexKit::GetParentNode(node));
		auto localPosition		= FlexKit::GetPositionL(node);
		auto localOrientation	= FlexKit::GetOrientation(node);
		auto localScale			= FlexKit::GetLocalScale(node);

		auto idx = nodes.size();
		nodes.emplace_back(localPosition, localOrientation, localScale, parent);

		nodeMap[node] = idx;
		return idx;
	}
}


/************************************************************************************************/


struct EntityStringIDComponent
{
	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& stringID	= static_cast<FlexKit::EntityStringIDComponent&>(component);
		auto& runtime	= static_cast<FlexKit::StringIDView&>(base);

		stringID.stringID = runtime.GetString();
	}

	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<EntityStringIDComponent, FlexKit::StringComponentID> _register;
};


struct TransformComponentUpdate
{
	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& sceneNode	= static_cast<FlexKit::EntitySceneNodeComponent&>(component);
		auto& runtime	= static_cast<FlexKit::SceneNodeView&>(base);

		sceneNode.nodeIdx = scene.MapNode(runtime.node);

	}

	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<TransformComponentUpdate, FlexKit::TransformComponentID> _register;
};


struct BrushComponentUpdate
{
	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& brush			= static_cast<FlexKit::EntityBrushComponent&>(component);
		auto& runtime		= static_cast<FlexKit::BrushView&>(base);
		auto& runtimeBrush	= runtime.GetBrush();

		//brush.material = runtimeBrush.material;
		// TODO: update material component data
		brush.meshes.clear();

		for (auto mesh : runtimeBrush.meshes)
			brush.meshes.push_back(FlexKit::GetMeshResource(mesh)->assetHandle);
	}

	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<BrushComponentUpdate, FlexKit::BrushComponentID> _register;
};


struct MaterialComponentUpdate
{
	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& material	= static_cast<FlexKit::EntityMaterialComponent&>(component);
		auto& runtime	= static_cast<FlexKit::MaterialView&>(base);
	}

	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<MaterialComponentUpdate, FlexKit::MaterialComponentID> _register;
};


struct SkeletonComponentUpdate
{
	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& skeleton	= static_cast<FlexKit::EntitySkeletonComponent&>(component);
		auto& runtime	= static_cast<FlexKit::SkeletonView&>(base);

		skeleton.skeletonResourceID = runtime.GetSkeleton()->guid;
	}

	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<SkeletonComponentUpdate, FlexKit::SkeletonComponentID> _register;
};


struct LightComponentUpdate
{
	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
		auto& light		= static_cast<FlexKit::EntityLightComponent&>(component);
		auto& runtime	= static_cast<FlexKit::LightView&>(base);

		light.I				= runtime.GetIntensity();
		light.K				= runtime.GetK();
		light.R				= runtime.GetRadius();
		light.outerRadius	= runtime.GetOuterAngle();
		light.innerRadius	= runtime.GetInnerAngle();
		light.size			= runtime.GetSize();
		light.type			= (uint32_t)runtime.GetType();
	}

	IEntityComponentRuntimeUpdater::RegisterConstructorHelper<LightComponentUpdate, FlexKit::LightComponentID> _register;
};


/************************************************************************************************/


ViewportObjectList ViewportScene::RayCast(FlexKit::Ray v) const
{
	auto&		visables		= FlexKit::SceneVisibilityComponent::GetComponent();
	const auto  rayCastResults	= scene.RayCast(v, FlexKit::SystemAllocator);

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


ViewportGameObject_ptr ViewportScene::CreateObject(ViewportGameObject* parent)
{
	auto obj = std::make_shared<ViewportGameObject>();
	obj->objectID = rand();

	sceneObjects.push_back(obj);

	if (parent)
	{
		auto parentNode = FlexKit::GetSceneNode(parent->gameObject);

		if(parentNode == FlexKit::InvalidHandle)
			parentNode = parent->gameObject.AddView<FlexKit::SceneNodeView>();

		auto& node = obj->gameObject.AddView<FlexKit::SceneNodeView>();
		node.SetParentNode(parentNode);
	}

	OnSceneChange();

	return obj;
}


/************************************************************************************************/


ViewportGameObject_ptr  ViewportScene::FindObject(uint64_t ID)
{
	auto res = std::find_if(
		std::begin(sceneObjects),
		std::end(sceneObjects),
		[&](auto& obj)
		{
			return obj->objectID == ID;
		});

	return res != std::end(sceneObjects) ? *res : nullptr;
}


/************************************************************************************************/


ViewportGameObject_ptr  ViewportScene::FindObject(FlexKit::NodeHandle node)
{
	auto res = std::find_if(
		std::begin(sceneObjects),
		std::end(sceneObjects),
		[&](auto& obj)
		{
			if (obj->gameObject.hasView(FlexKit::TransformComponentID))
				return FlexKit::GetSceneNode(obj->gameObject) == node;
			else
				return false;
		});

	return res != std::end(sceneObjects) ? *res : nullptr;
}


/************************************************************************************************/


DeletionHandle::DeletionHandle(
	ViewportGameObject_ptr	obj,
	ViewportScene*			scene,
	bool					objInScene,
	bool					removed) :
	controlSection{ new ControlSection{} }
{
	controlSection->obj_ptr			= obj;
	controlSection->scene			= scene;
	controlSection->removed			= removed;
	controlSection->objectInScene	= objInScene;
}


/************************************************************************************************/


DeletionHandle::DeletionHandle(const DeletionHandle& rhs) :
	controlSection{ rhs.controlSection }
{
	controlSection->ref_count++;
}


/************************************************************************************************/


DeletionHandle::~DeletionHandle()
{
	if (controlSection)
	{
		if(controlSection->ref_count.fetch_sub(1) == 1)
			delete controlSection;

		controlSection = nullptr;
	}
}


/************************************************************************************************/


DeletionHandle& DeletionHandle::operator = (const DeletionHandle& rhs)
{
	controlSection = rhs.controlSection;
	controlSection->ref_count++;

	return *this;
}


/************************************************************************************************/


void DeletionHandle::UndoDelete()
{
	if (controlSection)
		controlSection->UndoDelete();
}


/************************************************************************************************/


void DeletionHandle::RedoDelete()
{
	if (controlSection)
		controlSection->RedoDelete();
}


/************************************************************************************************/


ViewportGameObject_ptr DeletionHandle::GetObj()
{
	return controlSection ? controlSection->obj_ptr : nullptr;
}


/************************************************************************************************/


void DeletionHandle::ControlSection::UndoDelete()
{
	if (removed)
	{
		scene->_ReAddObject(obj_ptr);
		removed = false;
	}
}


/************************************************************************************************/


void DeletionHandle::ControlSection::RedoDelete()
{
	if (!removed)
	{
		scene->_RemoveObject(obj_ptr);
		removed = true;
	}
}


/************************************************************************************************/


DeletionHandle ViewportScene::RemoveObject(ViewportGameObject_ptr object)
{
	if (!object)
		return DeletionHandle{};

	_RemoveObject(object);

	DeletionHandle handle{
		object,
		this,
		object->gameObject.hasView(FlexKit::SceneVisibilityComponentID),
		true
	};

	return handle;
}


/************************************************************************************************/


void ViewportScene::_RemoveObject(ViewportGameObject_ptr object)
{
	markedForDeletion.push_back(object->objectID);
	scene.RemoveEntity(object->gameObject);

	std::erase_if(sceneObjects,
		[&](auto& obj)
		{
			return obj == object;
		});

	OnSceneChange();
}


/************************************************************************************************/


void ViewportScene::_ReAddObject(ViewportGameObject_ptr object)
{
	std::erase(markedForDeletion, object->objectID);
	scene.AddGameObject(object->gameObject);
	sceneObjects.push_back(object);

	OnSceneChange();
}


/************************************************************************************************/


FlexKit::LayerHandle ViewportScene::GetLayer()
{
	if(physicsLayer == FlexKit::InvalidHandle)
		physicsLayer = FlexKit::PhysXComponent::GetComponent().CreateLayer(true);

	return physicsLayer;
}


/************************************************************************************************/


void ViewportScene::Update()
{
	if (!sceneResource)
		return;

	auto& scene_impl	= sceneResource->resource->Object();
	auto& entities		= scene_impl->entities; // TODO: make this not dumb

	sceneResource->resource->dirtyFlag = true;

	ViewportSceneContext ctx;

	for (auto& object : sceneObjects)
	{
		std::vector<uint64_t> componentIds;

		auto entity_res = std::ranges::find_if(entities,
			[&](const FlexKit::SceneEntity& entity)
			{
				return (entity.objectID == object->objectID);
			});

		if (entity_res != entities.end())
		{   // Update Data
			for (auto& component : object->gameObject)
			{
				componentIds.push_back(component.GetID());
				auto component_res = entity_res->FindComponent(component.GetID());

				if (!component_res)
				{
					auto entityComponent = FlexKit::EntityComponent::CreateComponent(component.GetID());

					if (!entityComponent)
						continue;

					entity_res->components.emplace_back(FlexKit::EntityComponent_ptr(entityComponent));

					IEntityComponentRuntimeUpdater::Update(*entityComponent, component.Get_ref(), ctx);
				}
				else
					IEntityComponentRuntimeUpdater::Update(*component_res, component.Get_ref(), ctx);
			}

			auto range = std::ranges::partition(
				entity_res->components,
				[&](FlexKit::EntityComponent_ptr& element) -> bool
				{
					return std::ranges::find(componentIds, element->id) != componentIds.end();
				});

			entity_res->components.erase(range.begin(), range.end());
		}
		else
		{   // Add GameObject
			auto& entity	= entities.emplace_back();
			entity.objectID = object->objectID;

			for (auto& component : object->gameObject)
			{
				componentIds.push_back(component.GetID());

				if (auto entityComponent = FlexKit::EntityComponent::CreateComponent(component.GetID()); entityComponent)
				{
					IEntityComponentRuntimeUpdater::Update(*entityComponent, component.Get_ref(), ctx);
					entity.components.emplace_back(entityComponent);
				}
			}
		}
	}

	auto& nodes = sceneResource->resource->Object()->nodes;
	nodes.clear();

	for (const auto& node : ctx.nodes)
	{
		FlexKit::SceneNode sceneNode;
		sceneNode.scale			= node.scale;
		sceneNode.orientation	= node.orientation;
		sceneNode.position		= node.position;
		sceneNode.parent		= node.parent;

		nodes.emplace_back(sceneNode);
	}

	auto eraseRange = std::ranges::remove_if(
		entities,
		[&](auto& entity)
		{
			auto res = std::ranges::find_if(
				markedForDeletion,
				[&](auto& id)
				{
					return entity.objectID == id;
				});

			return res != markedForDeletion.end();
		});

	entities.erase(eraseRange.begin(), eraseRange.end());

	markedForDeletion.clear();
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
