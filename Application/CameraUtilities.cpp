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
#include "CameraUtilities.h"



void InitiateCamera3rdPersonContoller(SceneNodes* Nodes, Camera* C, Camera3rdPersonContoller* Out)
{
	ReleaseNode(Nodes, C->Node);

	auto Yaw	= GetZeroedNode(Nodes);
	auto Pitch	= GetZeroedNode(Nodes);
	auto Roll	= GetZeroedNode(Nodes);

	SetParentNode(Nodes, Pitch, Roll);
	SetParentNode(Nodes, Yaw, Pitch);


	C->Node		= Roll;
	Out->C		= C;
	Out->Nodes	= Nodes;

	Out->Pitch	= 0;
	Out->Roll	= 0;
	Out->Yaw	= 0;

	Out->Pitch_Node = Pitch;
	Out->Roll_Node  = Roll;
	Out->Yaw_Node   = Yaw;
}

void UpdateCameraController(SceneNodes* Nodes, Camera3rdPersonContoller* Controller, double dT)
{
}

void SetCameraOffset(Camera3rdPersonContoller* Controller, float3 xyz)
{
	TranslateWorld(Controller->Nodes, Controller->Pitch_Node, float3(0.0f, 1.0f, 0.0f) * xyz);
	TranslateWorld(Controller->Nodes, Controller->Roll_Node,  float3(0.0f, 0.0f, 1.0f) * xyz);

}

void SetCameraPosition(Camera3rdPersonContoller* Controller, float3 xyz)
{
	SetPositionW(Controller->Nodes, Controller->Yaw_Node, xyz);
}

void TranslateCamera(Camera3rdPersonContoller* Controller, float3 xyz)
{
	TranslateWorld(Controller->Nodes, Controller->Yaw_Node, xyz);
}

void YawCamera(Camera3rdPersonContoller* Controller, float Degree)
{
	Controller->Yaw += Degree;
}

void PitchCamera(Camera3rdPersonContoller* Controller, float Degree)
{
	Controller->Pitch += Degree;
}

void RollCamera(Camera3rdPersonContoller* Controller, float Degree)
{
	Controller->Roll += Degree;
}

float3 GetForwardVector(Camera3rdPersonContoller* Controller)
{
	Quaternion Q = GetOrientation(Controller->Nodes, Controller->Yaw_Node);
	return -(Q * float3(0, 0, 1)).normal();
}

float3 GetRightVector(Camera3rdPersonContoller* Controller)
{
	Quaternion Q = GetOrientation(Controller->Nodes, Controller->Yaw_Node);
	return -(Q * float3(-1, 0, 0)).normal();
}

namespace FlexKit
{
	void OrbitCameraSystem::Initiate(GameFramework* Framework, InputComponentSystem* Input)
	{
		Controllers.Allocator = Framework->Engine->GetBlockMemory();

		Nodes			= Framework->Engine->Nodes;
		InputSystem		= Input;
	}

	void OrbitCameraSystem::ReleaseHandle(ComponentHandle Handle)
	{

	}

	void OrbitCameraSystem::HandleEvent(ComponentHandle Handle, ComponentType EventSource, EventTypeID)
	{

	}

	void OrbitCameraSystem::Update(double dT)
	{
		for (auto Controller : this->Controllers) {
			Quaternion Q = ::GetOrientation(*Nodes, Controller.CameraNode);

			float3 dP = 0;

			auto MouseState = InputSystem->GetMouseState();

			dP += InputSystem->KeyState.Forward		? float3(0, 0, -1)	: float3(0, 0, 0);
			dP += InputSystem->KeyState.Backward	? float3(0, 0, 1)	: float3(0, 0, 0);
			dP += InputSystem->KeyState.Left		? float3(-1, 0, 0)	: float3(0, 0, 0);
			dP += InputSystem->KeyState.Right		? float3(1, 0, 0)	: float3(0, 0, 0);

			dP = Q * dP;

			Translate	(Controller.ParentGO, dP * dT * Controller.MoveRate);
			Pitch		(*Nodes, Controller.PitchNode, MouseState.dPos[1] * dT);
			Yaw			(*Nodes, Controller.YawNode, MouseState.dPos[0] * dT);
		}

	}

	void SetParentNode(GameObjectInterface* GO, NodeHandle Node)
	{
		auto C = FindComponent(GO, OrbitCameraComponentID);
		if (C)
		{
			auto System = (OrbitCameraSystem*)C->ComponentSystem;
			auto YawNode = System->GetNode(C->ComponentHandle);

			System->Nodes->SetParentNode(YawNode, Node);
		}
	}


	OrbitCameraArgs CreateOrbitCamera(OrbitCameraSystem* System, Camera* Cam, float MoveRate)
	{
		CameraOrbitController Controller;
		Controller.CameraNode	= Cam->Node;
		Controller.PitchNode	= System->Nodes->GetNewNode();
		Controller.RollNode		= System->Nodes->GetNewNode();
		Controller.YawNode		= System->Nodes->GetNewNode();
		Controller.MoveRate		= MoveRate;

		System->Nodes->SetParentNode(Controller.YawNode,	Controller.PitchNode);
		System->Nodes->SetParentNode(Controller.PitchNode,	Controller.RollNode);
		System->Nodes->SetParentNode(Controller.RollNode,	Controller.CameraNode);

		OrbitCameraArgs Out = {};
		Out.System = System;
		Out.Handle = CameraControllerHandle(System->Controllers.size());

		System->Controllers.push_back(Controller);

		return Out;
	}


	ThirdPersonCameraArgs CreateThirdPersonCamera(ThirdPersonCameraComponentSystem* System, Camera* C)
	{
		ThirdPersonCameraArgs Args;
		Args.C		= C;
		Args.System = System;

		return Args;
	}
}
