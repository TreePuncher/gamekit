#pragma once

#include "TriggerComponent.h"
#include "ComponentBlobs.h"
#include "KeyValueIds.h"
#include "Scene.h"
#include "SceneLoadingContext.h"

namespace FlexKit
{	/************************************************************************************************/


	void TriggerData::CreateTrigger(uint32_t id)
	{
		if (GetTrigger(id) != nullptr)
			return;

		triggers.emplace_back(allocator);
		triggerIDs.emplace_back(id);
	}


	/************************************************************************************************/


	size_t TriggerData::GetSignalIdx(uint32_t id) const
	{
		const size_t end = triggerIDs.size();

		for (size_t I = 0; I < end; I++)
			if (triggerIDs[I] == id)
				return I;

		return -1;
	}


	/************************************************************************************************/


	size_t	TriggerData::GetSlotIdx(uint32_t id) const
	{
		const size_t end = actionSlotIDs.size();

		for (size_t I = 0; I < end; I++)
			if (actionSlotIDs[I] == id)
				return I;

		return -1;
	}


	/************************************************************************************************/


	Signal_t* TriggerData::GetTrigger(uint32_t id)
	{
		auto idx = GetSignalIdx(id);
		return idx != -1 ? &triggers[id] : nullptr;
	}


	/************************************************************************************************/


	Slot_t* TriggerData::GetSlot(uint32_t id)
	{
		auto idx = GetSlotIdx(id);
		return idx != -1 ? &actionSlots[id] : nullptr;
	}


	/************************************************************************************************/


	void TriggerData::Connect(const uint32_t trigger, const uint32_t slot)
	{
		const auto triggerIdx	= GetSignalIdx(trigger);
		const auto slotIdx		= GetSlotIdx(slot);

		if (triggerIdx == -1 || slotIdx == -1)
			return;

		auto& trigger_ref	= triggers[triggerIdx];
		auto& slot_ref		= actionSlots[slotIdx];

		trigger_ref.Connect(slot_ref);
	}


	/************************************************************************************************/


	void TriggerData::RemoveSlot(const uint32_t slot)
	{

	}


	void TriggerData::RemoveTrigger(const uint32_t signal)
	{

	}


	/************************************************************************************************/


	void TriggerData::Trigger(uint32_t id, void* args, uint64_t ID)
	{
		const size_t end = triggerIDs.size();

		for (size_t I = 0; I < end; I++)
			if (triggerIDs[I] == id)
				triggers[I](args, ID);
	}


	/************************************************************************************************/


	void TriggerData::TriggerSlot(uint32_t id, void* args, uint64_t ID)
	{
		const size_t end = triggerIDs.size();

		for (size_t I = 0; I < end; I++)
			if (actionSlotIDs[I] == id)
				actionSlots[I](args, ID);
	}


	/************************************************************************************************/


	TriggerData TriggerComponentEventHandler::OnCreate(GameObject&) noexcept
	{
		return TriggerData{
			.triggers		= { allocator },
			.triggerIDs		= { allocator },
			.actionSlots	= { allocator },
			.actionSlotIDs	= { allocator },
			.allocator		= allocator
		};
	}


	/************************************************************************************************/


	void TriggerComponentEventHandler::OnCreateView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) noexcept
	{
		auto ctx = FindValue<SceneLoadingContext*>(userValues, SceneLoadingContextKID).value_or(nullptr);

		auto& view = gameObject.AddView<TriggerView>();

		TriggerComponentBlob triggerBlob;
		memcpy(&triggerBlob, (void*)buffer, sizeof(triggerBlob));

		const size_t currentSignalSize	= view->triggers.size();
		const size_t currentSlotSize	= view->actionSlots.size();

		view->triggers.reserve(triggerBlob.triggerCount + currentSignalSize);
		view->triggerIDs.reserve(triggerBlob.triggerCount + currentSignalSize);

		view->actionSlots.reserve(triggerBlob.slotCount + currentSignalSize);
		view->actionSlotIDs.reserve(triggerBlob.slotCount + currentSignalSize);

		for (size_t itr = 0; itr < triggerBlob.triggerCount; itr++)
		{
			uint32_t id;

			memcpy(
				&id,
				buffer +
					triggerBlob.TriggerOffset() +
					sizeof(uint32_t) * itr,
				sizeof(id));

			view->triggers.emplace_back(*allocator);
			view->triggerIDs.emplace_back(id);
		}

		for (size_t itr = 0; itr < triggerBlob.slotCount; itr++)
		{
			uint32_t id;

			memcpy(
				&id,
				buffer +
					triggerBlob.SlotOffset() + 
					sizeof(uint32_t) * itr,
				sizeof(id));

			view->actionSlots.emplace_back(*allocator);
			view->actionSlotIDs.emplace_back(id);
		}

		for (size_t itr = 0; itr < triggerBlob.slotCount; itr++)
		{
			InterObjectConnection connection;

			memcpy(
				&connection,
				buffer +
				triggerBlob.ConnectionOffset() +
				sizeof(uint32_t) * itr,
				sizeof(connection));

			if (ctx)
				ctx->pendingActions.emplace_back(
					[connection, &view, &gameObject](Scene& scene)
					{
						using results_ty = QueryResultObject<StringHashQuery>;
						static_vector<results_ty> results;

						scene.QueryFor(
							[&](GameObject& slotObject, results_ty& results)
							{
								ConnectTrigger(gameObject, connection.triggerID, slotObject, connection.slotID);
							},
							StringHashQuery{ connection.foreignObjectID });
					});
		}
	}


	/************************************************************************************************/


	void Trigger(GameObject& gameObject, uint32_t triggerID, void* args, uint64_t id)
	{
		Apply(
			gameObject,
			[&](TriggerView& triggers)
			{
				triggers->Trigger(triggerID, args, id);
			});
	}


	/************************************************************************************************/


	void TriggerSlot(GameObject& gameObject, uint32_t slotID, void* args, uint64_t id)
	{
		Apply(
			gameObject,
			[&](TriggerView& triggers)
			{
				triggers->TriggerSlot(slotID, args, id);
			});
	}


	/************************************************************************************************/


	void ConnectTrigger(GameObject& gameObject, uint32_t triggerID, uint32_t slotID)
	{
		Apply(
			gameObject,
			[&](TriggerView& triggers)
			{
				triggers->Connect(triggerID, slotID);
			});
	}


	/************************************************************************************************/


	void ConnectTrigger(GameObject& triggerObject, uint32_t triggerID, GameObject& slotObject, uint32_t slotID)
	{
		auto* triggerObjectView = triggerObject.GetView<TriggerView>();
		auto* slotObjectView	= slotObject.GetView<TriggerView>();

		if (triggerObjectView == nullptr && slotObjectView == nullptr)
			return;

		auto trigger	= (*triggerObjectView)->GetTrigger(triggerID);
		auto slot		= (*slotObjectView)->GetSlot(slotID);

		if (trigger == nullptr || slot == nullptr)
			return;

		trigger->Connect(*slot);
	}


}	/************************************************************************************************/


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
