#ifndef INPUTCOMPONENT_H
#define INPUTCOMPONENT_H

/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "GameFramework.h"
#include "..\coreutilities\Components.h"

struct PlayerInputState
{
	bool Forward;
	bool Backward;
	bool Left;
	bool Right;
	bool Shield;

	void ClearState()
	{
		Forward  = false;
		Backward = false;
		Left     = false;
		Right    = false;
	}
};


namespace FlexKit
{
	struct InputComponentSystem : public ComponentSystemInterface
	{
		InputComponentSystem(GameFramework* F)
		{
			Listeners.Allocator		= F->Core->GetBlockMemory();
			TargetSystems.Allocator = F->Core->GetBlockMemory();
			Framework				= F;
		}

		void Update			(double dt, MouseInputState MouseInput, GameFramework* Framework);

		ComponentHandle BindInput(ComponentHandle Handle, ComponentSystemInterface* System)
		{
			Listeners.push_back(Handle);
			TargetSystems.push_back(System);

			return ComponentHandle(Listeners.size() - 1);
		}

		MouseInputState	GetMouseState();

		void ReleaseHandle	(ComponentHandle Handle){}

		Vector<ComponentHandle>				Listeners;
		Vector<ComponentSystemInterface*>	TargetSystems;

		GameFramework*						Framework;
		PlayerInputState					KeyState;

		operator InputComponentSystem* (){return this;}
	};

	const uint32_t InputComponentID = GetTypeGUID(INPUTCOMPONENT);
}
#endif