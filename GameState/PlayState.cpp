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

#include "PlayState.h"


/************************************************************************************************/


bool PlayEventHandler(SubState* StateMemory, Event evt)
{
	PlayState* ThisState = (PlayState*)StateMemory;

	if (evt.InputSource == Event::Keyboard)
	{
		switch (evt.Action)
		{
		case Event::Pressed:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				ThisState->Input.Shield   = true;
				break;
			case KC_W:
				ThisState->Input.Forward  = true;
				break;
			case KC_S:
				ThisState->Input.Backward = true;
				break;
			case KC_A:
				ThisState->Input.Left     = true;
				break;
			case KC_D:
				ThisState->Input.Right    = true;
				break;
			}
		}	break;
		case Event::Release:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				ThisState->Input.Shield   = false;
				break;
			case KC_W:
				ThisState->Input.Forward  = false;
				break;
			case KC_S:
				ThisState->Input.Backward = false;
				break;
			case KC_A:
				ThisState->Input.Left     = false;
				break;
			case KC_D: 
				ThisState->Input.Right    = false;
				break;
			}
		}	break;
		}
	}
	return true;
}


/************************************************************************************************/


bool PlayUpdate(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (PlayState*)StateMemory;
	float HorizontalMouseMovement	= float(ThisState->Framework->MouseState.dPos[0]) / GetWindowWH(Engine)[0];
	float VerticalMouseMovement		= float(ThisState->Framework->MouseState.dPos[1]) / GetWindowWH(Engine)[1];
	float MovementFactor			= 50.0f;

	ThisState->Model.PlayerInputs[0].FrameID++;
	ThisState->Model.PlayerInputs[0].MouseInput		= { HorizontalMouseMovement, VerticalMouseMovement };
	ThisState->Model.PlayerInputs[0].KeyboardInput	= ThisState->Input;
	ThisState->Model.Update(StateMemory->Framework, dT);

	Yaw(ThisState->TestObject, dT * pi);

	return false;
}


/************************************************************************************************/


bool PreDrawUpdate(SubState* StateMemory, EngineMemory* Engine, double DT)
{
	auto ThisState = (PlayState*)StateMemory;

	if(ThisState->Framework->DrawPhysicsDebug)
	{
		auto PlayerPOS = ThisState->Model.Players[0].PlayerCTR.Pos;
		PushCapsule_Wireframe(&StateMemory->Framework->Immediate, Engine->TempAllocator, { PlayerPOS.x, PlayerPOS.y, PlayerPOS.z }, 5, 10, GREEN);
	}

	ThisState->Model.UpdateAnimations(ThisState->Framework, DT);

	return false;
}


/************************************************************************************************/


void ReleasePlayState(SubState* StateMemory)
{
	auto ThisState = (PlayState*)StateMemory;
	ThisState->Model.Release();
}


/************************************************************************************************/


PlayState* CreatePlayState(EngineMemory* Engine, GameFramework* Framework)
{
	PlayState* State = nullptr;

	State						 = &Engine->BlockAllocator.allocate_aligned<PlayState>();
	State->VTable.PreDrawUpdate  = PreDrawUpdate;
	State->VTable.Update         = PlayUpdate;
	State->VTable.EventHandler   = PlayEventHandler;
	State->VTable.PostDrawUpdate = nullptr;
	State->VTable.Release        = ReleasePlayState;
	State->Framework			 = Framework;

	CreatePlaneCollider(Engine->Physics.DefaultMaterial, &Framework->PScene);

	State->Model.Initiate(Framework);
	State->Model.SetPlayerCount(Framework, 1);

	FK_ASSERT(LoadScene(Engine->RenderSystem, Framework->Nodes, &Engine->Assets, &Engine->Geometry, 201, &Framework->GScene, Engine->TempAllocator), "FAILED TO LOAD!\n");

	EntityHandle Entity = Framework->GScene.CreateDrawableAndSetMesh("Flower");

	InitiateGameObject(
		State->TestObject,
		&Engine->Nodes,
		EntityComponentArgs{ Entity , &Framework->GScene });

	return State;
}


/************************************************************************************************/