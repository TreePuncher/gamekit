#ifdef _WIN32
#pragma once
#endif

/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#include "..\buildsettings.h"
#include "containers.h"

namespace FlexKit
{
	struct FLEXKITAPI Event
	{
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
			int32_t						mKC	[2];
			int32_t						mINT[2];
			uint64_t					mSize;
		} mData1, mData2;
	};

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
}

#endif