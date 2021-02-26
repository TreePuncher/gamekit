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


    void AddPlayer(ConnectionHandle connection, MultiplayerPlayerID_t ID);
    void AddCube(float3 POS);

    GameObject& CreateSpell(SpellData spell, float3 InitialPosition, float3 InitialVelocity);

    void UpdateRemotePlayer(const PlayerFrameState& playerState, MultiplayerPlayerID_t player);


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
    const GUID_t                    playerModel = 7896;
    const GUID_t                    cube1X1X1   = 7895;
    const GUID_t                    spellModel  = 7894;

    const PxShapeHandle cubeShape;

    FixedUpdate         fixedUpdate{ 60 };

    GraphicScene        gscene;
    PhysXSceneHandle    pscene;

    PlayerInputState    currentInputState;

    SpellComponent                      spellComponent;
    LocalPlayerComponent                localPlayerComponent;
    RemotePlayerComponent               remotePlayerComponent;

    CircularBuffer<ServerFrame, 240>	history;
    InputMap					        eventMap;

    ObjectPool<GameObject>              gameOjectPool;

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
