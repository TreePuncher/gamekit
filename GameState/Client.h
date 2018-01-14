#ifndef CLIENT_H
#define CLIENT_H

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


#include <WindowsIncludes.h>
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "Gameplay.h"

#include "..\Application\GameFramework.h"
#include "..\Application\GameMemory.h"

#include "Gameplay.h"

/*
TODO's
*/

struct PlayerImposters
{
	FlexKit::EntityHandle		Graphics;
	size_t						PlayerID;
	CapsuleCharacterController	Collider;

	void Move(GraphicScene* GS, float3 pos, double dT)
	{
		Collider.Controller->move({ pos.x, pos.y, pos.z }, 0.0001, dT, physx::PxControllerFilters());
	}
};


struct ClientState : public FrameworkState
{
	ClientState(GameFramework* framework, EngineCore* Engine, const char* Name = nullptr, const char* Server = nullptr) :
		FrameworkState(framework)
	{
		//State->VTable.Update       = JoinServer;
		//State->Framework           = Framework;
		//State->Peer                = RakNet::RakPeerInterface::GetInstance();
		//State->PlayerIds.Allocator = Engine->GetBlockMemory();

		char str[512];

		RakNet::SocketDescriptor desc;
		memset(str, 0, sizeof(str));

		if (!Name)
		{
			std::cout << "Enter Name\n";
			std::cin >> ClientName;
		}
		else
			strncpy(ClientName, Name, sizeof(ClientName));

		if (!Server)
		{
			std::cout << "Enter Server Address\n";
			std::cin >> str;
		}

		Peer->Startup(1, &desc, 1);

		//auto res = Peer->Connect(Server ? Server : str, gServerPort, nullptr, 0);
	}

	RakNet::RakPeerInterface*	Peer;
	RakNet::SystemAddress		ServerAddr;

	Vector<PlayerID_t>	PlayerIds;
	GUID_t					Scene;

	size_t ID;
	size_t PlayerCount;

	enum
	{
		JOINING,
		JOINED
	}CurrentState;

	char ClientName[256];
};

struct ClientPlayState : public FrameworkState
{
	ClientPlayState(GameFramework* framework, ClientState* Client) :
		FrameworkState(framework),
		NetState(Client),
		Scene(nullptr, framework->Core->Nodes, framework->Core->GetBlockMemory())
	{
		
		//PlayState->VTable.Update				  = UpdateClientGameplay;
		//PlayState->VTable.EventHandler			  = UpdateClientEventHandler;
		//PlayState->VTable.PreDrawUpdate			  = UpdateClientPreDraw;
		//PlayState->LocalPlayer.PlayerCTR.Pos      = float3(0, 0, 0);
		//PlayState->LocalPlayer.PlayerCTR.Velocity = float3(0, 0, 0);
		Imposters.Allocator = framework->Core->GetBlockMemory();
		Mode                = eWAITINGMODE; // Wait for all Players to Load and respond
		FrameCount          = 0;
		T2ServerUpdate      = 0.0;
		ServerUpdatePeriod  = 1.0;

		LocalInput.ClearState();
		Imposters.resize(Client->PlayerCount - 1);


		/*
		for (size_t I = 0; I < PlayState->Imposters.size(); ++I) {
		auto& Imposter	  = PlayState->Imposters[I];
		Imposter.Graphics = Framework->GScene.CreateDrawableAndSetMesh("PlayerModel");
		Imposter.PlayerID = Client->PlayerIds[I];

		CapsuleCharacterController_DESC Desc;
		Desc.FootPos = {0.0f, 0.0f, 0.0f};
		Desc.h = 20.0f;
		Desc.r = 5.0f;

		Initiate(&Imposter.Collider, &Framework->PScene, &Engine->Physics, Desc);
		}

		CreatePlaneCollider(Engine->Physics.DefaultMaterial, &Framework->PScene);
		InitiatePlayer(Framework, &PlayState->LocalPlayer);
		*/
	}

	ClientState*			NetState;
	ClientMode				Mode;
	PhysicsComponentSystem	Scene;

	CircularBuffer<InputFrame>	InputBuffer;// Buffers Last 64 Frames(1 Second of Input)

	//Player				LocalPlayer;

	PlayerInputState	LocalInput;
	float2				MouseInput;

	double				ServerUpdatePeriod;
	double				T2ServerUpdate;
	double				T;
	double				T_TillNextPositionUpdate; 
	size_t				FrameCount;

	FlexKit::Vector<PlayerImposters> Imposters;// Cause hes an imposter, trying to steal all the client info
};


#endif
