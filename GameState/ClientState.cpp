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


void SendReady(ClientState* Client)
{

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

				switch (IncomingPacket->ID)
				{
					case eGAMEINFODATA:
					{
						GameInfoPacket* Info   = (GameInfoPacket*)IncomingPacket;
						ThisState->PlayerCount = Info->PlayerCount;

						ThisState->PlayerIds.resize(Info->PlayerCount);

						for (size_t itr = 0; itr < ThisState->PlayerCount; ++itr)
							ThisState->PlayerIds[itr] = Info->PlayerIDs[itr];
					}	break;
				default:
					break;
				}
			}	break;
			case eGAMESTARTING:
			{
				printf("eGAMESTARTED Received!\n");
				PushSubState(ThisState->Base, CreateClientPlayState(Engine, ThisState->Base, ThisState));
			}	break;
			case eGAMEENDED:
			{
				printf("eGAMEENDED Received!\n");
				StateMemory->Base->Quit = true;
			}	break;
			case eREQUESTNAME: 
				printf("Name Request Received!\n");
			{	SendName(ThisState);
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


bool UpdateClientEventHandler(SubState* StateMemory, Event)
{
	return true;
}


/************************************************************************************************/


bool UpdateClientGameplay(SubState* StateMemory, EngineMemory*, double DT)
{
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

	PlayState->Vipiraks.resize(Client->PlayerCount);

	return PlayState;
}


/************************************************************************************************/