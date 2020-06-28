#ifdef _WIN32
#pragma once
#endif

/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#ifndef EVENT_H
#define EVENT_H

#include "buildsettings.h"
#include "containers.h"
#include "KeycodesEnums.h"

namespace FlexKit
{
	struct FLEXKITAPI Event
	{
		Event& operator = (const Event& rhs)
		{
			memcpy(this, &rhs, sizeof(Event));
			return *this;
		}

		enum EventType
		{
			Input,
			Internal,
			Invalid,
			iObject,
			Property,
			Net,
			World
		} mType;

		enum InputType
		{
			Joystick,
			Keyboard,
			Mouse,
			E_SystemEvent
		} InputSource;

		enum InputAction
		{
			Moved,
			Pressed,
			Release,
			Resized
		} Action;

		enum Error
		{};

		union
		{
			char*						mMessage;
			int64_t						mKC	[2];
			int64_t						mINT[2];
			uint64_t					mSize;
		} mData1, mData2;
	};


	/************************************************************************************************/


	template< typename Ty = Event >
	class EventNotifier
	{
	public:
		template< typename Ty = Event >
		class SubscriberTemplate
		{
		public:
			typedef void (*NotifyFN)(const Ty&, void*);
			NotifyFN Notify;
			void*	_ptr;
		};
		typedef SubscriberTemplate<Ty> Subscriber;

		inline void	NotifyEvent( Event Event )
		{
			for (auto Sub : mSubscribers)
				Sub.Notify( Event, Sub._ptr );
		}
		inline void	 Subscribe( Subscriber Subscriber )
		{
			mSubscribers.push_back(Subscriber);
		}

	private:
		static_vector<Subscriber, 8>	mSubscribers;
	};


	/************************************************************************************************/


	class InputMap
	{
	public:
		InputMap(iAllocator* Memory) :
			EventMap{ Memory }{}


		~InputMap()
		{
			EventMap.Release();
		}

		bool Map(const Event& evt_in, Event& evt_out)
		{
			if (evt_in.InputSource != Event::Keyboard)
				return false;

			auto res = FlexKit::find(
				EventMap, 
				[&](auto &rhs) 
					{
						return evt_in.mData1.mKC[0] == rhs.EventKC;
					});

			if (res == EventMap.end())
				return false;

			evt_out					= evt_in;
			evt_out.mData1.mINT[0]	= (*res).EventID;

			return true;
		}


		template<typename TY_FN>
		bool Handle(const Event& evt_in, TY_FN handler)
		{
			if (evt_in.InputSource != Event::Keyboard)
				return false;

			auto itr = EventMap.begin();
            bool handled = false;
			do {
				itr = std::find_if(
					itr, 
					EventMap.end(),
					[&](const auto& rhs) -> bool
					{
						return evt_in.mData1.mKC[0] == rhs.EventKC;
					});

				if (itr != EventMap.end())
				{
					auto evt = evt_in;
					evt.mData1.mINT[0] = (*itr).EventID;
					handler(evt);
					itr++;

                    handled = true;
				}

			} while (itr != EventMap.end());

            return handled;
		}


		void MapKeyToEvent(KEYCODES KC, int64_t EventID)
		{
			EventMap.push_back({ EventID, KC });
		}


		bool operator ()(const Event& evt_in, Event& evt_out)
		{
			return Map(evt_in, evt_out);
		}

	private:

		struct TiedEvent
		{
			int64_t		EventID;
			KEYCODES	EventKC;
		};

		Vector<TiedEvent> EventMap;
	};

}

#endif
