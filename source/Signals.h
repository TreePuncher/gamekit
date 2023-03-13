#pragma once

#include "buildsettings.h"
#include "type.h"
#include "memoryutilities.h"
#include "containers.h"

#include <algorithm>
#include <functional>


namespace FlexKit
{	/************************************************************************************************/


	using FlexKit::Type_t;


	template<typename FNDef = void ()>
	class Signal
	{
	public:
		class Slot
		{
		public:
			Slot(iAllocator& allocator = SystemAllocator) :
				signalTable{ allocator } {}

			~Slot()
			{
				for(auto& entry : signalTable)
					entry.signal->Disconnect(this, entry.ID);

				signalTable.clear();
			}


			void Remove(Signal* signal)
			{
				for (auto& entry : signalTable)
				{
					if (entry.signal == signal)
					{
						entry.signal->Disconnect(this, entry.ID);
						signalTable.remove_unstable(&entry);
						return;
					}
				}
			}


			void Remove(const uint64_t ID)
			{
				for (auto& entry : signalTable)
				{
					if (entry.ID == ID)
					{
						signalTable.remove_unstable(&entry);
						return;
					}
				}
			}


			Slot(Slot&& rhs) noexcept :
				signalTable{ std::move(rhs.signalTable) }
			{
				for (SignalEntry& slot : signalTable)
					slot.signal->MoveSlot(&rhs, this);
			}


			Slot& operator =	(Slot&& rhs)
			{
				signalTable = std::move(rhs.signalTable);

				for (auto& slot : rhs.signalTable)
					slot.signal->MoveSlot(&rhs, this);

				return *this;
			}


					Slot		(const Slot& rhs) = delete;
			Slot&	operator =	(const Slot& rhs) = delete;


			void NotifyOnDestruction(Signal* _ptr, uint64_t id)
			{
				signalTable.push_back(SignalEntry{ _ptr, id });
			}


			void MoveSignal(Signal* from, Signal* to)
			{
				for (SignalEntry& signalEntry : signalTable)
				{
					if (signalEntry.signal == from)
						signalEntry.signal = to;
				}
			}


			template<typename TY_Callable>
			void Bind(TY_Callable IN_callable)
			{
				callable = IN_callable;
			}


			template<typename ... TY_args>
			void operator()(TY_args&& ... args)
			{
				if(callable)
					callable(std::forward<TY_args>(args)...);
			}


		private:

			struct SignalEntry
			{
				Signal*		signal;
				uint64_t	ID;
			};

			TypeErasedCallable<FNDef, 32>		callable;
			Vector<SignalEntry, 4, uint32_t>	signalTable;
		};

	protected:

		void MoveSlot(Slot* from, Slot* to)
		{
			for (SlotEntry& outputSlot : outputSlots)
			{
				if (outputSlot.slot == from)
					outputSlot.slot = to;
			}
		}

	private:
		struct SlotEntry
		{
			uint64_t	ID;
			Slot*		slot;

			TypeErasedCallable<FNDef, 32>*	callable = nullptr;
		};

	public:
		Signal(iAllocator* IN_allocator = SystemAllocator) :
			outputSlots	{ IN_allocator },
			allocator	{ IN_allocator } {}

		~Signal()
		{
			DiconnectAll();
		}


		Signal(Signal&& rhs) noexcept :
			outputSlots	{ std::move(rhs.outputSlots) },
			allocator	{ rhs.allocator }
		{
			for (SlotEntry& slot : outputSlots)
				slot.slot->MoveSignal(&rhs, this);

			rhs.allocator = nullptr;
		}


		Signal& operator =	(Signal&& rhs)
		{
			outputSlots = std::move(rhs.outputSlots);
			allocator	= rhs.allocator;

			for (SlotEntry& slot : outputSlots)
				slot.slot->MoveSignal(&rhs, this);

			rhs.allocator = nullptr;
		}


				Signal		(const Signal& rhs) = delete;
		Signal&	operator =	(const Signal& rhs) = delete;


		template<typename ... TY_args>
		void operator()(TY_args&& ... args)
		{
			for (auto& slot : outputSlots)
			{
				if (slot.callable)
					(*slot.callable)(std::forward<TY_args>(args)...);
				else
					(*slot.slot)(std::forward<TY_args>(args)...);
			}
		}


		template<typename TY_Callable>
		uint64_t Connect(Slot& slot, TY_Callable callable)
		{
			slot.NotifyOnDestruction(this, outputSlots.size() );
			slot.Bind(callable);

			return outputSlots.emplace_back(
				SlotEntry{
					outputSlots.size(),
					&slot,
					new TypeErasedCallable<FNDef, 32>{ callable } });
		}


		uint64_t Connect(Slot& slot)
		{
			slot.NotifyOnDestruction(this, outputSlots.size());

			return outputSlots.emplace_back(SlotEntry{ outputSlots.size(), &slot });
		}


		void Disconnect(Slot* slot, const uint64_t ID)
		{
			const auto end = outputSlots.end();

			for(auto itr = outputSlots.begin(); itr != end; itr++)
			{
				auto entry = *itr;
				if(itr->slot == slot && entry.ID == ID)
				{
					itr->slot->Remove(ID);

					if (itr->callable)
						allocator->release(*itr->callable);

					outputSlots.remove_unstable(itr);
					break;
				}
			}
		}


		void DiconnectAll()
		{
			for( auto entry : outputSlots )
				entry.slot->Remove(this);

			outputSlots.clear();
		}


	private:
		Vector<SlotEntry, 4, uint32_t>	outputSlots;
		iAllocator*						allocator = nullptr;
	};


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

