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

#include "Gameplay.h"

void InitiatePlayer(BaseState* Engine, Player* Out)
{
	//Out->PlayerCTR.Orientation = Quaternion(0, 0, 0, 1);
	Out->PlayerCTR.Pos      = float3(0, 0, 0);
	Out->PlayerCTR.Velocity = float3(0, 0, 0);
	Out->Model              = Engine->GScene.CreateDrawableAndSetMesh("PlayerModel");

	Engine->GScene.EntityEnablePosing(Out->Model);

	CapsuleCharacterController_DESC Desc;
	Desc.FootPos = float3(0, 10, 0);
	Desc.h = 20.0f;
	Desc.r = 5.0f;

	InitiateCamera3rdPersonContoller(Engine->Nodes, Engine->ActiveCamera, &Out->CameraCTR);
	Initiate(&Out->PlayerCollider, &Engine->PScene, &Engine->Engine->Physics, Desc);
	InitiateASM(&Out->PlayerAnimation, Engine->Engine->BlockAllocator, Out->Model);
	TranslateCamera(&Out->CameraCTR, float3{ 0,  0, 0});
	SetCameraOffset(&Out->CameraCTR, float3{ 0, 20, 40.0f });
}

void SetPlayerPosition(Player* P, float3 Position)
{
	P->PlayerCTR.Pos = Position;
}


void YawPlayer(Player* P, float Degree)
{
	//Quaternion Q(0.0f, FlexKit::DegreetoRad(Degree), 0.0f);
	//Quaternion NewR = P->PlayerCTR.Orientation * Q;
	//P->PlayerCTR.Orientation = NewR;

	YawCamera(&P->CameraCTR, Degree );
}


void SetPlayerOrientation(Player* P, Quaternion Q)
{
	SetOrientation(P->CameraCTR.Nodes, P->CameraCTR.Yaw_Node, Q);
	//P->PlayerCTR.Orientation = Q;
}

void TranslateActor(Player* P, float3 pos, double dT)
{
	P->PlayerCollider.Controller->move({ pos.x, pos.y, pos.z }, 0.0001, dT, PxControllerFilters());
	//Actor->BPC.FloorContact = flags & PxControllerCollisionFlag::eCOLLISION_DOWN;
	//Actor->BPC.CeilingContact = flags & PxControllerCollisionFlag::eCOLLISION_UP;
}


void UpdatePlayer(BaseState* Engine, Player* P, PlayerInputState Input, float2 MouseMovement, double dT)
{
	float MovementFactor = 50;
	float Drag =  Input.Shield ? 6.0f : 5.0f;

	auto Cam_Ctr = &P->CameraCTR;

	Quaternion Q(0.0f, 360 * MouseMovement.x * dT * MovementFactor, 0.0f);
	
	//YawCamera	(Cam_Ctr, 360 * MouseMovement.x * dT * MovementFactor);
	YawPlayer(P, 360 * MouseMovement.x * dT * MovementFactor);
	PitchCamera	(Cam_Ctr, 360 * MouseMovement.y * dT * MovementFactor);

	//SetOrientation(Engine->Nodes, Cam_Ctr->Yaw_Node, P->PlayerCTR.Orientation);

	if (P->PlayerCTR.Velocity.magnitudesquared() > 0.001f)
	{
		if (!Input.Shield) {
			float3 Forward = P->PlayerCTR.Velocity.normal();
			Forward = Quaternion(0, -90, 0) * Forward;
			auto Q = FlexKit::Vector2Quaternion(Forward, { 0, 1, 0 }, Forward.cross({ 0, 1, 0 }));
			Engine->GScene.SetOrientation(P->Model, Q);
		}
		else {
			Engine->GScene.SetOrientation( P->Model, Quaternion(0, P->CameraCTR.Yaw + 180, 0));
		}
	}
	else
	{
		P->PlayerCTR.Velocity = { 0, 0, 0 };
	}

	float ForwardBackwardAccel = Input.Shield ? 5.0f : 10.0f;
	float StrafingAccel = 3.0f;


	if (Input.Shield) {
		if (Input.Forward) {
			auto Forward = GetForwardVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Forward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		else if (Input.Backward)
		{
			auto Backward = -GetForwardVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Backward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		if (Input.Right) {
			auto Right = GetRightVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Right * 1.0f / 60.0f * 20.0f * StrafingAccel;
		}
		else if (Input.Left) {
			auto Left = -GetRightVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Left * 1.0f / 60.0f * 20.0f * StrafingAccel;
		}
	}
	else
	{
		if (Input.Forward) {
			auto Forward = GetForwardVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Forward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		else if (Input.Backward)
		{
			auto Backward = -GetForwardVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Backward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		if (Input.Right) {
			auto Right = GetRightVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Right * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		else if (Input.Left) {
			auto Left = -GetRightVector(Cam_Ctr);
			P->PlayerCTR.Velocity += Left * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
	}


	TranslateActor(P, P->PlayerCTR.Velocity * dT + float3(0, -1, 0), dT);
	auto FinalPOS = P->PlayerCollider.Controller->getFootPosition();

	float Offset = P->PlayerCollider.Controller->getStepOffset();

	P->PlayerCTR.Pos = {FinalPOS.x, FinalPOS.y - Offset, FinalPOS.z};// P->PlayerCTR.Velocity * dT;
	P->PlayerCTR.Velocity -= P->PlayerCTR.Velocity * Drag * dT;
	
	SetPositionW(P->CameraCTR.Nodes, P->CameraCTR.Yaw_Node, (P->PlayerCTR.Pos));
	Engine->GScene.SetPositionEntity_WT(P->Model, P->PlayerCTR.Pos);

	UpdateCameraController(Engine->Nodes, Cam_Ctr, dT);
}

void UpdatePlayerAnimations(BaseState* Engine, Player* P, double dT)
{
	UpdateASM(dT, &P->PlayerAnimation, Engine->Engine->TempAllocator, Engine->Engine->BlockAllocator, Engine->GScene);
}



/************************************************************************************************/


void ReleasePlayer(Player* P, BaseState* Engine)
{
	Engine->GScene.RemoveEntity(P->Model);
	//P->PlayerCollider.Controller->release();
}


/************************************************************************************************/
