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

using FlexKit::GameFramework;

void InitiatePlayer(GraphicScene* Scene, GameFramework* Engine, Player* Out)
{
	//Out->PlayerCTR.Orientation = Quaternion(0, 0, 0, 1);
	//Out->PlayerCTR.Pos      = float3(0, 0, 0);
	//Out->PlayerCTR.Velocity = float3(0, 0, 0);
	//Out->Model              = Scene->CreateDrawableAndSetMesh("PlayerModel");

	//Scene->EntityEnablePosing(Out->Model);

	CapsuleCharacterController_DESC Desc;
	Desc.FootPos = float3(0, 10, 0);
	Desc.h       = 10.0f;
	Desc.r       = 5.0f;

	//InitiateCamera3rdPersonContoller(Engine->Engine->Nodes, Engine->ActiveCamera, &Out->CameraCTR);

	/*
	AnimationStateEntry_Desc WalkDesc;
	auto Res1 = FindResourceGUID(&Engine->Engine->Assets, "WALK_1");
	auto Res2 = FindResourceGUID(&Engine->Engine->Assets, "ANIMATION2");

	FK_ASSERT(Res1, "FAILED TO FIND WALK01 Animation!");
	FK_ASSERT(Res2, "FAILED TO FIND ANIMATION2 Animation!");

	WalkDesc.Animation       = (GUID_t)Res1;
	WalkDesc.ForceComplete   = true;
	WalkDesc.EaseOut         = EaseOut_RAMP;
	WalkDesc.EaseIn			 = EaseIn_RAMP;
	WalkDesc.Out             = WeightFunction::EWF_Ramp;
	WalkDesc.EaseOutDuration = 0.2f;
	WalkDesc.Loop			 = true;
	WalkDesc.EaseInDuration  = 0.2f;

	auto WalkState = DASAddState(WalkDesc, &Out->PlayerAnimation);

	WalkDesc.EaseInDuration  = 0.2f;
	WalkDesc.EaseOutDuration = 0.2f;
	WalkDesc.Animation		 = (GUID_t)Res2;
	WalkDesc.Loop			 = true;
	auto OtherState			 = DASAddState(WalkDesc, &Out->PlayerAnimation);

	AnimationCondition_Desc Walk_Cd;
	Walk_Cd.DrivenState = WalkState;
	Walk_Cd.InputType   = ASCondition_InputType::ASC_BOOL;
	Walk_Cd.Operation   = ASCondition_OP::EASO_TRUE;

	auto WalkCondition = DASAddCondition(Walk_Cd, &Out->PlayerAnimation);

	Walk_Cd.DrivenState = OtherState;
	auto DummyCondition = DASAddCondition(Walk_Cd, &Out->PlayerAnimation);

	Out->WalkCondition = WalkCondition;
	Out->OtherCondition = DummyCondition;

	TranslateCamera(&Out->CameraCTR, float3{ 0,  0, 0});
	SetCameraOffset(&Out->CameraCTR, float3{ 0, 20, 40.0f });

	*/
}


