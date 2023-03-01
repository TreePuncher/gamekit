#pragma once

#include <any>

#include "Signals.h"
#include "Components.h"
#include "RuntimeComponentIDs.h"
#include "Handle.h"


namespace FlexKit
{
	using Signal_t		= Signal<void (void*, uint64_t)>;
	using Slot_t		= Signal<void (void*, uint64_t)>::Slot;

	using SignalVector	= Vector<Signal_t>;
	using SlotVector	= Vector<Signal<void (void*, uint64_t)>::Slot>;

	constexpr uint64_t NullTypeID = 0xffffffffffffffff;

	struct TriggerData
	{
		void	CreateTrigger(uint32_t);

		Slot_t& CreateSlot(uint32_t id, auto fn)
		{
			auto idx = actionSlots.emplace_back();
			actionSlotIDs.emplace_back(id);

			auto& slot_ref = actionSlots[idx];
			slot_ref.Bind(fn);


			return actionSlots[idx];
		}

		Slot_t& CreateSlot(uint32_t id)
		{
			auto idx = actionSlots.emplace_back();
			actionSlotIDs.emplace_back(id);

			return actionSlots[idx];
		}

		void		Trigger(uint32_t,		void* args = nullptr, uint64_t typeID = NullTypeID);
		void		TriggerSlot(uint32_t,	void* args = nullptr, uint64_t typeID = NullTypeID);

		size_t		GetSignalIdx(uint32_t id) const;
		size_t		GetSlotIdx(uint32_t id) const;

		Signal_t*	GetTrigger(uint32_t id);
		Slot_t*		GetSlot(uint32_t id);

		void Connect(uint32_t id, const uint32_t slot, auto&& connectMe)
		{
			const auto triggerIdx	= GetSignalIdx(id);
			const auto slotIdx		= GetSlotIdx(slot);

			if (triggerIdx == -1 || slotIdx == -1)
				return;

			auto& trigger_ref	= triggers[triggerIdx];
			auto& slot_ref		= actionSlots[slotIdx];

			trigger_ref.Connect(slot_ref, connectMe);
		}

		void Connect(const uint32_t signal, const uint32_t slot);

		void RemoveSlot		(const uint32_t slot);
		void RemoveTrigger	(const uint32_t signal);


		SignalVector		triggers;
		Vector<uint32_t>	triggerIDs;

		Vector<Signal<void (void*, uint64_t)>::Slot>	actionSlots;
		Vector<uint32_t>								actionSlotIDs;

		std::any			userData;
	};

	struct TriggerComponentEventHandler
	{
		TriggerComponentEventHandler(iAllocator& IN_allocator) :
			allocator	{ &IN_allocator } {}

		TriggerData OnCreate() noexcept;
		TriggerData OnCreate(auto&& args) noexcept;

		void OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) noexcept;

		iAllocator* allocator;
	};

	using TriggerHandle			= Handle_t<32, TriggerComponentID>;
	using TriggerComponent		= BasicComponent_t<TriggerData, TriggerHandle, TriggerComponentID, TriggerComponentEventHandler>;
	using TriggerView			= TriggerComponent::View;

	void Trigger	(GameObject& gameObject, uint32_t id, void* _ptr = nullptr, uint64_t _ptr_id = NullTypeID);
	void TriggerSlot(GameObject& gameObject, uint32_t id, void* _ptr = nullptr, uint64_t _ptr_id = NullTypeID);

	void ConnectTrigger(GameObject& triggerObject, uint32_t triggerID, uint32_t slotID);
	void ConnectTrigger(GameObject& triggerObject, uint32_t triggerID, GameObject& slotObject, uint32_t slotID);
}


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
