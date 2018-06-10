/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "..\buildsettings.h"
#include "..\graphicsutilities\graphics.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\Events.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\type.h"



#ifndef COMPONENT_H
#define COMPONENT_H

namespace FlexKit
{
	// GOAL: Help reduce code involved to do shit which is currently excessive 
	/************************************************************************************************/

	// Components store all data needed for a component
	typedef FlexKit::Handle_t<32> ComponentHandle;

	const ComponentHandle InvalidComponentHandle(-1);

	typedef uint32_t EventTypeID;

	typedef uint32_t ComponentType;

	const	uint32_t UnknownComponentID = GetTypeGUID(Component);
	

	/************************************************************************************************/


	class ComponentSystemInterface
	{
	public:
		virtual ~ComponentSystemInterface() {}
		virtual void ReleaseHandle(ComponentHandle Handle) = 0;
		virtual void Update(double dt) {}
		
		const Vector<ComponentSystemInterface*> GetSystemDependencies() { return {}; }
	};


	/************************************************************************************************/


	class BehaviorBase
	{
	public:
		virtual void Release() {}
	};


	class iEventReceiver
	{
	public:
		virtual void Handle(const Event& evt) {}
	};



}	/************************************************************************************************/

#endif