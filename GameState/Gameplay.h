#ifndef GAMEPLAY_H
#define GAMEPLAY_H

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
#include "CameraUtilities.h"


struct PlayerController
{
	float3		Pos;
	float3		Velocity;
};

struct PlayerInputState
{
	bool Forward;
	bool Backward;
	bool Left;
	bool Right;
	bool Shield;

	void ClearState()
	{
		Forward = false;
		Backward = false;
		Left = false;
		Right = false;
	}
};

struct PlayerStateFrame
{
	size_t FrameID;
	float3 Velocity;
	float3 Position;
	float Yaw, Pitch, Roll;
};

struct Player
{
	EntityHandle				Model;
	PlayerController			PlayerCTR;
	Camera3rdPersonContoller	CameraCTR;
	AnimationStateMachine		PlayerAnimation;
	CapsuleCharacterController	PlayerCollider;

	DAConditionHandle WalkCondition;
	DAConditionHandle OtherCondition;

	size_t					ID;

	operator Player* () { return this; }// I'm Getting tired of typeing the &'s everywhere!
};


void InitiatePlayer			( GameFramework* Engine, Player* Out );
void ReleasePlayer			( Player* P, GameFramework* Engine );
void UpdatePlayer			( GameFramework* Engine, Player* P, PlayerInputState Input, float2 MouseMovement, double dT );
void UpdatePlayerAnimations	( GameFramework* Engine, Player* P, double dT );

void SetPlayerPosition	  ( Player* P, float3 Position );
void YawPlayer			  ( Player* P, float Degree );
void SetPlayerOrientation ( Player* P, Quaternion Q );

inline float GetPlayerYaw(Player* P)
{
	return P->CameraCTR.Yaw;
}

inline Quaternion GetOrientation(Player* P, GraphicScene* GS) 
{
	return GS->GetOrientation(P->Model);
}

inline float3 GetPlayerPosition(Player* P)
{
	return P->PlayerCTR.Pos;
}

struct InputFrame
{
	PlayerInputState	KeyboardInput;
	float2				MouseInput;
	size_t				FrameID;
};

struct Gameplay_Model
{
	static_vector<Player>		Players;
	static_vector<InputFrame>	PlayerInputs;
	static_vector<size_t>		LastFrameRecieved;
	double						T;

	void Initiate(GameFramework* Base)
	{
		auto* Engine = Base->Engine;
		CreatePlaneCollider(Engine->Physics.DefaultMaterial, &Base->PScene);
	}

	void Clear()
	{
		Players.clear();
	}

	void SetPlayerCount(GameFramework* Engine, size_t Count)
	{
		PlayerInputs.resize(Count);
		Players.resize(Count);
		LastFrameRecieved.resize(Count);

		for (auto& Counter : LastFrameRecieved)
			Counter = 0;

		for (auto& P : Players)
			InitiatePlayer(Engine, &P);
	}

	void Update(GameFramework* Engine, double dT)
	{
		const double FrameStep = 1.0 / 30.0;
		if (T > FrameStep) {
			for (size_t i = 0; i < Players.size(); ++i)
			{
				UpdatePlayer(Engine, &Players[i], 
					PlayerInputs[i].KeyboardInput, 
					PlayerInputs[i].MouseInput, 
					FrameStep);
#if 0
				printf("Player at Position: ");
				printfloat3(Players[i].PlayerCTR.Pos);
				printf("  Orientation:");
				printQuaternion(GetOrientation(&Players[i], Engine->ActiveScene));
				printf("\n");
#endif
			}
			T -= FrameStep;
		}
		T += dT;
	}


	void UpdateAnimations(GameFramework* Engine, double dT)
	{
		for (auto& P : Players)
			UpdatePlayerAnimations(Engine, &P, dT);
	}

};

#endif