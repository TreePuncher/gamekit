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
	printf("Sending Game Info!\n");

	RakNet::BitStream bsOut;
	bsOut.Write(eINCOMINGSTRUCT);

	size_t PacketSize = sizeof(GameInfoPacket) + (sizeof(PlayerID_t) * Host->PlayerCount);
	GameInfoPacket* OutPacket = (GameInfoPacket*)Host->Base->Engine->BlockAllocator._aligned_malloc( PacketSize);
	new(OutPacket) GameInfoPacket(Host->PlayerCount, "ShaderBallScene");

	for (size_t itr = 0, OutIndex = 0; itr < Host->PlayerCount; itr++) {
		if (Host->OpenConnections[itr].Addr != Addr)
		{
			OutPacket->PlayerIDs[OutIndex] = Host->OpenConnections[itr].ID;
			OutIndex++;
		}
	}

	bsOut.Write((char*)OutPacket, PacketSize);
	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/

void SendSetPlayerPositionRotation(HostState* Host, RakNet::SystemAddress Addr, float3 xyz, Quaternion Q, size_t PlayerID)
{
	printf("Setting Player Position:");
	SetPlayerInfoPacket NewPacket(xyz, Q);
	NewPacket.PlayerID = PlayerID;
	RakNet::BitStream bsOut;

	bsOut.Write(eINCOMINGSTRUCT);
	bsOut.Write((char*)&NewPacket, sizeof(NewPacket));

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


void SendGameBegin(HostState* Host, RakNet::SystemAddress Addr)
{
	printf("Sending Game Begin!\n");
	RakNet::BitStream bsOut;
	bsOut.Write(eGAMESTARTING);

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


void SendClientInfo(HostState* Host, RakNet::SystemAddress Addr, size_t ID)
{
	printf("Sending Client Info!\n");

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
	printf("Sending Game Over!\n");

	RakNet::BitStream bsOut;
	bsOut.Write(eGAMEENDED);

	Host->Peer->Send(&bsOut, PacketPriority::IMMEDIATE_PRIORITY, PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, Addr, false);
}


/************************************************************************************************/


void SendNameRequest(HostState* Host, RakNet::SystemAddress Addr)
{
	printf("Sending Name Request!\n");

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


bool CheckPlayerReadyStates(HostState* Host)
{
	bool out = !Host->PlayerReadyState.empty();

	for (auto B : Host->PlayerReadyState)
		out &= B;

	return out;
}


/************************************************************************************************/


void QueueGameLoad(HostState* Host)
{
	printf("Starting Game!\n");

	auto ThisState = (HostState*)Host;

	for (auto& C : ThisState->OpenConnections) {
		SendGameINFO(ThisState, C.Addr);
	}

	ThisState->Game.SetPlayerCount(Host->Base, Host->PlayerCount);
	ThisState->ServerMode = ServerMode::eCLIENTLOADWAIT;
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


void LobbyMode(HostState* ThisState, EngineMemory* ENgine, double dT)
{
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
			size_t PlayerID = ThisState->OpenConnections.size();
			ThisState->OpenConnections.push_back({ Packet->systemAddress, PlayerID });
			ThisState->PlayerCount++;
			ThisState->PlayerReadyState.push_back(false);
			ThisState->ClientModes.push_back(ClientMode::eLOBBYMODE);

			SendClientInfo(ThisState, ThisState->OpenConnections.back().Addr, PlayerID);
			SendNameRequest(ThisState, ThisState->OpenConnections.back().Addr);
		}	break;
		case eGAMEREADYUP:
		{
			auto Index = GetPlayerIndex(ThisState, Packet->systemAddress);
			printf("Player %s Ready\n", ThisState->Names[Index].Name);
			ThisState->PlayerReadyState[Index] = true;
		}	break;
		case eINCOMINGSTRUCT:
		{
			printf("PacketIncoming\n");

			PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];
			switch (IncomingPacket->ID)
			{
			case GetTypeGUID(GetNamePacket): {
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

	if (ThisState->PlayerCount >= ThisState->MinPlayerCount) {
		if (CheckPlayerReadyStates(ThisState))
			QueueGameLoad(ThisState);
	}
}


void LoadWaitMode(HostState* ThisState, EngineMemory* ENgine, double dT)
{
	RakNet::Packet* Packet = nullptr;

	while (Packet = ThisState->Peer->Receive(), Packet) {
		switch (Packet->data[0])
		{
		case eINCOMINGSTRUCT:
		{
			PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];
			switch (IncomingPacket->ID)
			{
			case GetTypeGUID(GameModePacket):
			{
				size_t ClientIdx = GetPlayerIndex(ThisState, Packet->systemAddress);
				GameModePacket* ModePacket = (GameModePacket*)IncomingPacket;
				ThisState->ClientModes[ClientIdx] = ModePacket->Mode;
			}break;
			}
		}	break;
		}
		ThisState->Peer->DeallocatePacket(Packet);
	}

	bool ClientsStillLoading = false;
	for (auto ClientMode : ThisState->ClientModes)
	{
		if (ClientMode == ClientMode::eLOADINGMODE) {
			ClientsStillLoading = true;
			break;
		}
	}

	if (!ClientsStillLoading)
	{
		Sleep(1000);

		// Set Initial Player Positions
		for (size_t I = 0; I < ThisState->PlayerCount; ++I) {
			float3 Position = float3{ 10, 0, 20 } + I * float3{ 10, 0, 0 };
			SetPlayerPosition(&ThisState->Game.Players[I], Position);
			SendSetPlayerPositionRotation(ThisState, ThisState->OpenConnections[I].Addr, 
				Position, 
				GetOrientation(&ThisState->Game.Players[I]),
				ThisState->OpenConnections[I].ID);
		}

		Sleep(1000);

		for (auto& C : ThisState->OpenConnections) {
			SendGameBegin(ThisState, C.Addr);
			ThisState->ServerMode = ServerMode::eGAMEINPROGRESS;
		}

	}
}


/************************************************************************************************/


void GameInProgressMode(HostState* ThisState, EngineMemory* Engine, double dT)
{
	RakNet::Packet* Packet = nullptr;

	while (Packet = ThisState->Peer->Receive(), Packet) {
		switch (Packet->data[0])
		{
		case eINCOMINGSTRUCT:
		{
			PacketBase* IncomingPacket = (PacketBase*)&Packet->data[1];
			switch (IncomingPacket->ID)
			{
			case GetTypeGUID(GameModePacket):
			{
				size_t ClientIdx = GetPlayerIndex(ThisState, Packet->systemAddress);
				GameModePacket* ModePacket = (GameModePacket*)IncomingPacket;
				ThisState->ClientModes[ClientIdx] = ModePacket->Mode;
			}break;
			case GetTypeGUID(PlayerInputPacket):
			{
				size_t ClientIdx = GetPlayerIndex(ThisState, Packet->systemAddress);
				PlayerInputPacket* InputPacket = (PlayerInputPacket*)IncomingPacket;
				
				size_t FramesLost = InputPacket->Frame - ThisState->Game.LastFrameRecieved[ClientIdx];
				for (size_t I = 0; I < FramesLost; ++I)// Replicated last known Input State for missing InputFrames
				{
					auto LastFrameRecieved = ThisState->Game.BufferedInputs[ClientIdx].back();
					ThisState->Game.BufferedInputs[ClientIdx].push_back(LastFrameRecieved);
				}

				ThisState->Game.BufferedInputs[ClientIdx].push_back( 
				{	InputPacket->PlayerInput,
					InputPacket->Mouse, 
					InputPacket->Frame }, true);

				ThisState->Game.LastFrameRecieved[ClientIdx] = InputPacket->Frame;
			}	break;
			case GetTypeGUID(_PlayerInfoPacket_):
			{
				PlayerInfoPacket* InfoPacket = (PlayerInfoPacket*)IncomingPacket;
				for (auto I : ThisState->OpenConnections ) {
					if (I.ID == InfoPacket->PlayerID) {
						auto ClientIdx   = GetPlayerIndex(ThisState, I.Addr);
						auto Position    = GetPlayerPosition(ThisState->Game.Players[ClientIdx]);
						auto Orientation = GetOrientation(ThisState->Game.Players[ClientIdx]);

						bool updateClient = false;
						if (Orientation.dot(InfoPacket->R) > 0.02f)
							updateClient = true;
							SetPlayerOrientation(&ThisState->Game.Players[ClientIdx], InfoPacket->R);

						if ((Position - InfoPacket->POS).magnitude() > 1.0f)
							updateClient = true;

						if (updateClient) {
							SendSetPlayerPositionRotation(ThisState, I.Addr, Position, Orientation, InfoPacket->PlayerID);
						}

					}
				}
			}	break;
			}
		}	break;
		}
		ThisState->Peer->DeallocatePacket(Packet);
	}

	ThisState->Game.Update(ThisState->Base, dT);

	for(auto Client : ThisState->OpenConnections)
	{
		size_t Destination = GetPlayerIndex(ThisState, Client.Addr);

		for (auto OtherClient : ThisState->OpenConnections)
		{
			size_t Target = GetPlayerIndex(ThisState, OtherClient.Addr);
			if (Destination != Target) {
				auto Position = GetPlayerPosition(&ThisState->Game.Players[Target]);
				SendSetPlayerPositionRotation(ThisState, Client.Addr, 
					Position, 
					GetOrientation(&ThisState->Game.Players[Target]),
					ThisState->OpenConnections[Target].ID);
			}
		}
	}
}


/************************************************************************************************/


bool UpdateHost(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (HostState*)StateMemory;
	ThisState->T += dT;

	switch (ThisState->ServerMode)
	{
	case ServerMode::eSERVERLOBBYMODE:
		LobbyMode(ThisState, Engine, dT);
		break;
	case ServerMode::eCLIENTLOADWAIT:
		LoadWaitMode(ThisState, Engine, dT);
		break;
	case ServerMode::eGAMEINPROGRESS:
		GameInProgressMode(ThisState, Engine, dT);
		break;
	default:
		break;
	}

	return true;
}


/************************************************************************************************/


HostState* CreateHostState(EngineMemory* Engine, BaseState* Base)
{
	auto State = &Engine->BlockAllocator.allocate_aligned<HostState>();
	State->VTable.Update  = UpdateHost;
	State->VTable.Release = CloseServer;
	State->MinPlayerCount = 1;
	State->Base           = Base;
	State->PlayerCount	  = 0;
	State->Peer			  = RakNet::RakPeerInterface::GetInstance();
	
	RakNet::SocketDescriptor sd(gServerPort, nullptr);

	auto res = State->Peer->Startup(16, &sd, 1);

	State->Peer->SetMaximumIncomingConnections(16);
	State->ServerMode = ServerMode::eSERVERLOBBYMODE;

	return State;
}


/************************************************************************************************/
