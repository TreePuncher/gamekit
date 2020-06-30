#ifndef HOSTSTATE_H_INCLUDED
#define HOSTSTATE_H_INCLUDED


/************************************************************************************************/

#include "MultiplayerState.h"
#include "MultiplayerGameState.h"
#include "Packets.h"
#include "LobbyGUI.h"


class GameHostState;


/************************************************************************************************/


class GameHostLobbyState : public FlexKit::FrameworkState
{

public:
	GameHostLobbyState(
		GameFramework&  IN_framework,
        BaseState&      IN_base,
		GameHostState&  IN_host);

	~GameHostLobbyState();

	void AddLocalPlayer(const MultiplayerPlayerID_t);
	void HandleNewConnection(const ConnectionHandle);
    void HandleDisconnection(const ConnectionHandle);

	void Update			(EngineCore&, UpdateDispatcher&, double dT) final override;
	void Draw			(EngineCore&, UpdateDispatcher&, double dT, FrameGraph&) final override;
	void PostDrawUpdate	(EngineCore&, UpdateDispatcher&, double dT, FrameGraph&) final override;

	bool EventHandler(Event evt) final override;


	struct PlayerLobbyEntry
	{
		MultiplayerPlayerID_t	ID;
		bool					Ready;
	};

	std::pair<PlayerLobbyEntry, bool> GetPlayerLobbyState(const MultiplayerPlayerID_t id)
	{
		for (auto& player : playerLobbyState)
			if(player.ID == id)
			    return { player, true };

		return { {}, false };
	}

private:


	Vector<PacketHandler*>		packetHandlers;
	Vector<PlayerLobbyEntry>	playerLobbyState;

	bool					localHostReady = false;
    BaseState&              base;
	GameHostState&			host;
	MultiplayerLobbyScreen	screen;
};


/************************************************************************************************/


enum PlayerNetState : uint32_t
{
	Joining,
	Lobby,
	StartingGame,
    LoadingScreen,
	InGame
};


MultiplayerPlayerID_t GeneratePlayerID();


struct MultiPlayerEntry
{
	bool									local;
	PlayerNetState							state;
	MultiplayerPlayerID_t					ID;
    ConnectionHandle                        address;
	char									name[32];
};


/************************************************************************************************/


class GameHostState : public FlexKit::FrameworkState
{
public:
	GameHostState(
		GameFramework&          IN_framework,
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

		auto& lobby = framework.PushState<GameHostLobbyState>(base, *this);
		lobby.AddLocalPlayer(hostPlayer);

		return *this;
	}


    void RemovePlayer(const MultiplayerPlayerID_t id)
    {
        players.remove_unstable(std::find_if(std::begin(players), std::end(players),
            [&](auto player) { return player.ID == id; }));
    }


    MultiPlayerEntry* GetPlayer(const ConnectionHandle handle)
    {
        auto res = std::find_if(std::begin(players), std::end(players),
            [&](auto player) { return player.address == handle; });
        
        return (res == players.end()) ? nullptr : res;
    }


	MultiPlayerEntry* GetPlayer(const MultiplayerPlayerID_t id)
	{
        auto res = std::find_if(std::begin(players), std::end(players),
            [&](auto player) { return player.ID == id; });

        return (res == players.end()) ? nullptr : res;
	}


	void BeginGame()
	{
        FK_LOG_INFO("Starting Game!");

        // Send Start Game Packets
        network.Broadcast(StartGamePacket{});

		framework.PopState();

		auto& gameState = framework.PushState<GameState>(base);

        auto PlayersStillLoading = [&]() -> bool
        {
            auto pred = [](auto& player) -> bool { return player.state == LoadingScreen; };
            return std::find_if(std::begin(players), std::end(players), pred) != players.end();
        };

        auto OnCompletion = [&, PlayersStillLoading](auto& core, auto& Dispatcher, double dT)
        {
            if (PlayersStillLoading())
            {
                auto& WaitState = framework.PushState<LocalPlayerState>(base, gameState);
                WaitState.Update(core, Dispatcher, dT);
            }
            else
            {
                auto& localState = framework.PushState<LocalPlayerState>(base, gameState);

                // if we don't update, these states will miss their first frame update
                localState.Update(core, Dispatcher, dT);
            }
        };

        FK_ASSERT(0);

        //auto& localState = framework.PushState<GameLoadSceneState<decltype(OnCompletion)>> (base, gameState.scene, "MultiplayerTestLevel", OnCompletion);
	}


	void SetPlayerName(const MultiplayerPlayerID_t id, const char* name, const size_t namelen)
	{
		auto player = GetPlayer(id);
		strncpy(
			player->name, 
			name, 
			min(namelen, sizeof(MultiPlayerEntry::name) - 1)); // one less to account for null character
	}


	void SetState(const MultiplayerPlayerID_t playerID, const PlayerNetState netState)
	{
		if (auto player = GetPlayer(playerID); player)
			player->state = netState;
	}


	void Update(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final override
	{
	}


	MultiplayerPlayerID_t GetNewID()
	{
        while (true)
        {
            auto newID = GeneratePlayerID();

            if (std::find_if(std::begin(players), std::end(players),
                    [&](auto& player) -> bool { return player.ID == newID; }) == players.end() )
                return newID;
		}
	}


    MultiplayerPlayerID_t               hostPlayer;
	static_vector<MultiPlayerEntry>		players;
	BaseState&							base;
	NetworkState&						network;
};


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


#endif
