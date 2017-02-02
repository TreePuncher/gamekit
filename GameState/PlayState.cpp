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

//typedef void(*FN_EVENT_HANDLER) (SubState* StateMemory, Event);


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
			case KC_W: {
				ThisState->Forward = true;
				break;
			}
			case KC_S: {
				ThisState->Backward = true;
			}	break;
			case KC_A: {
				ThisState->Left = true;
			}	break;
			case KC_D: {
				ThisState->Right = true;
			}	break;
			default:
				break;
			}
		}	break;
		case Event::Release:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_W: {
				ThisState->Forward = false;
				break;
			}
			case KC_S: {
				ThisState->Backward = false;
			}	break;
			case KC_A: {
				ThisState->Left = false;
			}	break;
			case KC_D: {
				ThisState->Right = false;
			}	break;
			default:
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
	float HorizontalMouseMovement	= float(ThisState->Base->MouseState.dPos[0]) / GetWindowWH(Engine)[0];
	float VerticalMouseMovement		= float(ThisState->Base->MouseState.dPos[1]) / GetWindowWH(Engine)[1];

	float MovementFactor = 50;

	YawCamera(&ThisState->Cam_Ctr, 360 * HorizontalMouseMovement * dT * MovementFactor);
	PitchCamera(&ThisState->Cam_Ctr, 360 * VerticalMouseMovement * dT * MovementFactor);

	if (ThisState->Forward) {
		auto Forward = GetForwardVector(&ThisState->Cam_Ctr);
		TranslateCamera(&ThisState->Cam_Ctr, Forward * 1.0f / 60.0f * 20.0f);
	}
	else if (ThisState->Backward)
	{
		auto Backward = -GetForwardVector(&ThisState->Cam_Ctr);
		TranslateCamera(&ThisState->Cam_Ctr, Backward * 1.0f / 60.0f * 20.0f);
	}
	if (ThisState->Right) {
		auto Right = GetRightVector(&ThisState->Cam_Ctr);
		TranslateCamera(&ThisState->Cam_Ctr, Right * 1.0f / 60.0f * 20.0f);
	}
	else if (ThisState->Left) {
		auto Left = -GetRightVector(&ThisState->Cam_Ctr);
		TranslateCamera(&ThisState->Cam_Ctr, Left * 1.0f / 60.0f * 20.0f);
	}

	UpdateCameraController(&Engine->Nodes, &ThisState->Cam_Ctr, dT);

	return false;
}


/************************************************************************************************/


bool PreDrawUpdate(SubState* StateMemory, EngineMemory*, double DT)
{
	return false;
}


/************************************************************************************************/


PlayState* CreatePlayState(EngineMemory* Engine, BaseState* Base)
{
	PlayState* State = nullptr;

	State						 = &Engine->BlockAllocator.allocate_aligned<PlayState>();
	State->VTable.PreDrawUpdate  = PreDrawUpdate;
	State->VTable.Update         = PlayUpdate;
	State->VTable.EventHandler   = PlayEventHandler;
	State->VTable.PostDrawUpdate = nullptr;
	State->VTable.Release        = nullptr;
	State->Base				     = Base;

	InitiateCamera3rdPersonContoller(&Engine->Nodes, Base->ActiveCamera, &State->Cam_Ctr);
	YawCamera(&State->Cam_Ctr, 180);

	TranslateCamera(&State->Cam_Ctr, float3{ 0,  0, -25.921f });
	SetCameraOffset(&State->Cam_Ctr, float3{ 0, 10, 0 });

	AddResourceFile("assets\\ShaderBallTestScene.gameres", &Engine->Assets);
	FK_ASSERT(FlexKit::LoadScene(Engine->RenderSystem, Base->Nodes, &Engine->Assets, &Engine->Geometry, 201, &Base->GScene, Engine->TempAllocator), "FAILED TO LOAD!\n");

	return State;
}


/************************************************************************************************/