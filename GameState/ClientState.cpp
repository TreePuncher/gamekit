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

#include "Client.h"
#include "PlayState.h"
#include "HostState.h"


/************************************************************************************************/


void SendName(ClientState* Client)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eINCOMINGSTRUCT);
	GetNamePacket OutPacket(Client->ClientName);
	bsOut.Write((char*)&OutPacket, sizeof(GetNamePacket));

	Client->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE, 0, Client->ServerAddr, false);
}


void SendMode(ClientState* Client, ClientMode Mode)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eINCOMINGSTRUCT);
	GameModePacket ModePacket(Mode);
	bsOut.Write((char*)&ModePacket, sizeof(ModePacket));

	Client->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE, 0, Client->ServerAddr, false);
}


template<typename TY_1, typename ... TY_2>
void SendDataPacket(ClientState* Client, TY_2 ... Input)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eINCOMINGSTRUCT);
	TY_1 OutPacket(Input...);
	bsOut.Write(OutPacket, sizeof(TY_1));

	Client->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::UNRELIABLE, 0, Client->ServerAddr, false);
}


void SendReady(ClientState* Client)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eGAMEREADYUP);

	Client->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE, 0, Client->ServerAddr, false);
}


/************************************************************************************************/


void SendPlayerClientStateInfoUpdate(ClientState* Client, Player* P, size_t FrameID)
{
	//printf("Sending to Host the Client Player State!\n");

	RakNet::BitStream bsOut;
	bsOut.Write(eINCOMINGSTRUCT);

	PlayerStateFrame Frame;
	Frame.FrameID  = FrameID;
	Frame.Yaw      = P->CameraCTR.Yaw;
	Frame.Pitch    = P->CameraCTR.Pitch;
	Frame.Roll     = P->CameraCTR.Roll;
	Frame.Velocity = P->PlayerCTR.Velocity;
	Frame.Position = P->PlayerCTR.Pos;

	PlayerClientStateInfoUpdate Packet(Frame);
	bsOut.Write(Packet, sizeof(Packet));

	Client->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Client->ServerAddr, false);
}


/************************************************************************************************/


bool JoinServer(SubState* StateMemory, EngineMemory* Engine, double DT)
{
	auto ThisState = (ClientState*)StateMemory;

	RakNet::Packet* Packet = nullptr;

	while (Packet = ThisState->Peer->Receive(), Packet) {
		if (Packet)
		{
			printf("Client Packet Received!\n");

			switch (Packet->data[0])
			{
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				printf("Club Entry Form Accepted!\n");
				ThisState->ServerAddr = Packet->systemAddress;
			}	break;
			case eSENDCLIENTINFO:
			{	ConnectionAccepted* ClientInfo = (ConnectionAccepted*)&Packet->data[1];
				ThisState->ID = ClientInfo->PlayerID;
				printf("Client ID: %u", unsigned int(ThisState->ID));
			}	break;
			case ID_DISCONNECTION_NOTIFICATION:
				break;
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				break;
			case ID_REMOTE_CONNECTION_LOST:
			{
			}	break;
			case ID_NEW_INCOMING_CONNECTION:
			{
			}	break;
			case eINCOMINGSTRUCT:
			{
				PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];
				printf("eIncomingStruct!\nType %u", IncomingPacket->ID);

				switch (IncomingPacket->ID)
				{
				case GetTypeGUID(GameInfoPacket):
					{
						printf("GameInfoPacket!\n");

						GameInfoPacket* Info   = (GameInfoPacket*)IncomingPacket;
						ThisState->PlayerCount = Info->PlayerCount;

						ThisState->PlayerIds.resize(Info->PlayerCount - 1);

						for (size_t itr = 0; itr < ThisState->PlayerCount - 1; ++itr)
							ThisState->PlayerIds[itr] = Info->PlayerIDs[itr];

						SendMode(ThisState, eLOADINGMODE);

						bool RES = LoadScene(ThisState->Base, Info->LevelName);
						if (!RES)
							printf("Failed to Load Scene: %s \n", Info->LevelName);

						PushSubState(ThisState->Base, CreateClientPlayState(Engine, ThisState->Base, ThisState));

						SendMode(ThisState, eWAITINGMODE);
					}	break;
				default:
					break;
				}
			}	break;
			case eGAMEENDED:
			{
				printf("eGAMEENDED Received!\n");
				StateMemory->Base->Quit = true;
			}	break;
			case eREQUESTNAME: 
				printf("Name Request Received!\n");
			{	SendName(ThisState);
				SendReady(ThisState);
			}	break;
			default:
				break;
			}
			ThisState->Peer->DeallocatePacket(Packet);
		}
	};

	return true;
}


/************************************************************************************************/


