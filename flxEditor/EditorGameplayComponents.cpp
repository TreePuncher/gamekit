#include "EditorGameplayComponents.hpp"
#include "EditorInspectorView.h"
#include "EditorResourcePickerDialog.h"
#include "ViewportScene.h"

#include "EditorSelectObjectDialog.h"
#include <TriggerSlotIDs.hpp>
#include <fmt/format.h>
#include <qlabel.h>

/************************************************************************************************/


FlexKit::Blob PortalEntityComponent::GetBlob()
{
	FlexKit::PortalComponentBlob portalBlob;
	portalBlob.sceneID			= sceneID;
	portalBlob.spawnObjectID	= spawnObjectID;

	return FlexKit::Blob{ portalBlob };
}


void PortalComponentEventHandler::OnCreateView(FlexKit::GameObject& gameObject, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
{
	auto& triggers		= gameObject.AddView<FlexKit::TriggerView>();
	auto& portalView	= gameObject.AddView<EditorPortalView>();

	triggers->CreateTrigger(FlexKit::ActivateTrigger);
	triggers->CreateSlot(FlexKit::PortalSlot,
		[&gameObject](void* _args, uint64_t)
		{
			FlexKit::Apply(gameObject,
			[](EditorPortalView& portalView)
			{
				portalView->sceneID;
			});
		});

	FlexKit::PortalComponentBlob blob;
	memcpy(&blob, buffer, sizeof(blob));

	portalView->sceneID			= blob.sceneID;
	portalView->spawnObjectID	= blob.spawnObjectID;
}


/************************************************************************************************/


struct PortalEditorComponent final : public IEditorComponent
{
	inline static const std::string name{ "Portal" };

	FlexKit::ComponentID ComponentID() const noexcept { return PortalComponentID; }
	const std::string& ComponentName() const noexcept { return name; }


	PortalEditorComponent(EditorProject& IN_project) :
		project{ IN_project } { }


	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject& gameObject, FlexKit::ComponentViewBase& view) override
	{
		panelCtx.PushVerticalLayout();

		auto& portal = static_cast<EditorPortalView&>(view);


		QLabel* level		= panelCtx.AddText(portal->sceneID != INVALIDHANDLE ? fmt::format("LevelID: {}", portal->sceneID) : "No Level Set");
		QLabel* spawn		= panelCtx.AddText(portal->spawnObjectID != (uint32_t)-1 ? fmt::format("SpawnID ID: {}", portal->spawnObjectID) : "No Spawn Object Set");

		panelCtx.AddButton("Connect to spawn point",
			[&, level, spawn]()
			{
				auto diag = new EditorResourcePickerDialog(SceneResourceTypeID, project, nullptr);
				diag->OnSelection(
					[&, diag, level, spawn](ProjectResource_ptr proj)
					{
						auto* editorScene		= static_cast<FlexKit::SceneResource*>(proj->resource.get());
						auto& editorPortalView	= static_cast<EditorPortalView&>(view);

						auto dialog = new EditorSelectObjectDialog{ editorScene };

						struct Portals
						{
							FlexKit::SceneEntity* entity;
							EntitySpawnComponent* component;
						};
						std::vector<Portals> portals;

						for (auto& entity : editorScene->entities)
						{
							auto component = entity.FindComponent(SpawnPointComponentID);

							if (!component)
								continue;

							auto portal = static_cast<EntitySpawnComponent*>(component.get());

							portals.emplace_back(&entity, portal);
						}

						dialog->GetItemCount =
							[size = portals.size()]() -> size_t
							{
								return size;
							};

						dialog->GetItemLabel =
							[=](size_t i) -> std::string
							{
								auto component = static_cast<FlexKit::EntityStringIDComponent*>(portals[i].entity->FindComponent(FlexKit::StringComponentID).get());

								return fmt::format("{}", component->stringID);
							};

						dialog->OnSelection =
							[=, &editorPortalView, guid = editorScene->guid](size_t i)
							{
								auto idComponent = static_cast<FlexKit::EntityStringIDComponent*>(portals[i].entity->FindComponent(FlexKit::StringComponentID).get());

								auto hash = idComponent->GetStringHash();
								editorPortalView->spawnObjectID	= hash;
								editorPortalView->sceneID		= guid;

								level->setText(fmt::format("LevelID: {}", guid).c_str());
								spawn->setText(fmt::format("SpawnID: {}", editorPortalView->spawnObjectID).c_str());
							};

						dialog->UpdateContents();

						dialog->show();

						diag->close();
					});

				diag->show();
			});

		panelCtx.Pop();
	}


	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return gameObject.AddView<EditorPortalView>();
	}


	static bool Register(EditorProject& project)
	{
		static PortalEditorComponent compoment{ project };
		EditorInspectorView::AddComponent(compoment);

		return true;
	}


	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& view, ViewportSceneContext& scene)
	{
		auto& entityDoorComponent	= static_cast<PortalEntityComponent&>(component);
		auto& editorPortalView		= static_cast<EditorPortalView&>(view);

		entityDoorComponent.sceneID			= editorPortalView->sceneID;
		entityDoorComponent.spawnObjectID	= editorPortalView->spawnObjectID;
	}


	inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<PortalEditorComponent, PortalComponentID> _register;

	inline static EditorPortalComponent component{ FlexKit::SystemAllocator };

	EditorProject& project;
};


void RegisterPortalComponent(EditorProject& project)
{
	PortalEditorComponent::Register(project);
}


/************************************************************************************************/


FlexKit::Blob EntitySpawnComponent::GetBlob()
{
	FlexKit::SpawnComponentBlob spawnBlob;

	spawnBlob.spawnDirection	= Quaternion(0, 0, 0, 1);
	spawnBlob.spawnOffset		= float3(0, 0, 0);

	return FlexKit::Blob{ spawnBlob };
}


void SpawnComponentEventHandler::OnCreateView(FlexKit::GameObject& gameObject, FlexKit::ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
{
	gameObject.AddView<EditorSpawnView>();
}


/************************************************************************************************/


struct SpawnPointComponentFactory final : public IEditorComponent
{
	FlexKit::ComponentViewBase& Construct(FlexKit::GameObject& gameObject, ComponentConstructionContext& ctx)
	{
		return gameObject.AddView<EditorSpawnView>();
	}

	inline static const std::string name = "Spawn";

	const std::string&		ComponentName() const noexcept final { return name; }
	FlexKit::ComponentID	ComponentID()	const noexcept final { return SpawnPointComponentID; }


	static bool Register()
	{
		static SpawnPointComponentFactory component;
		EditorInspectorView::AddComponent(component);

		return true;
	}


	static void Update(FlexKit::EntityComponent& component, FlexKit::ComponentViewBase& base, ViewportSceneContext& scene)
	{
	}

	void Inspect(ComponentViewPanelContext& panelCtx, FlexKit::GameObject& gameObject, FlexKit::ComponentViewBase& view) override
	{
	}

	inline static IEntityComponentRuntimeUpdater::RegisterConstructorHelper<SpawnPointComponentFactory, SpawnPointComponentID> _register;

	inline static bool _registered = Register();
	inline static EditorSpawnComponent component{ FlexKit::SystemAllocator };
};


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2023 Robert May

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
