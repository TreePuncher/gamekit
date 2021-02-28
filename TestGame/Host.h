#ifndef HOSTSTATE_H_INCLUDED
#define HOSTSTATE_H_INCLUDED


/************************************************************************************************/


#include "MultiplayerState.h"
#include "MultiplayerGameState.h"
#include "Packets.h"
#include "LobbyGUI.h"


/************************************************************************************************/


enum PlayerNetState : uint32_t
{
    Joining,
    Lobby,
    StartingGame,
    LoadingScreen,
    InGame
};


class HostWorldStateMangager : public WorldStateMangagerInterface
{
public:
    HostWorldStateMangager(MultiplayerPlayerID_t IN_player, NetworkState& IN_net, BaseState& IN_base);

    ~HostWorldStateMangager();

    WorldStateUpdate Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final;

    void SendFrameState(const MultiplayerPlayerID_t ID, const PlayerFrameState& state, const ConnectionHandle connection);
    bool EventHandler(Event evt) final;

    GraphicScene&   GetScene() final;
    CameraHandle    GetActiveCamera() const final;

    void SetOnGameEventRecieved(GameEventHandler handler) final
    {
        gameEventHandler = handler;
    }

    void AddPlayer(ConnectionHandle connection, MultiplayerPlayerID_t ID);

    void BroadcastEvent(static_vector<Event> events)
    {
        GameEventPacket packet;
        packet.events = events;

        for (auto& remotePlayer : remotePlayerComponent)
            net.Send(packet.header, remotePlayer.componentData.connection);
    }

    struct PlayerState
    {
        float3              position;
        PlayerInputState    input;
    };

    struct ServerFrame
    {
        Vector<PlayerState> players;
    };

    const MultiplayerPlayerID_t     localPlayerID;

    FixedUpdate         fixedUpdate{ 60 };

    GameWorld           world;

    PlayerInputState            currentInputState;
    Vector<PlayerFrameState>    pendingRemoteUpdates;

    SpellComponent                      spellComponent;
    PlayerComponent                     playerComponent;        // players game state such as health, player deck, etc
    LocalPlayerComponent                localPlayerComponent;   // Handles local tasks such as input, and local state history
    RemotePlayerComponent               remotePlayerComponent;  // Updates remote player proxies

    GameEventHandler                    gameEventHandler;

    CircularBuffer<ServerFrame, 240>	history;
    InputMap					        eventMap;

    GameObject&                         localPlayer;

    PacketHandlerVector                 packetHandlers;

    BaseState&      base;
    NetworkState&   net;
};


/************************************************************************************************/


// Manage host network state
class HostState : public FrameworkState
{
public:
    HostState(GameFramework& framework, GameInfo IN_info, BaseState& IN_base, NetworkState& IN_net);

    void HandleIncomingConnection(ConnectionHandle connectionhandle);
    void HandleDisconnection(ConnectionHandle connection);

    void BroadCastMessage(std::string);
    void SendPlayerList(ConnectionHandle connection);
    void SendGameStart();

    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override;


    struct Player
    {
        std::string             name;
        MultiplayerPlayerID_t   ID;
        ConnectionHandle        connection;
    };


    MultiplayerPlayerID_t   GetNewID() const;
    HostState::Player*      GetPlayer(MultiplayerPlayerID_t);

    std::vector<Player> players;

    std::function<void (std::string)>       OnMessageRecieved   = [](std::string msg) {};
    std::function<void (Player& player)>    OnPlayerJoin        = [](Player& player) {};

    Vector<PacketHandler*>  handler;

    MultiplayerPlayerID_t   hostID;

    GameInfo        info;
    NetworkState&   net;
    BaseState&      base;
};


void PushHostState(const GameInfo&, GameFramework&, BaseState&, NetworkState&);


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 Robert May

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
