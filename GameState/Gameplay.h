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

#include "..\Application\CameraUtilities.h"
#include "..\Application\GameFramework.h"
#include "..\Application\InputComponent.h"

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
	ComponentListInterface* ComponentList;

	DAConditionHandle WalkCondition;
	DAConditionHandle OtherCondition;

	size_t ID;

	float Health;

	operator Player* () { return this; }// I'm Getting tired of typeing the &'s everywhere!
};


typedef size_t			PlayerID_t;
typedef Handle_t<16>	PlayerHandle;


/************************************************************************************************/


enum ClientMode : unsigned char
{
	eLOADINGMODE,
	eLOBBYMODE,
	ePLAYMODE,
	eWAITINGMODE,
};

enum ServerMode : unsigned char
{
	eSERVERLOBBYMODE,	// Waits for Min Number of Players
	eCLIENTLOADWAIT,	// Waits for all Players to Load Scene
	eGAMEINPROGRESS,
};


/************************************************************************************************/


struct InputFrame
{
	PlayerInputState	KeyboardInput;
	float2				MouseInput;
	size_t				FrameID;
};



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
	}


	void HandleEvent(ComponentHandle Handle, ComponentType EventSource, EventTypeID ID)
	{
		if (EventSource == InputComponentID && ID == GetCRCGUID(LOCALINPUT)) {
		}
	}


	void ObjectMoved(ComponentHandle Handle, ComponentSystemInterface* System, ComponentListInterface* GO)
	{
	}


	void Initiate( PhysicsComponentSystem* System, GameFramework* framework )
	{
		Framework	= framework;
		auto* Core	= framework->Core;
	}

	void Clear()
	{
		Players.clear();
	}

	void Update(GameFramework* Engine, double dT)
	{
		const double FrameStep = 1.0 / 30.0;
		if (T > FrameStep) {
			for (size_t i = 0; i < Players.size(); ++i)
			{
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
void CreateComponent(ComponentList<SIZE>& GO, PlayersComponentArgs& Args)
{
	PlayerHandle	Player					= Args.Gameplay->CreatePlayer();
	ComponentHandle InputComponentHandle	= Args.Input->BindInput(Player, Args.Gameplay);

	auto C = FindComponent(GO, TransformComponentID);

	if (GO.ComponentCount + 2 < GO.MaxComponentCount)
	{

		if (!C)
		{
			auto Node = Args.Gameplay->GetPlayerNode(Player);
			CreateComponent(GO, TransformComponentArgs{ Args.Framework->Core->Nodes, Node });
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

#endif