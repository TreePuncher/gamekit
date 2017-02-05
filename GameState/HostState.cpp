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

#include "HostState.h"
#include <Raknet\Source\MessageIdentifiers.h>
#include <Raknet\Source\RakPeerInterface.h>


/************************************************************************************************/


void SendGameINFO(HostState* Host, RakNet::SystemAddress Addr)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eINCOMINGSTRUCT);
	size_t PacketSize = sizeof(GameInfoPacket) + (sizeof(PlayerID_t) * Host->PlayerCount);
	GameInfoPacket* OutPacket = (GameInfoPacket*)Host->Base->Engine->BlockAllocator._aligned_malloc( PacketSize);
	new(OutPacket) GameInfoPacket(Host->PlayerCount);

	
	for (size_t itr = 0, OutIndex = 0; itr < Host->PlayerCount - 1; itr++) {
		if (Host->OpenConnections[itr].Addr != Addr)
		{
			OutPacket->PlayerIDs[OutIndex] = Host->OpenConnections[OutIndex].ID;
			OutIndex++;
		}
	}

	bsOut.Write((char*)&OutPacket, PacketSize);
	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


void SendGameBegin(HostState* Host, RakNet::SystemAddress Addr)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eGAMESTARTING);

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


void SendClientInfo(HostState* Host, RakNet::SystemAddress Addr, size_t ID)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eSENDCLIENTINFO);

	ConnectionAccepted Packet;
	Packet.PlayerID = ID;

	bsOut.Write((char*)&Packet, sizeof(Packet));

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


void SendServerTerminated(HostState* Host, RakNet::SystemAddress Addr)
{
	RakNet::BitStream bsOut;
	bsOut.Write(eGAMEENDED);

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


void SendNameRequest(HostState* Host, RakNet::SystemAddress Addr)
{
	RakNet::BitStream bsOut;
	bsOut.Write(NetCommands::eREQUESTNAME);

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


size_t GetPlayerIndex(HostState* Host, RakNet::SystemAddress Addr)
{
	size_t out = -1;

	for (size_t i = 0; i < Host->OpenConnections.size(); ++i)
	{
		if (Host->OpenConnections[i].Addr == Addr) {
			out = i;
			break;
		}
	}

	return out;
}


/************************************************************************************************/


void BeginGame(HostState* Host)
{
	printf("Starting Game!\n");

	auto ThisState = (HostState*)Host;

	for (auto& C : ThisState->OpenConnections) {
		SendGameINFO(ThisState, C.Addr);
	}

	for (auto& C : ThisState->OpenConnections) {
		SendGameBegin(ThisState, C.Addr);
	}

	ThisState->VTable.Update = nullptr;

}


/************************************************************************************************/


void CloseServer(SubState* StateMemory)
{
	printf("Shutting down Game!\n");

	auto ThisState = (HostState*)StateMemory;
	
	for (auto& C : ThisState->OpenConnections) {
		SendServerTerminated(ThisState, C.Addr);
	}

	Sleep(1000);
	ThisState->Peer->Shutdown(0);

	//RakNet::RakPeerInterface::ReleaseInstance(ThisState->Peer);
}


/************************************************************************************************/


bool WaitForPlayers(SubState* StateMemory, EngineMemory*, double dT)
{
	auto ThisState = (HostState*)StateMemory;
	ThisState->T += dT;

	RakNet::Packet* Packet = nullptr;
	
	while (Packet = ThisState->Peer->Receive(), Packet) {
		switch (Packet->data[0])
		{
		case ID_DISCONNECTION_NOTIFICATION:
			break;
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			break;
		case ID_REMOTE_CONNECTION_LOST:
			break;
		case ID_NEW_INCOMING_CONNECTION:
		{
			ThisState->OpenConnections.push_back({ Packet->systemAddress });
			ThisState->PlayerCount++;
			ThisState->PlayerReadyState.push_back(false);

			SendClientInfo(ThisState, ThisState->OpenConnections.back().Addr, ThisState->OpenConnections.size());
			SendNameRequest(ThisState, ThisState->OpenConnections.back().Addr);
		}	break;
		case eINCOMINGSTRUCT:
		{
			printf("PacketIncoming\n");

			PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];
			switch (IncomingPacket->ID)
			{
			case eNAMEREPLY: {
				auto Index = GetPlayerIndex(ThisState, Packet->systemAddress);
				GetNamePacket* Name = (GetNamePacket*)IncomingPacket;
				strncpy(ThisState->Names[Index].Name, Name->Name, MaxNameSize);

				printf("%s Joined!\n", ThisState->Names[Index].Name);
			}	break;
			default:
				break;
			}
		}	break;
		default:
			break;
		}
		ThisState->Peer->DeallocatePacket(Packet);
	};

	if (ThisState->PlayerCount >= ThisState->MinPlayerCount)
		BeginGame(ThisState);

	return true;
}


/************************************************************************************************/


HostState* CreateHostState(EngineMemory* Engine, BaseState* Base)
{
	auto State = &Engine->BlockAllocator.allocate_aligned<HostState>();
	State->VTable.Update  = WaitForPlayers;
	State->VTable.Release = CloseServer;
	State->MinPlayerCount = 3;
	State->Base           = Base;
	State->PlayerCount	  = 0;
	State->Peer			  = RakNet::RakPeerInterface::GetInstance();
	
	RakNet::SocketDescriptor sd(gServerPort, nullptr);

	auto res = State->Peer->Startup(16, &sd, 1);

	State->Peer->SetMaximumIncomingConnections(16);
	State->CurrentState = HostState::WaitingForPlayers;

	return State;
}


/************************************************************************************************/
