/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "MultiplayerMode.h"

#if USING(RACKNET)


#include "CommonStructs.h"
#include "GameUtilities.h"

#pragma warning( disable : 4267 )

using namespace RakNet;
using namespace FlexKit;

/************************************************************************************************/

enum MultiplayerCommandCodes
{
	LOBBY_TEXT			= ID_USER_PACKET_ENUM + 1,
	LOBBY_GETNAME		= ID_USER_PACKET_ENUM + 2,
	LOBBY_SETNAME		= ID_USER_PACKET_ENUM + 3,
	LOBBY_SETREADY		= ID_USER_PACKET_ENUM + 4,
	LOBBY_GAMEBEGIN		= ID_USER_PACKET_ENUM + 5,
	GAME_GETPLAYERNAME	= ID_USER_PACKET_ENUM + 18,
	GAME_PLAYERJOINED	= ID_USER_PACKET_ENUM + 19,
	GAME_BEGIN			= ID_USER_PACKET_ENUM + 20,
	GAME_KEYSTATE		= ID_USER_PACKET_ENUM + 21,
	GAME_STATECHECK		= ID_USER_PACKET_ENUM + 22,
	GAME_STATECHECKRES	= ID_USER_PACKET_ENUM + 23,
	GAME_STATESET		= ID_USER_PACKET_ENUM + 24,
	GAME_MESSAGE		= ID_USER_PACKET_ENUM + 25,
	GAME_END			= ID_USER_PACKET_ENUM + 26
};

/************************************************************************************************/

struct GetNameMsg
{
	MultiplayerCommandCodes ID;
	char					length;
	char					Name[32];
};

/************************************************************************************************/

struct PlayerStateCheck
{
	float3		xyz;
	float3		dv;
	Quaternion	Q;
};

/************************************************************************************************/

void AddPlayer(EngineMemory* Game, PlayerList& Players, RakNet::SystemAddress addr)
{
	Player NewPlayer;
	NewPlayer.dV = float3(0.0, 0.0, 0.0);
	InitialisePlayer( &NewPlayer, Game->Nodes );

	Players.push_back({ NewPlayer, Players.size(), addr, "", NOT_READY });
	int c = 0; // For Debug Watches
}

/************************************************************************************************/

bool FindPlayerByAddress( RakNet::SystemAddress addr, PlayerList& in, PlayerInstance res)
{
	for (auto P : in)
	{
		if (P.Address == addr)
		{
			res = P;
			return true;
		}
	}
	return false;
}

/************************************************************************************************/

bool FindPlayerHandleByAddress( RakNet::SystemAddress addr, PlayerList& in, PlayerHandle& res)
{
	for (size_t itr = 0;itr <in.size(); ++itr)
	{
		if (in[itr].Address == addr)
		{
			res = itr;
			return true;
		}
	}
	return false;
}

/************************************************************************************************/

void LobbySetName(Packet* packet, PlayerList& Players )
{
#ifdef _DEBUG
	printf("GetName\n");
#endif
	PlayerHandle hndl;
	if (FindPlayerHandleByAddress(packet->systemAddress, Players, hndl))
	{
		size_t Length = packet->data[1];
		memcpy(Players[hndl].Name, &packet->data[2], Length);
		printf("Player: %s Joined.\n", Players[hndl].Name);
	}
}

/************************************************************************************************/

void LobbySetReady(Packet* packet, PlayerList& Players )
{
#ifdef _DEBUG
	printf("SetReady\n");
#endif
	PlayerHandle hndl;
	if (FindPlayerHandleByAddress(packet->systemAddress, Players, hndl))
	{
		Players[hndl].State = READY;
		printf("Player: %s Ready.\n", Players[hndl].Name);
	}
}

/************************************************************************************************/

void SendPlayer(PlayerInstance* P, SystemAddress addr, Network* net)
{
	RakNet::BitStream bsOut;
	
	bsOut.Write((MessageID)GAME_PLAYERJOINED);
	bsOut.Write((char*)P, sizeof(PlayerInstance));
	net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, addr, false);
}

/************************************************************************************************/

