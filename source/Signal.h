#pragma once

/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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


#include "buildsettings.h"
#include "type.h"
#include "memoryutilities.h"
#include "containers.h"
#include <algorithm>
#include <functional>


namespace FlexKit
{
	using FlexKit::Type_t;


	template<typename FNDef = void ()>
	class Signal
	{
	public:
		class Slots
		{
		public:
            Slots() {}

			~Slots()
			{
				for( auto entry : signalTable )
                    entry.signal->Disconnect( this, entry.ID );
			}

			void Release(Signal* signal)
			{
				for (auto entry : signalTable)
				{
					if (entry.signal == signal)
					{
                        entry.signal->Disconnect(this, entry.ID);
                        signalTable.erase(&entry);
                        return;
					}
				}
			}

			void NotifyOnDestruction( Signal* _ptr, uint64_t id )
			{
                signalTable.push_back( Signal_Entry( _ptr, id ) );
			}
		private:

			struct Signal_Entry
			{
				Signal*		signal;
                uint64_t    ID;
			};

			static_vector<Signal_Entry>	signalTable;
		};


	private:
		struct Slot_Entry
		{
            TypeErasedCallable<FNDef, 48>   callable;
            uint64_t                        ID;
			Slots*			                Ptr;
		};

	public:
		Signal(iAllocator* allocator = SystemAllocator) : outputSlots{ allocator } {}

		~Signal( void )
		{
			DiconnectAll();
		}

        template<typename ... TY_args>
		void operator()(TY_args&& ... args)
		{
			for( auto slot : outputSlots )
				slot.callable(std::forward<TY_args>(args)...);
		}

        template<typename TY_Callable>
		void Connect(Slots& slot, TY_Callable callable)
		{
            slot.NotifyOnDestruction(this, outputSlots.size() );
            outputSlots.emplace_back(Slot_Entry{ callable, outputSlots.size(), &slot });
		}

		void Disconnect(Slots* _ptr, unsigned int ID )
		{
			auto end = outputSlots.end();

			for( auto itr = outputSlots.begin(); itr != end; itr++ )
			{
				auto entry = *itr;
				if( entry.Ptr == _ptr && entry.ID == ID )
				{
					outputSlots.remove_unstable(itr);
					break;
				}
			}
		}

		void DiconnectAll()
		{
			for( auto entry : outputSlots )
				entry.Ptr->Release( this );

			outputSlots.clear();
		}

	private:
		Vector<Slot_Entry> outputSlots;
	};
}
