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

#include "stdafx.h"
#include "InputComponent.h"

#include <iostream>

namespace FlexKit
{
	/************************************************************************************************/


	void InputComponentSystem::Update(double dt, MouseInputState MouseInput, GameFramework* Framework)
	{
		float HorizontalMouseMovement	= float(MouseInput.dPos[0]) / GetWindowWH(Framework->Core)[0];
		float VerticalMouseMovement		= float(MouseInput.dPos[1]) / GetWindowWH(Framework->Core)[1];

		Framework->MouseState.Normalized_dPos = { HorizontalMouseMovement, VerticalMouseMovement };
#if 0
		std::cout << "H: " << HorizontalMouseMovement << "\n";
		std::cout << "V: " << VerticalMouseMovement << "\n";
#endif

		for (size_t I = 0; I < this->Listeners.size(); ++I) {
			auto& L			= Listeners[I];
			auto& System	= this->TargetSystems[I];

			System->HandleEvent(L, InputComponentID, this, GetTypeGUID(KEYINPUT), nullptr);
		}
	}


	/************************************************************************************************/


	MouseInputState	InputComponentSystem::GetMouseState()
	{
		return Framework->MouseState;
	}


	/************************************************************************************************/

}