void WaitForGame(EngineMemory* Game, Network* net, PlayerList& PL)
{
	// Handle Packets
	auto packet = net->Peer->Receive();
	if (packet)
	{
		switch (packet->data[0])
		{
		case ID_DISCONNECTION_NOTIFICATION:
			printf("Client Disconnected\n");
			break;
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			net->ServerState->PlayerCount--;
			printf("Client Disconnected\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			net->ServerState->PlayerCount--;
			printf("Client Lost Connection\n");
			break;
		case ID_NEW_INCOMING_CONNECTION:
		{
			AddPlayer(Game, PL, packet->systemAddress);
			printf("Client Connected\n");

			RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID) LOBBY_GETNAME);
			net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);

			break;
		}
		case LOBBY_TEXT:
		{
			RakNet::RakString rs;
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIn.Read(rs);

			printf("%s\n", rs.C_String());
			break;
		}
		case GAME_GETPLAYERNAME:
			LobbySetName(packet, PL);
			break;
		case LOBBY_SETNAME:
			LobbySetName(packet, PL);
			break;
		case LOBBY_SETREADY:
			LobbySetReady(packet, PL);
			break;
		case GAME_BEGIN:
			break;
		}
	}

	net->Peer->DeallocatePacket(packet);

	if (PL.size() >= 1)
	{
		bool ready = true;

		for (auto P : PL)
		{
			if (P.State == NOT_READY)
				ready = ready && false;
		}
		if (ready)
		{
			printf("Game Starting\n");

			for (auto P : PL)
				for (auto P1 : PL)
					if (P.Address != P1.Address)
						SendPlayer(&P1, P.Address, net);

			for (auto P : PL)
			{
				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID) GAME_BEGIN);
				net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, P.Address, false);
			}

			net->ServerState->CurrentState = Network::sServerState::GamePlaying;
		}

	}
}

/************************************************************************************************/

void PrintKeyState(sKeyState& k)
{
	std::cout 
		<< "W:" << k.W << ","
		<< "A:" << k.A << ","
		<< "S:" << k.S << ","
		<< "D:" << k.D << ','
		<< "Q:" << k.Q << ','
		<< "E:" << k.E << "\n";
}

/************************************************************************************************/

void ServerSendPlayerState(PlayerInstance RINOUT PI, PlayerStateCheck* PS, Network* net)
{
	RakNet::BitStream bsOut;

	bsOut.Write((MessageID)GAME_STATESET);
	bsOut.Write((char*)PS, sizeof(PlayerStateCheck));
	net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, PI.Address, false);
}

/************************************************************************************************/

void UpdateServerNetworkEvents(EngineMemory* Game, Network* net, PlayerList& PL)
{
	auto packet = net->Peer->Receive();
	while(packet)
	{
		switch (packet->data[0])
		{
			case GAME_KEYSTATE:
			{
				PlayerHandle hndl;
				if (FindPlayerHandleByAddress(packet->systemAddress, PL, hndl))
				{
					if (packet->length == sizeof(sKeyState) + sizeof(MessageID))
					{
						PL[hndl].InputBuffer[(PL[hndl].tail++)%INPUTBUFFERSIZE] = *reinterpret_cast<sKeyState*>(&packet->data[1]);
					}
				}
				break;
			}
			case GAME_STATECHECKRES:
			{
				PlayerHandle hndl;
				if (FindPlayerHandleByAddress(packet->systemAddress, PL, hndl))
				{
					if (PL[hndl].WaitingCheck)
					{
						PlayerStateCheck PS = *reinterpret_cast<PlayerStateCheck*>(&packet->data[1]);
						PL[hndl].WaitingCheck = false;
						float3 xyz = FlexKit::GetPositionW(&Game->Nodes, PL[hndl].p.PitchNode);


						if (float3(PS.xyz - xyz).magnitude() + ( PL[hndl].p.dV.magnitude() ) > 5)
						{
							Quaternion q;
							FlexKit::GetOrientation(&Game->Nodes, PL[hndl].p.PitchNode, &q);
							printf("Player: %s Failed Check\n", PL[hndl].Name);
							std::cout << "Player Position at: "  << xyz[0] << ',' << xyz[1] << ',' << xyz[2] << "\n  Expected at: "  << PS.xyz[0] << ',' << PS.xyz[1] << ',' << PS.xyz[2] << "\n";
							std::cout << "Q: " << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3] << '\n';
							PS.xyz	= xyz;
							PS.Q	= q;
							ServerSendPlayerState(PL[hndl],&PS, net);
						}
						else
						{
							PL[hndl].p.dV = PS.dv;
							FlexKit::SetPositionW	(&Game->Nodes, PL[hndl].p.PitchNode, PS.xyz);
							FlexKit::SetOrientation	(&Game->Nodes, PL[hndl].p.PitchNode, PS.Q);
						}
					}

				}
				break;
			}
		}
		net->Peer->DeallocatePacket(packet);
		packet = net->Peer->Receive();
	}
}

/************************************************************************************************/

