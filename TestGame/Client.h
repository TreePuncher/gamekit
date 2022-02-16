#pragma once

#include "MultiplayerState.h"
#include "MultiplayerGameState.h"
#include "Packets.h"
#include "LobbyGUI.h"


#include <regex>

/************************************************************************************************/


class ClientWorldStateMangager : public WorldStateMangagerInterface
{
public:
    ClientWorldStateMangager(ConnectionHandle IN_server, MultiplayerPlayerID_t IN_playerID, NetworkState& IN_net, BaseState& IN_base);
    ~ClientWorldStateMangager();


    void AddRemotePlayer(MultiplayerPlayerID_t playerID);
    void UpdateRemotePlayer(const PlayerFrameState& playerState, MultiplayerPlayerID_t player);

    WorldStateUpdate    Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final;
    Vector<UpdateTask*> DrawTasks(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final { return {}; }


    void SendFrameState(const MultiplayerPlayerID_t ID, const PlayerFrameState& state, const ConnectionHandle connection);
    bool EventHandler(Event evt) final;

    void SetOnGameEventRecieved(GameEventHandler handler)
    {
        gameEventHandler = handler;
    }

    void BroadcastEvent(static_vector<Event> events)
    {
        GameEventPacket packet;
        packet.events = events;

        for (auto& remotePlayer : remotePlayerComponent)
            net.Send(packet.header, remotePlayer.componentData.connection);
    }

    Scene&          GetScene() final;
    CameraHandle    GetActiveCamera() const final;

    GameObject&     CreateGameObject() final { return world.objectPool.Allocate(); };

    struct LocalFrame
    {
        float3              playerPosition;
        PlayerInputState    input;
    };

    Vector<PlayerFrameState>        pendingRemoteUpdates;

    const ConnectionHandle          server;
    const MultiplayerPlayerID_t     ID;

    GUID_t                          playerModel = 7896;

    FixedUpdate                     fixedUpdate{ 60 };

    GameWorld                       world;

    PlayerInputState                currentInputState;

    GadgetComponent                 gadgetComponent;
    PlayerComponent                 playerComponent;
    LocalPlayerComponent            localPlayerComponent;
    RemotePlayerComponent           remotePlayerComponent;

    GameObject&                     localPlayer;

    CircularBuffer<LocalFrame, 240>	history;
    InputMap					    eventMap;

    PacketHandlerVector             packetHandlers;

    GameEventHandler                gameEventHandler;

    Scene           scene;
    BaseState&      base;
    NetworkState&   net;
};


/************************************************************************************************/


class ClientState : public FrameworkState
{
public:
    ClientState(GameFramework& framework, MultiplayerPlayerID_t, ConnectionHandle handle, BaseState& IN_base, NetworkState& IN_net);

    void SendChatMessage(std::string msg);
    void RequestPlayerList();

    void SendGadgetUpdate(std::vector<GadgetInterface*>& spellbook);

    UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT) override;
    UpdateTask* Draw(UpdateTask* update, EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph)  override;

    void PostDrawUpdate(EngineCore&, double dT) override;


    struct Player
    {
        std::string           name;
        MultiplayerPlayerID_t ID;
    };

    std::function<void()>                OnPlayerListReceived   = []() {};
    std::function<void(std::string)>     OnMessageRecieved      = [](std::string msg) {};
    std::function<void(Player& player)>  OnPlayerJoin           = [](Player& player) {};
    std::function<void()>                OnGameStart            = []() {};

    std::vector<Player>         peers;
    const MultiplayerPlayerID_t clientID;
    ConnectionHandle            server;
    PacketHandlerVector         packetHandlers;

    NetworkState& net;
    BaseState& base;
};


void PushClientState(const MultiplayerPlayerID_t, const ConnectionHandle server, GameFramework&, BaseState&, NetworkState&);


/************************************************************************************************/
