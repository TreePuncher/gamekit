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

#pragma once
#include "buildsettings.h"
#include "events.h"
#include "containers.h"
#include "mathutils.h"

#include <string>

#ifndef INPUT_H
#define INPUT_H

namespace FlexKit
{
	enum KEYCODES;

	class InputInterface;
	class InputObserver;
	class iRenderWindow;
	struct Event;

	class iRenderWindow;
	struct Event;
	enum KEYCODES;

	// Forward Declarations
	class MouseEventContainer;
	class KeyEventContainer;
	struct Event;

	class KeyEventContainer
	{
	public:
		KeyEventContainer( void ) {}
		template< typename Ty >
		inline static Ty* castBack( KeyEventContainer* Container )
		{
			return static_cast< Ty* >( Container );
		}
	};

	class MouseEventContainer
	{
	public:
		MouseEventContainer( void ) {}
		template< typename Ty >
		inline static Ty* castBack( MouseEventContainer* Container )
		{
			return static_cast< Ty* >( Container );
		}
	};

	class MouseInputOberver
	{
	public:
		virtual void MouseMoved		( MouseEventContainer* ) = 0;
		virtual void MousePressed	( MouseEventContainer* ) = 0;
		virtual void MouseReleased	( MouseEventContainer* ) = 0;
	};

	class KeyInputOberver
	{
	public:
		virtual void KeyPressed	( KeyEventContainer* ) = 0;
		virtual void KeyReleased( KeyEventContainer* ) = 0;
	};

	class InputObserver
	{
	public:
		virtual void KeyPressed	( Event* )	= 0;
		virtual void KeyReleased( Event* )	= 0;

		virtual void MouseMoved		( Event* ) = 0;
		virtual void MousePressed	( Event* ) = 0;
		virtual void MouseReleased	( Event* ) = 0;
	};

	class InputInterface
	{
	public:
		virtual ~InputInterface() {}

		virtual void				CaptureKeyboard()											= 0;
		virtual void				CaptureMouse()												= 0;
	
		virtual void				Load( unsigned long HWND )									= 0;
	
		virtual std::deque<Event>&	GetEventQueue()												= 0;
		virtual void				GetMouseRelativemotion( uint2& )							= 0;
		virtual void				GetMouseAbsolutePosition( uint2& )							= 0;
	
		virtual bool				isKeyDown( KEYCODES )										= 0;
		virtual void				KeyPressed()												= 0;
		virtual void				KeyReleased()												= 0;
	
		virtual void				MouseMoved()												= 0;
		virtual void				MouseButtonPressed()										= 0;
		virtual void				MouseButtonReleased()										= 0;
			
		virtual void				SetWindowSize( unsigned int, unsigned int )					= 0;
		virtual void				SetMainInputWindow( iRenderWindow* )						= 0;
	
		virtual void				RegisterKeyMappedEvent	( KEYCODES, const Event&, bool )	= 0;
		virtual void				RegisterMouseObserver	( MouseInputOberver * )				= 0;
		virtual void				RegisterKeyboardObserver( KeyInputOberver * )				= 0;
		virtual void				RegisterInputObserver	( InputObserver* )					= 0;
		virtual void				Subscribe( EventNotifier<>::Subscriber* SubscribeMe )		= 0;

		// Inherited Members
		virtual const std::string		GetName()				= 0;
		virtual bool					isLoaded()				= 0;
	};
}

#endif