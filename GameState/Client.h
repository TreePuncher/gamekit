/**********************************************************************

Copyright (c) 2019 Robert May

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
	short		port			= 1337;
	const char*	serverAddress	= "127.0.0.1"; // no place like home
	const char* name			= nullptr;
};


struct RemotePlayer
{
	MultiplayerPlayerID_t	id;
	char					name[32];
};


class ClientLobbyState : public FlexKit::FrameworkState
{
public:
	ClientLobbyState(
		GameFramework*		IN_framework,
		GameClientState*	IN_client,
		NetworkState*		IN_network,
		const char*			IN_localPlayerName);


	~ClientLobbyState();


	bool Update			(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT);
	bool Draw			(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph&);
	bool PostDrawUpdate	(FlexKit::EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph&);

	bool EventHandler	(FlexKit::Event evt) override;

	size_t					refreshCounter;
	Vector<PacketHandler*>	packetHandlers;
	NetworkState&			network;
	GameClientState&		client;
	MultiplayerLobbyScreen	screen;

	bool					ready;
	const char*				localPlayerName;
};


/************************************************************************************************/


class GameClientState : public FlexKit::FrameworkState
{
public:
	GameClientState(
		FlexKit::GameFramework* IN_framework,
		BaseState&				IN_base,
		NetworkState&			IN_network,
		ClientGameDescription	IN_desc = ClientGameDescription{}) :
			FrameworkState		{ IN_framework							},
			base				{ IN_base								},
			network				{ IN_network							},
			packetHandlers		{ IN_framework->core->GetBlockMemory()	},
			remotePlayers		{ IN_framework->core->GetBlockMemory()	}
	{
		char Address[256];
		if (!IN_desc.name)
		{
			std::cout << "Please Enter Name: \n";
			std::cin >> localName;
		}
		else 
			strncpy(localName, IN_desc.name, strnlen(IN_desc.name, 32) + 1);

		if (!IN_desc.serverAddress)
		{
			std::cout << "Please Address: \n";
			std::cin >> Address;
		}

		std::cout << "Connecting now\n";
        network.Connect("127.0.0.1", 1337);
	}


	~GameClientState()
	{
		for (auto handler : packetHandlers)
			framework->core->GetBlockMemory().free(handler);
	}


	void StartGame()
	{
		auto framework_temp = framework;
		framework_temp->PopState();
		//auto& predictedState		= framework_temp->PushState<GameState>();
		//auto& localState			= framework_temp->PushState<RemotePlayerState>();
	}


	void ConnectionSuccess()
	{
		FK_ASSERT(0);
		framework->PushState<ClientLobbyState>(this, &network, localName);
	}


	void ServerLost()
	{
		FK_ASSERT(0);
		framework->quit = true;
	}


	bool Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		// Handle Packets

		return true;
	}

	Vector<RemotePlayer>	remotePlayers;
	Vector<PacketHandler*>	packetHandlers;
	NetworkState&			network;
	BaseState&				base;

	MultiplayerPlayerID_t	localID;
	char					localName[32];
};


/************************************************************************************************/
