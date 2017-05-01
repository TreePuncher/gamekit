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
#include "InputComponent.h"

#include "..\coreutilities\Components.h"
struct PlayerController
{
	float3		Pos;
	float3		Velocity;
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

	size_t ID;

	float Health;

	operator Player* () { return this; }// I'm Getting tired of typeing the &'s everywhere!
};


/************************************************************************************************/


struct InputFrame
{
	PlayerInputState	KeyboardInput;
	float2				MouseInput;
	size_t				FrameID;
};

typedef Handle_t<16>	PlayerHandle;


/*
struct GameplayComponentSystem : public ComponentSystemInterface
{
	GameFramework*				Framework;
	PhysicsComponentSystem*		Scene;

	static_vector<Player>		Players;
	static_vector<InputFrame>	PlayerInputs;
	static_vector<size_t>		LastFrameRecieved;
	double						T;

	void ReleaseHandle(ComponentHandle Handle)
	{
		ReleasePlayer(&Players[Handle], Framework);
	}

	void HandleEvent(ComponentHandle Handle, ComponentType EventSource, EventTypeID ID)
	{
		if (EventSource == InputComponentID && ID == GetCRCGUID(LOCALINPUT)) {
		}
	}

	void Initiate( PhysicsComponentSystem* System, GameFramework* framework )
	{
		Framework		= framework;
		auto* Engine	= framework->Engine;

		CreatePlaneCollider(Engine->Physics.DefaultMaterial, &System->Scene);
	}

	void Clear()
	{
		Players.clear();
	}

	void SetPlayerCount(size_t Count)
	{
		PlayerInputs.resize(Count);
		Players.resize(Count);
		LastFrameRecieved.resize(Count);

		for (auto& Counter : LastFrameRecieved)
			Counter = 0;

		for (auto& P : Players)
			InitiatePlayer(Engine, &P);
	}

	PlayerHandle CreatePlayer()
	{
		PlayerInputs.push_back(InputFrame());
		Players.push_back(Player());
		LastFrameRecieved.push_back(0);

		InitiatePlayer(Framework, &Players.back());

		return PlayerHandle(Players.size() - 1);
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

	void Release()
	{

	}

	void UpdateAnimations(GameFramework* Engine, double dT)
	{
		for (auto& P : Players)
			UpdatePlayerAnimations(Engine, &P, dT);
	}

	NodeHandle GetPlayerNode(PlayerHandle)
	{
		return NodeHandle(-1);
	}

	void SetPlayerNode(PlayerHandle Player, NodeHandle Node)
	{
		Players[Player].CameraCTR.Yaw_Node = Node;
		Framework->Engine->Nodes.SetParentNode(Node, Players[Player].CameraCTR.Pitch_Node);
	}
};

const uint32_t PlayerComponentID = GetTypeGUID(PlayerComponent);

struct PlayersComponentArgs
{
	GameplayComponentSystem*	Gameplay;
	InputComponentSystem*		Input;
	GameFramework*				Framework;
};


template<size_t SIZE>
void CreateComponent(GameObject<SIZE>& GO, PlayersComponentArgs& Args)
{
	PlayerHandle	Player					= Args.Gameplay->CreatePlayer();
	ComponentHandle InputComponentHandle	= Args.Input->BindInput(Player, Args.Gameplay);

	auto C = FindComponent(GO, TransformComponentID);

	if (GO.ComponentCount + 2 < GO.MaxComponentCount)
	{

		if (!C)
		{
			auto Node = Args.Gameplay->GetPlayerNode(Player);
			CreateComponent(GO, TransformComponentArgs{ Args.Framework->Engine->Nodes, Node });
		}
		else
		{
			Args.Gameplay->SetPlayerNode(Player, GetNodeHandle(GO));
		}

		GO.AddComponent(Component{ Args.Gameplay, Player, PlayerComponentID });
		GO.AddComponent(Component{ Args.Input, InputComponentHandle, InputComponentID });
	}
}


PlayersComponentArgs CreateLocalPlayer(GameplayComponentSystem* GameplaySystem, InputComponentSystem* Input, GameFramework* Framework)	 { return { GameplaySystem, Input, Framework };}

*/

#endif