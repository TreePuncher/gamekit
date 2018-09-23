/**********************************************************************

Copyright (c) 2018 Robert May

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

#include "MultiplayerState.h"
#include "Packets.h"
#include "LobbyGUI.h"


/************************************************************************************************/

class GameClientState;


struct ClientGameDescription
{
	short	port			= 1337;
	char*	serverAddress	= "127.0.0.1"; // no place like home
};


class ClientLobbyState : public FlexKit::FrameworkState
{
public:
	ClientLobbyState(
		FlexKit::GameFramework* IN_framework,
		GameClientState*		IN_client,
		NetworkState*			IN_network,
		char*					IN_localPlayerName);


	~ClientLobbyState();


	bool Update			(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT);
	bool Draw			(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph&);
	bool PostDrawUpdate	(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph&);

	bool EventHandler	(FlexKit::Event evt) override;

	size_t					refreshCounter;

	struct RemotePlayer
	{
		MultiplayerPlayerID_t	id;
		char					name[32];
	};

	Vector<RemotePlayer>	remotePlayers;
	Vector<PacketHandler*>	packetHandlers;
	NetworkState*			network;
	GameClientState*		client;
	MultiplayerLobbyScreen	screen;

	bool					ready;
	char*					localPlayerName;
};


/************************************************************************************************/


class GameClientState : public FlexKit::FrameworkState
{
public:
	GameClientState(
		FlexKit::GameFramework* IN_framework,
		BaseState*				IN_base,
		NetworkState*			IN_network,
		ClientGameDescription	IN_desc = ClientGameDescription{}) :
			FrameworkState		{ IN_framework },
			network				{ IN_network },
			packetHandlers		{ IN_framework->Core->GetBlockMemory() },
			base				{ IN_base }
	{
		char	Address[256];
		std::cout << "Please Enter Name: \n";
		std::cin >> localName;
		std::cout << "Please Address: \n";
		std::cin >> Address;
		std::cout << "Connecting now\n";

		RakNet::SocketDescriptor desc;

		network->localPeer->Startup(1, &desc, 1);
		RakNet::ConnectionAttemptResult res = network->localPeer->Connect(Address, 1337, nullptr, 0);
		network->ConnectionAcceptedHandler	= [&](auto packet) {ConnectionSuccess(packet); };
		network->DisconnectHandler			= [&](auto packet) {ServerLost(packet); };
	}


	~GameClientState()
	{
		for (auto handler : packetHandlers)
			Framework->Core->GetBlockMemory().free(handler);
	}


	void ConnectionSuccess(RakNet::Packet* packet)
	{
		ServerAddress = packet->systemAddress;
		Framework->PushState<ClientLobbyState>(this, network, localName);
	}


	void ServerLost(RakNet::Packet* packet)
	{
		Framework->Quit = true;
	}


	bool Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		// Handle Packets

		return true;
	}

	RakNet::SystemAddress	ServerAddress;
	Vector<PacketHandler*>	packetHandlers;
	NetworkState*			network;
	BaseState*				base;

	MultiplayerPlayerID_t	localID;
	char					localName[32];
};


/************************************************************************************************/