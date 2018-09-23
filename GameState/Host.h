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

#ifndef HOSTSTATE_H_INCLUDED
#define HOSTSTATE_H_INCLUDED


/************************************************************************************************/

#include "MultiplayerState.h"
#include "Packets.h"
#include "LobbyGUI.h"


class GameHostState;


/************************************************************************************************/


class GameHostLobbyState : public FlexKit::FrameworkState
{
public:
	GameHostLobbyState(
		FlexKit::GameFramework* IN_framework,
		GameHostState*			IN_host);

	~GameHostLobbyState();

	void HandleNewConnection(RakNet::Packet* packet);

	bool Update			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) override;
	bool Draw			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph) override;
	bool PostDrawUpdate	(EngineCore* Core,	 UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph) override;

private:


	struct PlayerLobbyEntry
	{
		MultiplayerPlayerID_t	ID;
		bool					Ready;
	};

	Vector<PacketHandler*>		packetHandlers;
	Vector<PlayerLobbyEntry>	playerLobbyState;

	GameHostState*			host;
	MultiplayerLobbyScreen	screen;
};


/************************************************************************************************/


enum PlayerNetState : uint32_t
{
	Joining,
	Lobby
};


MultiplayerPlayerID_t GeneratePlayerID();


struct MultiPlayerEntry
{
	bool									Local;
	PlayerNetState							State;
	MultiplayerPlayerID_t					PlayerID;
	RakNet::SystemAddress					Address;
	char									Name[32];
};


/************************************************************************************************/


class GameHostState : public FlexKit::FrameworkState
{
public:
	GameHostState(
		FlexKit::GameFramework* IN_framework,
		BaseState*				IN_base,
		NetworkState*			IN_Network,
		GameDescription			IN_GameDesc = GameDescription{}) :
		FrameworkState{ IN_framework },
		players{ IN_framework->Core->GetBlockMemory() },
		network{ IN_Network },
		base{ IN_base }
	{
		RakNet::SocketDescriptor sd(IN_GameDesc.Port, 0);
		network->localPeer->Startup(IN_GameDesc.MaxPlayerCount, &sd, 1);
	}


	~GameHostState()
	{}

	GameHostState& InitiateGame()
	{
		Framework->PushState<GameHostLobbyState>(this);

		players.push_back({
			true,
			Lobby,
			GeneratePlayerID(),
			RakNet::SystemAddress{},
			"LocalPlayer" });

		return *this;
	}


	MultiPlayerEntry* GetPlayer(MultiplayerPlayerID_t id)
	{
		for (auto& player : players)
			if (player.PlayerID == id)
				return &player;

		return nullptr;
	}


	void SetPlayerName(MultiplayerPlayerID_t id, char* name, size_t namelen)
	{
		auto player = GetPlayer(id);
		strncpy(
			player->Name, 
			name, 
			min(namelen, sizeof(MultiPlayerEntry::Name) - 1)); // one less to account for null character
	}


	void SetState(MultiplayerPlayerID_t playerID, PlayerNetState netState)
	{
		auto player = GetPlayer(playerID);
		if (player)
			player->State = netState;
	}


	bool Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) override
	{
		return true;
	}


	MultiplayerPlayerID_t GetNewID()
	{
		while (true)
		{
			auto NewID = GeneratePlayerID();
			for (auto& player : players)
				if (player.PlayerID == NewID)
					continue;
				else
					return NewID;
		}
	}


	Vector<MultiPlayerEntry>	players;
	BaseState*					base;
	NetworkState*				network;
};


/************************************************************************************************/

#endif