void GameSendStateCheck(PlayerInstance* P, Network* net )
{
	printf("Validating: %s Local State\n", P->Name);

	RakNet::BitStream bsOut;

	bsOut.Write((MessageID)GAME_STATECHECK);
	net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::UNRELIABLE, 0, P->Address, false);
}

/************************************************************************************************/

void GameSendStateCheckRes(PlayerStateCheck* PS, Network* net )
{
	RakNet::BitStream bsOut;

	bsOut.Write((MessageID)GAME_STATECHECKRES);
	bsOut.Write((char*)PS, sizeof(PlayerStateCheck));
	net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, net->ClientState->ServerAddress, false);
}

/************************************************************************************************/

void Server(EngineMemory* Game, Network* net)
{
	// Handle Server
	PlayerList Players;
	double dt = 0.0f;
	double T = 0.0f;
	double StepSize = 1.0f/30.0f;
	double C1 = 0.0f;
	size_t updateCounter = 0;
	Game->Time.Clear();

	// Clear Player Inputs
	for (auto& I : Players)
	{
		memset(I.InputBuffer, 0, sizeof(I.InputBuffer));
	}

	while (!Game->End)
	{
		Game->Time.Before();
		dt = Game->Time.GetAveragedFrameTime();
	
		switch (net->ServerState->CurrentState)
		{
		case Network::sServerState::WaitingForPlayers:
			WaitForGame(Game, net, Players);
			break;
		case Network::sServerState::GamePlaying:
			T += dt;

			if (T > StepSize)
			{
				UpdateServerNetworkEvents(Game, net, Players);
				updateCounter++;

				T -= StepSize;
				for (auto& I : Players)
				{
					I.StateCheckTimer += StepSize;
					if (I.StateCheckTimer > 1.0)
					{
						GameSendStateCheck(&I, net);
						I.StateCheckTimer = 0;
						I.WaitingCheck = true;
					}
					sKeyState k = I.InputBuffer[(I.head++)%INPUTBUFFERSIZE];
					UpdatePlayerMovement(&I.p, k, &Game->Nodes, StepSize);
					float3 POS	= GetPositionW(&Game->Nodes, I.p.PitchNode);
					float3 Dv	= I.p.dV;
				}
			}

			UpdateTransforms(&Game->Nodes);

			C1 += dt;
			if (C1 >= 1.0f)
			{
				C1 = 0.0f;
				for (auto I : Players)
				{

					float3 POS = GetPositionW(&Game->Nodes, I.p.PitchNode);
					float3 DV = I.p.dV;
					std::cout << "Player: " << I.Name << " Position: " << POS[0] << ',' << POS[1] << ',' << POS[2] << " : DV: " << DV[0]  << ',' << DV[1] << ',' << DV[2] << '\n';
				}
				std::cout << " --------------------- FPS: " << updateCounter << '\n';
				updateCounter = 0;
			}
			break;
		default:
			break;
		}

		Sleep(StepSize/4-dt);
		Game->Time.After();
		Game->Time.Update();
	}
}

/************************************************************************************************/

void GameUpdate(EngineMemory* Game, double T, double dt)
{
	T += dt;
		
	UpdateInput();
	/*
	HandleCamera(&Game->Nodes, Game->KeyState, &Game->Player1, dt);


	UpdatePlayerMovement(&Game->Player1, Game->KeyState, &Game->Nodes, dt);

	UpdateCamera(Game->RenderSystem, &Game->Nodes, &Game->PlayerCam, dt);
	UpdateScene(&Game->TestScene, dt);

	UpdateColliders(&Game->TestScene, &Game->Nodes);

	ClearBuffer(&Game->RenderSystem, Game->Window, Game->DSView, { 0.0, 0.0, 0.0, 0.0 });
	DrawRenderables(Game->RenderSystem, &Game->Nodes, &Game->PlayerCam, &Game->PVS, dt);

	PresentWindow(Game->Window);
	*/
}

/************************************************************************************************/

void GameSendKeyState(sKeyState* P, Network* net )
{
	RakNet::BitStream bsOut;

	bsOut.Write((MessageID)GAME_KEYSTATE);
	bsOut.Write((char*)P, sizeof(sKeyState));
	net->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::UNRELIABLE, 0, net->ClientState->ServerAddress, false);
}

/************************************************************************************************/

