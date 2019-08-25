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
		GameHostState&			IN_host);

	~GameHostLobbyState();

	void AddLocalPlayer(MultiplayerPlayerID_t ID);
	void HandleNewConnection(ConnectionHandle handle);
    void HandleDisconnection(ConnectionHandle handle);


	bool Update			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) override;
	bool Draw			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph) override;
	bool PostDrawUpdate	(EngineCore* Core,	 UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph) override;

	bool EventHandler(FlexKit::Event evt) override;


	struct PlayerLobbyEntry
	{
		MultiplayerPlayerID_t	ID;
		bool					Ready;
	};

	std::pair<PlayerLobbyEntry, bool> GetPlayerLobbyState(MultiplayerPlayerID_t id)
	{
		for (auto& player : playerLobbyState)
			if(player.ID == id)
			return { player, true };

		return { {}, false };
	}

private:


	FlexKit::Vector<PacketHandler*>		packetHandlers;
	FlexKit::Vector<PlayerLobbyEntry>	playerLobbyState;

	bool					localHostReady = false;
	GameHostState&			host;
	MultiplayerLobbyScreen	screen;
};


/************************************************************************************************/


enum PlayerNetState : uint32_t
{
	Joining,
	Lobby,
	StartingGame,
	InGame
};


MultiplayerPlayerID_t GeneratePlayerID();


struct MultiPlayerEntry
{
	bool									Local;
	PlayerNetState							State;
	MultiplayerPlayerID_t					PlayerID;
    ConnectionHandle                        Address;
	char									Name[32];
};


/************************************************************************************************/


class GameHostState : public FlexKit::FrameworkState
{
public:
	GameHostState(
		FlexKit::GameFramework* IN_framework,
		BaseState&				IN_base,
		NetworkState&			IN_Network,
		GameDescription			IN_GameDesc = GameDescription{}) :
			FrameworkState{ IN_framework					},
			network	{ IN_Network							},
			base	{ IN_base								}
	{
        network.Startup(1337);
		StartLobby();
	}


	~GameHostState(){}


	GameHostState& StartLobby()
	{
        hostPlayer = GeneratePlayerID();
		players.push_back({
			true,
			Lobby,
            hostPlayer,
			InvalidHandle_t,
			"LocalPlayer" });


		char name[32];
		std::cout << "Please enter player name: ";
		std::cin >> name;
		std::cout << "\n";
		SetPlayerName(hostPlayer, name, strnlen(name, 32) + 1);

		auto& lobby = framework->PushState<GameHostLobbyState>(*this);
		lobby.AddLocalPlayer(hostPlayer);

		return *this;
	}


    void RemovePlayer(MultiplayerPlayerID_t id)
    {
        for (auto& player : players)
        {
            if (player.PlayerID == id)
            {
                player = players.back();
                players.pop_back();
                return;
            }
        }
    }


    MultiPlayerEntry* GetPlayer(ConnectionHandle handle)
    {
        for (auto& player : players)
            if (player.Address == handle)
                return &player;

        return nullptr;
    }


	MultiPlayerEntry* GetPlayer(MultiplayerPlayerID_t id)
	{
		for (auto& player : players)
			if (player.PlayerID == id)
				return &player;

		return nullptr;
	}


	void BeginGame()
	{
		framework->PopState();

		FK_ASSERT(0);
		//auto& gameState = framework->PushState<GameState>(base);
		//framework->PushState<LocalPlayerState>(gameState);
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


    MultiplayerPlayerID_t               hostPlayer;
	static_vector<MultiPlayerEntry>		players;
	BaseState&							base;
	NetworkState&						network;
};


/************************************************************************************************/

#endif