void UpdatePlayer(GameFramework* Framework, Player* P, PlayerInputState Input, float2 MouseMovement, double dT)
{
	float MovementFactor = 50;
	float Drag =  Input.Shield ? 6.0f : 5.0f;

	//auto Cam_Ctr = &P->CameraCTR;

	Quaternion Q(0.0f, 360 * MouseMovement.x * dT * MovementFactor, 0.0f);
	
	//YawCamera	(Cam_Ctr, 360 * MouseMovement.x * dT * MovementFactor);
	//YawPlayer(P, 360 * MouseMovement.x * dT * MovementFactor);
	//PitchCamera	(Cam_Ctr, 360 * MouseMovement.y * dT * MovementFactor);

	//SetOrientation(Engine->Nodes, Cam_Ctr->Yaw_Node, P->PlayerCTR.Orientation);

	//if (P->PlayerCTR.Velocity.magnitudesquared() > 0.001f)
	{
		if (!Input.Shield) {
			//float3 Forward = P->PlayerCTR.Velocity.normal();
			//Forward = Quaternion(0, -90, 0) * Forward;
			//auto Q = FlexKit::Vector2Quaternion(Forward, { 0, 1, 0 }, Forward.cross({ 0, 1, 0 }));
			//Framework->GScene.SetOrientation(P->Model, Q);
		}
		else {
			//Framework->GScene.SetOrientation( P->Model, Quaternion(0, P->CameraCTR.Yaw + 180, 0));
		}
	}
	//else
	{
		//P->PlayerCTR.Velocity = { 0, 0, 0 };
	}


	float ForwardBackwardAccel = Input.Shield ? 5.0f : 10.0f;
	float StrafingAccel = 3.0f;


	if (Input.Shield) {
		if (Input.Forward) {
			//auto Forward = GetForwardVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Forward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		else if (Input.Backward)
		{
			//auto Backward = -GetForwardVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Backward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		if (Input.Right) {
			//auto Right = GetRightVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Right * 1.0f / 60.0f * 20.0f * StrafingAccel;
		}
		else if (Input.Left) {
			//auto Left = -GetRightVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Left * 1.0f / 60.0f * 20.0f * StrafingAccel;
		}
	}
	else
	{
		if (Input.Forward) {
			//auto Forward = GetForwardVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Forward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		else if (Input.Backward)
		{
			//auto Backward = -GetForwardVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Backward * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		if (Input.Right) {
			//auto Right = GetRightVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Right * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
		else if (Input.Left) {
			//auto Left = -GetRightVector(Cam_Ctr);
			//P->PlayerCTR.Velocity += Left * 1.0f / 60.0f * 20.0f * ForwardBackwardAccel;
		}
	}

	/*

	TranslateActor(P, P->PlayerCTR.Velocity * dT + float3(0, -1, 0), dT);
	//auto FinalPOS = P->PlayerCollider.Controller->getFootPosition();

	//float Offset = P->PlayerCollider.Controller->getStepOffset();

	//P->PlayerCTR.Pos = {FinalPOS.x, FinalPOS.y - Offset, FinalPOS.z};// P->PlayerCTR.Velocity * dT;
	//P->PlayerCTR.Velocity -= P->PlayerCTR.Velocity * Drag * dT;
	
	if (P->PlayerCTR.Velocity.magnitudesquared() > 5.0f) {
		ASSetBool(P->WalkCondition, true, &P->PlayerAnimation);
		ASSetBool(P->OtherCondition, true, &P->PlayerAnimation);
	}
	else {
		ASSetBool(P->WalkCondition, false, &P->PlayerAnimation);
		ASSetBool(P->OtherCondition, false, &P->PlayerAnimation);
	}

	SetPositionW(P->CameraCTR.Nodes, P->CameraCTR.Yaw_Node, (P->PlayerCTR.Pos));
	Framework->GScene.SetPositionEntity_WT(P->Model, P->PlayerCTR.Pos);

	UpdateCameraController(Framework->Core->Nodes, Cam_Ctr, dT);

	*/
}




//void UpdatePlayerAnimations(GameFramework* Engine, Player* P, double dT)
//{
	//UpdateASM(dT, &P->PlayerAnimation, Engine->Engine->TempAllocator, Engine->Engine->GetBlockMemory(), Engine->GScene);

	//if (Engine->GScene.GetDrawable(P->Model).PoseState)
	//	DEBUG_DrawPoseState(Engine->GScene.GetDrawable(P->Model).PoseState, Engine->Nodes, Engine->GScene.GetNode(P->Model), &Engine->DebugLines);
//}




//void ReleasePlayer(Player* P, GameFramework* Engine)
//{
	//Engine->GScene.RemoveEntity(P->Model);
	//P->PlayerCollider.Controller->release();
//}