void ClientConnecting(Packet* packet, Network* net, EngineMemory* Game)
{
	if (packet)
	{
		switch (packet->data[0])
		{
		case ID_NEW_INCOMING_CONNECTION:
			printf("Client Connected\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			net->ClientState->CurrentState	= Network::sClientState::ClientInLobby;
			net->ClientState->ServerAddress = packet->systemAddress;
			break;
		}
	}
}

/************************************************************************************************/

void ClientInLobby(Packet* packet, Network* net, EngineMemory* Game )
{
	if (packet)
	{
		printf("PacketRecieved\n");

		switch (packet->data[0])
		{
		case LOBBY_TEXT:
		{
			RakNet::RakString rs;
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIn.Read(rs);

			printf("%s\n",rs.C_String());
			break;
		}
		case LOBBY_GETNAME:
		{
			std::cout << "Enter Player Name:\n";
			std::cin >> net->ClientState->PlayerName;
			char length = strlen(net->ClientState->PlayerName);
			BitStream bsOut;
			bsOut.Write((MessageID)LOBBY_SETNAME );
			bsOut.Write(length);
			bsOut.Write(net->ClientState->PlayerName, length );
			net->Peer->Send(&bsOut, PacketPriority::MEDIUM_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, net->ClientState->ServerAddress, false );
		}
		{
			BitStream bsOut;
			bsOut.Write((MessageID)LOBBY_SETREADY );
			net->Peer->Send(&bsOut, PacketPriority::MEDIUM_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, net->ClientState->ServerAddress, false );

		}
		break;
		case GAME_PLAYERJOINED:
		{
			assert(0);
			PlayerInstance P;
			memcpy(&P, &packet->data[1], sizeof(PlayerInstance));
			P.p.PitchNode = GetNewNode( &Game->Nodes );
					
			net->ClientState->OtherPlayers.push_back(P);
			/*
			CubeDesc cdesc;
			cdesc.GeometryShader	= Game->GShader;
			cdesc.PixelShader		= Game->PShader;
			cdesc.VertexShader		= Game->VShader;
			CreateBox(&Game->RenderSystem, P.p.Node, &net->ClientState->OtherPlayers.back().p.R, cdesc);

			Game->PVS.push_back(&net->ClientState->OtherPlayers.back().p.R );
			*/
			break;
		}
		case GAME_BEGIN:
		{
			assert(0);
			printf("Game Starting\n");
			FlexKit::NodeHandle node = FlexKit::GetNewNode(&Game->Nodes);
			FlexKit::SetParentNode(&Game->Nodes, Game->RootSN, node);
			ZeroNode(&Game->Nodes, node);

			//InitialisePlayer(&Game->Player1, node );
			//Game->Player1.Node = node;

			/*
			CubeDesc cdesc;
			cdesc.GeometryShader	= Game->GShader;
			cdesc.PixelShader		= Game->PShader;
			cdesc.VertexShader		= Game->VShader;
			CreateBox(&Game->RenderSystem, node, &Game->Player1.R, cdesc);
			Game->PVS.push_back( &Game->Player1.R );
			SetParentNode(&Game->Nodes, node, Game->PlayerCam.Node );

			net->ClientState->CurrentState = Network::sClientState::ClientInGame;
			*/
		}
		break;
		}
	}
}

/************************************************************************************************/

void ClientUpdateNetwork(EngineMemory* Game, Network* net )
{
	/*
	RakNet::Packet* packet = nullptr;
	packet = net->Peer->Receive();
	if (packet)
	{
		while (packet)
		{
			switch (packet->data[0])
			{
			case GAME_STATECHECK:
			{
				printf("Server Validating Local State\n");
				PlayerStateCheck PS;
				FlexKit::GetOrientation(&Game->Nodes, Game->Player1.Node, &PS.Q);
				PS.xyz = FlexKit::GetPositionW(&Game->Nodes, Game->Player1.Node);
				PS.dv  = Game->Player1.dV;

				//GetOrientation(&Game->Nodes, Game->Player1.Node, &q);
				std::cout << "Position: " << PS.xyz[0] << ',' << PS.xyz[1] << ',' << PS.xyz[2] << '\n';
				std::cout << "Q: " << PS.Q[0] << ',' << PS.Q[1] << ',' << PS.Q[2] << ',' << PS.Q[3] << '\n';

				GameSendStateCheckRes(&PS, net);
			}	break;
			case GAME_STATESET:
			{
				printf("Server Setting Local State\n");
				PlayerStateCheck* PS = reinterpret_cast<PlayerStateCheck*>(&packet->data[1]);
				Game->Player1.dV = PS->dv;
				FlexKit::SetPositionW	(&Game->Nodes, Game->Player1.Node, PS->xyz);
				FlexKit::SetOrientation	(&Game->Nodes, Game->Player1.Node, PS->Q);

				UpdateTransforms(&Game->Nodes);
				float3 POS = GetPositionW(&Game->Nodes, Game->Player1.Node);
				Quaternion q = PS->Q;
				//GetOrientation(&Game->Nodes, Game->Player1.Node, &q);
				std::cout << "Position: " << POS[0] << ',' << POS[1] << ',' << POS[2] << '\n';
				std::cout << "Q: " << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3] << '\n';

			}	break;
			case GAME_END:
				Game->End = true;
			default:
				break;
			}
			net->Peer->DeallocatePacket(packet);
			packet = net->Peer->Receive();
		}
	}
	GameSendKeyState(&Game->KeyState, net);
	*/
}

/************************************************************************************************/

bool GameLobby(EngineMemory* Game, Network* net )
{
	bool PlayGame = false;

	while (!Game->End && !&Game->Window.Close && !PlayGame)
	{
		FlexKit::UpdateInput();
		auto packet = net->Peer->Receive();

		switch (net->ClientState->CurrentState)
		{
		case Network::sClientState::ClientDisconnected:
			break;
		case Network::sClientState::ClientEstablishingConnection:
		{
			ClientConnecting(packet, net, Game);
		}	break;
		case Network::sClientState::ClientInGame:
			PlayGame = true;
			break;
		case Network::sClientState::ClientInLobby:
		{
			ClientInLobby(packet, net, Game);
		}
			break;

		default:
			break;
		}
		net->Peer->DeallocatePacket(packet);
	}

	return PlayGame;
}

/************************************************************************************************/

void PlayGame(EngineMemory* Game, Network* net )
{
	/*
	double Time           = 0.0f;
	size_t FPS            = 0;
	size_t FrameCounter   = 0;;
	double T              = 0.0f;
	double dt             = 0;
	const double StepSize = 1 / 30.0f;

	while (!Game->End && !Game->Window->Close)
	{
		Game->Time.Before();

		double dt = Game->Time.GetAveragedFrameTime();
		T += dt;
		
		UpdateInput();

		UpdateScene(&Game->TestScene, dt);
		UpdateColliders(&Game->TestScene, &Game->Nodes);
	
		if (T > StepSize)
		{
			T -= StepSize;
			ClientUpdateNetwork( Game, net );

			UpdatePlayerMovement(&Game->Player1, Game->KeyState, &Game->Nodes, StepSize);
			UpdateTransforms(&Game->Nodes);
			UpdateCamera(Game->RenderSystem, &Game->Nodes, &Game->PlayerCam, dt);

			ClearGBuffer(Game->RenderSystem, &Game->GB);
			ClearBuffer(&Game->RenderSystem, Game->Window, Game->DSView, { 0.0, 0.0, 0.0, 0.0 });
			
			SetupDraw(Game);
			DrawRenderables(Game->RenderSystem, &Game->Nodes, &Game->PlayerCam, &Game->PVS, dt);
			//ShadeGBuffer(Game->RenderSystem, &Game->GB, &Game->PlayerCam, Game->Window);
		}

		PresentWindow(Game->Window);

		Game->Time.After();
		Game->Time.Update();
	}
	*/
}

/************************************************************************************************/

void Client(EngineMemory* Game, Network* net )
{
	/*
	LoadShaders			(Game);
	InitiateCoreSystems (Game);
	SetupDraw			(Game);
	
	FlexKit::NodeHandle node = FlexKit::GetNewNode(&Game->Nodes);
	FlexKit::ZeroNode(&Game->Nodes, node);
	InitialisePlayer(&Game->Player1, node);

	ZeroNode(&Game->Nodes,node);
	FlexKit::SetParentNode(&Game->Nodes, Game->RootSN, node);

	SetParentNode(&Game->Nodes, Game->Player1.Node, Game->PlayerCam.Node );
	Game->Time.PrimeLoop();
	
	if(GameLobby(Game, net))
		PlayGame(Game, net);

	CleanUpPlayer(&Game->Player1, &Game->Nodes);
	ReleaseEngine(Game);
	*/
}

/************************************************************************************************/

void MultiplayerMode(EngineMemory* Game, bool isServer)
{
	memset(Game, 0, PRE_ALLOC_SIZE);
	InitiateEngineMemory(Game);

	Game->TempAllocator.Init(Game->TempMem, sizeof(Game->TempMem));
	Game->LevelAllocator.Init(Game->TempMem, sizeof(Game->LevelMem));

	Network* net = &Game->LevelAllocator.allocate<Network>();
	InitiateNetwork( net, &Game->LevelAllocator, isServer, 60000);

	if (isServer)
		Server(Game, net);
	else
		Client(Game, net);

	CleanupNetwork(net);
}

/************************************************************************************************/

#endif