ClientState* CreateClientState(EngineMemory* Engine, GameFramework* Base, const char* Name, const char* Server)
{
	auto State = &Engine->BlockAllocator.allocate_aligned<ClientState>();
	State->VTable.Update       = JoinServer;
	State->Base                = Base;
	State->Peer                = RakNet::RakPeerInterface::GetInstance();
	State->PlayerIds.Allocator = Engine->BlockAllocator;


	char str[512];

	RakNet::SocketDescriptor desc;
	memset(str, 0, sizeof(str));

	if (!Name)
	{
		std::cout << "Enter Name\n";
		std::cin >> State->ClientName;
	}
	else
		strncpy(State->ClientName, Name, sizeof(State->ClientName));

	if (!Server)
	{
		std::cout << "Enter Server Address\n";
		std::cin >> str;
	}

	State->Peer->Startup(1, &desc, 1);

	auto res = State->Peer->Connect(Server ? Server : str, gServerPort, nullptr, 0);


	return State;
}


/************************************************************************************************/


bool UpdateClientEventHandler(SubState* StateMemory, Event evt)
{
	ClientPlayState* ThisState = (ClientPlayState*)StateMemory;

	if (evt.InputSource == Event::Keyboard)
	{
		switch (evt.Action)
		{
		case Event::Pressed:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_W: {
				ThisState->LocalInput.Forward  = true;
				break;
			}
			case KC_S: {
				ThisState->LocalInput.Backward = true;
			}	break;
			case KC_A: {
				ThisState->LocalInput.Left     = true;
			}	break;
			case KC_D: {
				ThisState->LocalInput.Right    = true;
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
				ThisState->LocalInput.Forward  = false;
				break;
			}
			case KC_S: {
				ThisState->LocalInput.Backward = false;
			}	break;
			case KC_A: {
				ThisState->LocalInput.Left     = false;
			}	break;
			case KC_D: {
				ThisState->LocalInput.Right    = false;
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


bool UpdateClientPreDraw(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	ClientPlayState* ThisState = (ClientPlayState*)StateMemory;

	UpdatePlayerAnimations(ThisState->Base, &ThisState->LocalPlayer, dT);

	return true;
}


/************************************************************************************************/


void HandlePackets(ClientPlayState* ThisState, EngineMemory*, double dT)
{
	RakNet::Packet* Packet = nullptr;

	while (Packet = ThisState->NetState->Peer->Receive(), Packet) {
		if (Packet)
		{
			switch (Packet->data[0])
			{
			case eINCOMINGSTRUCT: {
				PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];

				switch (IncomingPacket->ID)
				{
				case GetTypeGUID(PlayerClientStateInfoUpdate): {
					PlayerClientStateInfoUpdate* Info = (PlayerClientStateInfoUpdate*)IncomingPacket;
					Info->State.Yaw;

					printf("Server Moving Local Player!\n");

					size_t FrameIDLocal		= ThisState->FrameCount;
					size_t FrameIDServer	= Info->State.FrameID;

					size_t FrameDelta = FrameIDLocal - FrameIDServer;
					std::cout << FrameDelta << "Frame Delta\n";

					float3 POS;
					float3 V;
					memcpy(&POS, &Info->State.Position, sizeof(POS));
					memcpy(&V, &Info->State.Velocity, sizeof(V));

					SetPlayerPosition(&ThisState->LocalPlayer, POS);
					ThisState->LocalPlayer.CameraCTR.Yaw		= Info->State.Yaw;
					ThisState->LocalPlayer.PlayerCTR.Velocity	= V;

					
					size_t InputBufferStartIdx = 0;
					for (size_t I = 0; I < ThisState->InputBuffer.size(); ++I)
						if (ThisState->InputBuffer[I].FrameID == FrameIDServer) {
							InputBufferStartIdx = I;
							break;
						}

					
					for (size_t I = 0; I < FrameDelta; ++I) {
						auto input = ThisState->InputBuffer[InputBufferStartIdx + I];
						UpdatePlayer(ThisState->Base, &ThisState->LocalPlayer, input.KeyboardInput, input.MouseInput, dT);
					}

				}	break;
				case GetTypeGUID(SetPlayerInfoPacket): {

					SetPlayerInfoPacket* Info = (SetPlayerInfoPacket*)IncomingPacket;

					if (Info->PlayerID == ThisState->NetState->ID) {
						printf("Server Moving Local Player!\n");
						SetPlayerPosition	(&ThisState->LocalPlayer, Info->POS);
						SetPlayerOrientation(&ThisState->LocalPlayer, Info->R);
					}
					else
					{
						for (auto& I : ThisState->Imposters) {
							if (I.PlayerID == Info->PlayerID)
							{
								auto temp = Info->R;
								ThisState->Base->GScene.SetPositionEntity_WT(I.Graphics, Info->POS);
								ThisState->Base->GScene.SetOrientation(I.Graphics, temp.Inverse());
							}
						}
					}
				}	break;
				default:
					break;
				}

			}	break;
			case eGAMESTARTING:
			{
				SendMode(ThisState->NetState, ePLAYMODE);
				printf("eGAMESTARTED Received!\n");
			}	break;
			default:
				break;
			}
			ThisState->NetState->Peer->DeallocatePacket(Packet);
		}
	}
}


/************************************************************************************************/


bool UpdateClientGameplay(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (ClientPlayState*)StateMemory;
	auto Scene = ThisState->Base->ActiveScene;

	float HorizontalMouseMovement	= float(ThisState->Base->MouseState.dPos[0]) / GetWindowWH(Engine)[0];
	float VerticalMouseMovement		= float(ThisState->Base->MouseState.dPos[1]) / GetWindowWH(Engine)[1];

	ThisState->T			+= dT;
	ThisState->MouseInput	+= float2{ HorizontalMouseMovement, VerticalMouseMovement };

	HandlePackets(ThisState, Engine, dT);

	InputFrame CurrentInputState;
	CurrentInputState.FrameID		= ThisState->FrameCount;
	CurrentInputState.KeyboardInput = ThisState->LocalInput;
	CurrentInputState.MouseInput	= ThisState->MouseInput;

	ThisState->InputBuffer.push_back(CurrentInputState);

	const double UpdateStep = 1.0 / 30.0;
	if (ThisState->T > UpdateStep) {
		UpdatePlayer(ThisState->Base, &ThisState->LocalPlayer, ThisState->LocalInput,
					 ThisState->MouseInput, UpdateStep);

		SendDataPacket<PlayerInputPacket>(ThisState->NetState, CurrentInputState);// Send Client Input

		ThisState->FrameCount++;
		ThisState->T -= UpdateStep;
		ThisState->MouseInput = {0.0f, 0.0f};
	}

	ThisState->T2ServerUpdate += dT;
	if (ThisState->T2ServerUpdate > ThisState->ServerUpdatePeriod)
	{
		ThisState->T2ServerUpdate = 0;
		SendPlayerClientStateInfoUpdate(ThisState->NetState, &ThisState->LocalPlayer, ThisState->FrameCount);
	}


	if (ThisState->T_TillNextPositionUpdate > 1.0f)
	{
		SendDataPacket<PlayerInfoPacket>(
			ThisState->NetState,
			ThisState->NetState->ID,
			GetOrientation(&ThisState->LocalPlayer, Scene),
			GetPlayerPosition(&ThisState->LocalPlayer));

		ThisState->T_TillNextPositionUpdate = 0;
	}
	else
		ThisState->T_TillNextPositionUpdate += dT;

	auto POS = GetPositionW(Engine->Nodes, ThisState->LocalPlayer.CameraCTR.Yaw_Node);

#if 0
	printf("POS{");
	printfloat3(POS);
	printf("}, Q{");
	printQuaternion(GetOrientation(ThisState->LocalPlayer, Scene));
	printf("}\n");
#endif

	return true;
}



/************************************************************************************************/


ClientPlayState* CreateClientPlayState(EngineMemory* Engine, GameFramework* Base, ClientState* Client)
{
	ClientPlayState* PlayState = &Engine->BlockAllocator.allocate_aligned<ClientPlayState>();
	PlayState->Base                           = Base;
	PlayState->NetState						  = Client;
	PlayState->VTable.Update				  = UpdateClientGameplay;
	PlayState->VTable.EventHandler			  = UpdateClientEventHandler;
	PlayState->VTable.PreDrawUpdate			  = UpdateClientPreDraw;
	PlayState->LocalPlayer.PlayerCTR.Pos      = float3(0, 0, 0);
	PlayState->LocalPlayer.PlayerCTR.Velocity = float3(0, 0, 0);
	PlayState->Imposters.Allocator			  = Engine->BlockAllocator;
	PlayState->Mode							  = eWAITINGMODE; // Wait for all Players to Load and respond
	PlayState->FrameCount					  = 0;
	PlayState->T2ServerUpdate				  = 0.0;
	PlayState->ServerUpdatePeriod			  = 1.0;

	PlayState->LocalInput.ClearState();
	PlayState->Imposters.resize(Client->PlayerCount - 1);

	for (size_t I = 0; I < PlayState->Imposters.size(); ++I) {
		auto& Imposter	  = PlayState->Imposters[I];
		Imposter.Graphics = Base->GScene.CreateDrawableAndSetMesh("PlayerModel");
		Imposter.PlayerID = Client->PlayerIds[I];
	}

	CreatePlaneCollider(Engine->Physics.DefaultMaterial, &Base->PScene);
	InitiatePlayer(Base, &PlayState->LocalPlayer);

	return PlayState;
}


/************************************************************************************************/
