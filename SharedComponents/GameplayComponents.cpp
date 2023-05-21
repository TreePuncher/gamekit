#include <ComponentBlobs.h>
#include <GameplayComponents.hpp>
#include <Level.hpp>
#include <TriggerComponent.h>
#include <TriggerSlotIDs.hpp>


using namespace FlexKit;


/************************************************************************************************/


void PortalFactory::OnCreateView(
	FlexKit::GameObject&	gameObject,
	FlexKit::ValueMap		userValues,
	const std::byte*		buffer,
	const size_t			bufferSize,
	FlexKit::iAllocator*	allocator)
{
	PortalComponentBlob portal;
	memcpy(&portal, buffer, sizeof(portal));

	auto& triggers			= gameObject.AddView<TriggerView>();
	auto& portalView		= gameObject.AddView<PortalView>();
	portalView->levelID			= portal.sceneID;
	portalView->spawnPointID	= portal.spawnObjectID;

	triggers->CreateTrigger(ActivateTrigger);
	triggers->CreateSlot(PortalSlot,
		[&](void*, uint64_t)
		{
			auto& portalView	= *gameObject.GetView<PortalView>();
			auto levelID		= portalView->levelID;
			auto spawnPointID	= portalView->spawnPointID;

			if (levelID == INVALIDHANDLE)
				return;

			if (!LoadLevel(levelID, *core))
				throw std::runtime_error("Failed to load Level!");

			auto currentLevel	= GetActiveLevel();
			auto level			= GetLevel(levelID);

			if (!level)
				return;

			level->scene.QueryFor(
				[&](auto& gameObject, auto&& res)
				{
					const float3 newPosition = GetWorldPosition(gameObject);

					SetControllerPosition(*playerObject, newPosition);

					if (levelID != GetActiveLevelID())
					{
						// Remove Character Controller
						// ReAdd new Character controller in new layer
						SetActiveLevel(levelID);

						auto& physx = PhysXComponent::GetComponent();

						physx.GetLayer_ref(currentLevel->layer).paused = true;
						physx.GetLayer_ref(level->layer).paused = false;

						currentLevel->scene.RemoveEntity(*playerObject);
						level->scene.AddGameObject(*playerObject);

						Apply(gameObject,
							[&](CharacterControllerView& ccv)
							{
								ccv.ChangeLayer(level->layer);
							});
					}
				},
				FlexKit::StringHashQuery{ spawnPointID });
		});

	triggers->Connect(ActivateTrigger, PortalSlot);
}


/************************************************************************************************/


void SpawnFactory::OnCreateView(
	FlexKit::GameObject&	gameObject,
	FlexKit::ValueMap		user_ptr,
	const std::byte*		buffer,
	const size_t			bufferSize,
	FlexKit::iAllocator*	allocator)
{
	SpawnComponentBlob spawn;
}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
