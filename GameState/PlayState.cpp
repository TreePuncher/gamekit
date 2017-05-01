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
#include "..\coreutilities\GraphicsComponents.h"

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
				ThisState->Input.KeyState.Shield   = true;
				break;
			case KC_W:
				ThisState->Input.KeyState.Forward  = true;
				break;
			case KC_S:
				ThisState->Input.KeyState.Backward = true;
				break;
			case KC_A:
				ThisState->Input.KeyState.Left     = true;
				break;
			case KC_D:
				ThisState->Input.KeyState.Right    = true;
				break;
			}
		}	break;
		case Event::Release:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				ThisState->Input.KeyState.Shield   = false;
				break;
			case KC_W:
				ThisState->Input.KeyState.Forward  = false;
				break;
			case KC_S:
				ThisState->Input.KeyState.Backward = false;
				break;
			case KC_A:
				ThisState->Input.KeyState.Left     = false;
				break;
			case KC_D: 
				ThisState->Input.KeyState.Right    = false;
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

	float MovementFactor			= 50.0f;

	//ThisState->Model.PlayerInputs[0].FrameID++;
	//ThisState->Model.PlayerInputs[0].MouseInput		= { HorizontalMouseMovement, VerticalMouseMovement };
	//ThisState->Model.PlayerInputs[0].KeyboardInput	= ThisState->Input;

	ThisState->Input.Update(dT, ThisState->Framework->MouseState, StateMemory->Framework );

	double T = ThisState->Framework->TimeRunning;
	double CosT = (float)cos(T);
	double SinT = (float)sin(T);

	float Begin	= 0.0f;
	float End	= 60.0f;
	float IaR	= 10000 * (1 + (float)cos(T * 6)) / 2;

	Yaw(ThisState->TestObject, dT * pi);
	SetDrawableColor(ThisState->TestObject, Grey(CosT));
	SetWorldPosition(ThisState->TestObject, float3( 100.0f * CosT, 60.0f, SinT * 100));

	SetLightRadius		(ThisState->TestObject, 100 + IaR);
	SetLightIntensity	(ThisState->TestObject, 100 + IaR);
	
	ThisState->OrbitCameras.Update(dT);

	return false;
}


/************************************************************************************************/


bool PreDrawUpdate(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (PlayState*)StateMemory;
	ThisState->Physics.UpdateSystem(dT);

	if(ThisState->Framework->DrawPhysicsDebug)
	{
		//auto PlayerPOS = ThisState->Model.Players[0].PlayerCTR.Pos;
		//PushCapsule_Wireframe(&StateMemory->Framework->Immediate, Engine->TempAllocator, { PlayerPOS.x, PlayerPOS.y, PlayerPOS.z }, 5, 10, GREEN);
	}

	//ThisState->Model.UpdateAnimations(ThisState->Framework, DT);

	return false;
}


/************************************************************************************************/


void ReleasePlayState(SubState* StateMemory)
{
	auto ThisState = (PlayState*)StateMemory;
	ThisState->Player.Release();
	ThisState->TestObject.Release();
	ThisState->Physics.Release();
	ThisState->GScene.ClearScene();
	//ThisState->Model.Release();
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

	InitiateGraphicScene(&State->GScene, Engine->RenderSystem, &Engine->Assets, Engine->Nodes, Engine->Geometry, Engine->BlockAllocator, Engine->TempAllocator);
	State->Drawables.InitiateSystem(&State->GScene, Engine->Nodes);
	State->Lights.InitiateSystem(&State->GScene, Engine->Nodes);
	State->Physics.InitiateSystem(&Engine->Physics, Engine->Nodes, Engine->BlockAllocator);

	State->Input.Initiate(Framework);
	State->OrbitCameras.Initiate(Framework, State->Input);

	Framework->ActivePhysicsScene	= &State->Physics;
	Framework->ActiveScene			= &State->GScene;

	FK_ASSERT(LoadScene(Engine->RenderSystem, Engine->Nodes, &Engine->Assets, &Engine->Geometry, 201, &State->GScene, Engine->TempAllocator), "FAILED TO LOAD!\n");

	GameObject<> Test;

	InitiateGameObject(
		State->TestObject,
			CreateEnityComponent(&State->Drawables, "Flower"),
			CreateLightComponent(&State->Lights));

	SetLightColor(State->TestObject, RED);

	InitiateGameObject( 
		State->FloorObject,
			State->Physics.CreateStaticBoxCollider({10000, 1, 10000}, {0, -0.5, 0}));

	for(size_t I = 0; I < 64; ++I)
	InitiateGameObject( 
		State->CubeObjects[I],
			CreateEnityComponent(&State->Drawables, "Flower"),

			State->Physics.CreateCubeComponent(
				{ 0, 20.0f + 10.0f * I, 0}, 
				{ 0, 10, 0}, 1));

	InitiateGameObject(
		State->Player,
			CreateOrbitCamera(State->OrbitCameras, Framework->ActiveCamera));

	Translate(State->Player, {0, 10, -10});
	Yaw(State->Player, pi);

	return State;
}


/************************************************************************************************/