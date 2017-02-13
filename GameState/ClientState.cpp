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


ClientState* CreateClientState(EngineMemory* Engine, BaseState* Base)
{
	auto State = &Engine->BlockAllocator.allocate_aligned<ClientState>();
	State->VTable.Update       = JoinServer;
	State->Base                = Base;
	State->Peer                = RakNet::RakPeerInterface::GetInstance();
	State->PlayerIds.Allocator = Engine->BlockAllocator;


	char str[512];

	RakNet::SocketDescriptor desc;
	memset(str, 0, sizeof(str));
	std::cout << "Enter Name\n";
	std::cin >> State->ClientName;

	std::cout << "Enter Server Address\n";
	std::cin >> str;
	State->Peer->Startup(1, &desc, 1);

	auto res = State->Peer->Connect(str, gServerPort, nullptr, 0);


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


void HandlePackets(ClientPlayState* ThisState, EngineMemory*, double dT)
{
	RakNet::Packet* Packet = nullptr;

	while (Packet = ThisState->ClientState->Peer->Receive(), Packet) {
		if (Packet)
		{
			printf("Client Packet Received!\n");
			switch (Packet->data[0])
			{
			case eINCOMINGSTRUCT: {
				PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];

				switch (IncomingPacket->ID)
				{
				case GetTypeGUID(SetPlayerInfoPacket): {
					printf("Server Moving Player!\n");

					SetPlayerInfoPacket* Info = (SetPlayerInfoPacket*)IncomingPacket;

					if (Info->PlayerID == ThisState->ClientState->ID) {
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
				SendMode(ThisState->ClientState, ePLAYMODE);
				printf("eGAMESTARTED Received!\n");
			}	break;
			default:
				break;
			}
			ThisState->ClientState->Peer->DeallocatePacket(Packet);
		}
	}
}


/************************************************************************************************/


bool UpdateClientGameplay(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (ClientPlayState*)StateMemory;

	float HorizontalMouseMovement	= float(ThisState->Base->MouseState.dPos[0]) / GetWindowWH(Engine)[0];
	float VerticalMouseMovement		= float(ThisState->Base->MouseState.dPos[1]) / GetWindowWH(Engine)[1];

	ThisState->T += dT;

	ThisState->MouseInput += float2{ HorizontalMouseMovement, VerticalMouseMovement };

	HandlePackets(ThisState, Engine, dT);

	const double UpdateStep = 1.0 / 30.0;
	if (ThisState->T > UpdateStep) {
		UpdatePlayer(ThisState->Base, &ThisState->LocalPlayer, ThisState->LocalInput,
					 ThisState->MouseInput, UpdateStep);

		SendDataPacket<PlayerInputPacket>(
				ThisState->ClientState, ThisState->LocalInput, 
				ThisState->MouseInput, 
				ThisState->FrameCount);// Send Client Input

		ThisState->FrameCount++;
		ThisState->T -= UpdateStep;
		ThisState->MouseInput = {0.0f, 0.0f};
	}

	if (ThisState->T_TillNextPositionUpdate > 1.0f)
	{
		SendDataPacket<PlayerInfoPacket>(
			ThisState->ClientState,
			ThisState->ClientState->ID,
			GetOrientation(&ThisState->LocalPlayer),
			GetPlayerPosition(&ThisState->LocalPlayer));

		ThisState->T_TillNextPositionUpdate = 0;
	}
	else
		ThisState->T_TillNextPositionUpdate += dT;

	auto POS = GetPositionW(Engine->Nodes, ThisState->LocalPlayer.CameraCTR.Roll_Node);

#if 1
	printf("POS{");
	printfloat3(POS);
	printf("}, Q{");
	printQuaternion(GetOrientation(ThisState->LocalPlayer));
	printf("}\n");
#endif

	return true;
}


/************************************************************************************************/


ClientPlayState* CreateClientPlayState(EngineMemory* Engine, BaseState* Base, ClientState* Client)
{
	ClientPlayState* PlayState = &Engine->BlockAllocator.allocate_aligned<ClientPlayState>();
	PlayState->Base                           = Base;
	PlayState->ClientState                    = Client;
	PlayState->VTable.Update				  = UpdateClientGameplay;
	PlayState->VTable.EventHandler			  = UpdateClientEventHandler;
	PlayState->LocalPlayer.PlayerCTR.Pos      = float3(0, 0, 0);
	PlayState->LocalPlayer.PlayerCTR.Velocity = float3(0, 0, 0);
	PlayState->Imposters.Allocator			  = Engine->BlockAllocator;
	PlayState->Mode							  = eWAITINGMODE; // Wait for all Players to Load and respond
	PlayState->FrameCount					  = 0;

	PlayState->LocalInput.ClearState();
	PlayState->Imposters.resize(Client->PlayerCount - 1);

	for (size_t I = 0; I < PlayState->Imposters.size(); ++I) {
		auto& Imposter	  = PlayState->Imposters[I];
		Imposter.Graphics = Base->GScene.CreateDrawableAndSetMesh("PlayerModel");
		Imposter.PlayerID = Client->PlayerIds[I];
	}

	InitiatePlayer(Base, &PlayState->LocalPlayer);

	return PlayState;
}


/************************************************************************************************/