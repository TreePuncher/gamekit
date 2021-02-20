#ifndef MENUSTATE_H
#define MENUSTATE_H

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


#include "GameFramework.h"
#include "CameraUtilities.h"
#include "BaseState.h"
#include "Packets.h"
#include "MultiplayerState.h"

using FlexKit::iAllocator;
using FlexKit::FrameworkState;
using FlexKit::GameFramework;
using FlexKit::EngineCore;
using FlexKit::Event;

class NetworkState;


/************************************************************************************************/


struct GameInfo
{
    std::string name;
    std::string lobbyName;

    ConnectionHandle connectionHandle = InvalidHandle_t;
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


/************************************************************************************************/


class ClientState : public FrameworkState
{
public:
    ClientState(GameFramework& framework, MultiplayerPlayerID_t, ConnectionHandle handle, BaseState& IN_base, NetworkState& IN_net);

    void SendChatMessage(std::string msg);
    void RequestPlayerList();

    UpdateTask* Update  (                       EngineCore&, UpdateDispatcher&, double dT) override;
    UpdateTask* Draw    (UpdateTask* update,    EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph)  override;

    void PostDrawUpdate (EngineCore&, double dT) override;

    struct Player
    {
        std::string           name;
        MultiplayerPlayerID_t ID;
    };

    std::function<void()>                OnPlayerListReceived    = [](){};
    std::function<void(std::string)>     OnMessageRecieved       = [](std::string msg) {};
    std::function<void(Player& player)>  OnPlayerJoin            = [](Player& player) {};
    std::function<void()>                OnGameStart             = []() {};



    std::vector<Player>         peers;
    const MultiplayerPlayerID_t clientID;
    ConnectionHandle            server;
    PacketHandlerVector         packetHandlers;

    NetworkState&               net;
    BaseState&                  base;
};


/************************************************************************************************/


struct WorldStateUpdate
{
    UpdateTask* update = nullptr;
};


class WorldStateMangagerInterface
{
public:
    virtual WorldStateUpdate    Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) = 0;
    virtual bool                EventHandler(Event evt) = 0;
    virtual CameraHandle        GetActiveCamera() const = 0;
    virtual GraphicScene&       GetScene() = 0;
};


struct PlayerInputState
{
    float forward   = 0;
    float backward  = 0;
    float left      = 0;
    float right     = 0;
    float up        = 0;
    float down      = 0;

    FlexKit::float2 mousedXY = { 0, 0 };
};


struct PlayerFrameState
{
    float3                      pos;
    float3                      velocity;
    Quaternion                  orientation;

    PlayerInputState            inputState;
    ThirdPersonCameraFrameState cameraState;
};


struct PlayerUpdatePacket
{
    UserPacketHeader        header{ sizeof(PlayerUpdatePacket), { PlayerUpdate } };

    MultiplayerPlayerID_t   playerID{ (uint64_t)-1 };
    PlayerFrameState        state{};
};

void HandleEvents(PlayerInputState& keyState, Event evt)
{
    if (evt.InputSource == FlexKit::Event::Keyboard)
    {
        auto state = evt.Action == Event::Pressed ? 1.0f : 0.0f;

        switch (evt.mData1.mINT[0])
        {
        case TPC_MoveForward:
            keyState.forward    = state;
            break;
        case TPC_MoveBackward:
            keyState.backward   = state;
            break;
        case TPC_MoveLeft:
            keyState.left       = state;
            break;
        case TPC_MoveRight:
            keyState.right      = state;
            break;
        case TPC_MoveUp:
            keyState.up         = state;
            break;
        case TPC_MoveDown:
            keyState.down       = state;
            break;
        }
    }
}


struct LocalPlayerData
{
    CircularBuffer<PlayerFrameState> inputHistory;
};


struct RemotePlayerData
{
    GameObject*             gameObject;
    ConnectionHandle        connection;
    MultiplayerPlayerID_t   ID;

    void Update(PlayerFrameState state)
    {
        SetWorldPosition(*gameObject, state.pos);
        SetOrientation(*gameObject, state.orientation);
    }

    PlayerFrameState    GetFrameState() const
    {
        PlayerFrameState out
        {
            .pos            = GetWorldPosition(*gameObject),
            .velocity       = { 0, 0, 0 },
            .orientation    = GetOrientation(*gameObject),
        };

        return out;
    }
};


inline static const ComponentID LocalPlayerComponentID  = GetTypeGUID(LocalPlayerData);

using LocalPlayerHandle     = Handle_t<32, LocalPlayerComponentID>;
using LocalPlayerComponent  = BasicComponent_t<LocalPlayerData, LocalPlayerHandle, LocalPlayerComponentID>;
using LocalPlayerView       = LocalPlayerComponent::View;

inline static const ComponentID RemotePlayerComponentID  = GetTypeGUID(RemotePlayerData);

using RemotePlayerHandle     = Handle_t<32, RemotePlayerComponentID>;
using RemotePlayerComponent  = BasicComponent_t<RemotePlayerData, RemotePlayerHandle, RemotePlayerComponentID>;
using RemotePlayerView       = RemotePlayerComponent::View;


PlayerFrameState GetPlayerFrameState(GameObject& gameObject)
{
    PlayerFrameState out;

    Apply(
        gameObject,
        [&](LocalPlayerView& view)
        {
            const float3        pos = GetCameraControllerHeadPosition(gameObject);
            const Quaternion    q   = GetCameraControllerOrientation(gameObject);

            out.pos         = pos;
            out.orientation = q;
        });

    return out;
}

RemotePlayerData* FindPlayer(MultiplayerPlayerID_t ID, RemotePlayerComponent& players)
{
    for (auto& player : players)
    {
        if (player.componentData.ID == ID)
            return &players[player.handle];
    }

    return nullptr;
}

class FixedUpdate
{
public:
    FixedUpdate(double IN_updateRate) : updateRate{ 1.0 / IN_updateRate } {}

    template<typename TY_FN> requires std::invocable<TY_FN, double>
    void operator() (double dT, TY_FN FN)
    {
        T += dT;

        while (T > updateRate) {
            FN(updateRate);

            T -= updateRate;
        }
    }

    double GetdT() const { return updateRate; }

    double updateRate   = 0.0;
    double T            = 0.0;
};


void UpdatePlayerState(GameObject& player, const PlayerInputState& currentInputState, double dT)
{
    Apply(player,
        [&](LocalPlayerView&        player,
            CameraControllerView&   camera)
        {
            const ThirdPersonCamera::KeyStates tpc_keyState =
            {
                .forward    = currentInputState.forward > 0.0,
                .backward   = currentInputState.backward > 0.0,
                .left       = currentInputState.left > 0.0,
                .right      = currentInputState.right > 0.0,
                .up         = currentInputState.up > 0.0,
                .down       = currentInputState.down > 0.0,
            };

            const auto cameraState  = camera.GetData().GetFrameState();

            player.GetData().inputHistory.push_back({ {}, {}, {}, currentInputState, cameraState });
            camera.GetData().Update(currentInputState.mousedXY, tpc_keyState, dT);
        });
}


class HostWorldStateMangager : public WorldStateMangagerInterface
{
public:
    HostWorldStateMangager(MultiplayerPlayerID_t IN_player, NetworkState& IN_net, BaseState& IN_base) :
        net                     { IN_net    },
        base                    { IN_base   },

        eventMap                { IN_base.framework.core.GetBlockMemory() },
        pscene                  { IN_base.physics.CreateScene() },

        localPlayerID           { IN_player },
        localPlayer             { CreateThirdPersonCameraController(pscene, IN_base.framework.core.GetBlockMemory()) },

        localPlayerComponent    { IN_base.framework.core.GetBlockMemory() },
        remotePlayerComponent   { IN_base.framework.core.GetBlockMemory() },

        packetHandlers          { IN_base.framework.core.GetBlockMemory() }
    {
        auto& allocator = IN_base.framework.core.GetBlockMemory();

        packetHandlers.push_back(
            CreatePacketHandler(
                PlayerUpdate,
                [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
                {
                    auto* updatePacket = reinterpret_cast<PlayerUpdatePacket*>(header);
                    UpdateRemotePlayer(updatePacket->state, updatePacket->playerID);
                },
                allocator));

        net.PushHandler(packetHandlers);

        eventMap.MapKeyToEvent(KEYCODES::KC_W, TPC_MoveForward);
        eventMap.MapKeyToEvent(KEYCODES::KC_S, TPC_MoveBackward);

        eventMap.MapKeyToEvent(KEYCODES::KC_A, TPC_MoveLeft);
        eventMap.MapKeyToEvent(KEYCODES::KC_D, TPC_MoveRight);
        eventMap.MapKeyToEvent(KEYCODES::KC_Q, TPC_MoveDown);
        eventMap.MapKeyToEvent(KEYCODES::KC_E, TPC_MoveUp);

        localPlayer.AddView<LocalPlayerView>();

        static const GUID_t sceneID = 1234;
        AddAssetFile("assets\\multiplayerAssets.gameres");

        FK_ASSERT(LoadScene(base.framework.core, gscene, sceneID));
    }

    ~HostWorldStateMangager()
    {
        localPlayer.Release();
        gscene.ClearScene();
    }

    WorldStateUpdate Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final
    {
        ProfileFunction();

        net.Update(core, dispatcher, dT);

        WorldStateUpdate out;

        fixedUpdate(dT,
            [&](auto dT)
            {
                currentInputState.mousedXY = base.renderWindow.mouseState.Normalized_dPos;
                UpdatePlayerState(localPlayer, currentInputState, dT);

                
                const PlayerFrameState localState = GetPlayerFrameState(localPlayer);

                // Send Player Updates
                for (auto& playerView : remotePlayerComponent)
                {
                    const auto playerID     = playerView.componentData.ID;
                    const auto state        = playerView.componentData.GetFrameState();
                    const auto connection   = playerView.componentData.connection;

                    SendFrameState(localPlayerID, localState, connection);

                    for (auto otherPlayer : remotePlayerComponent)
                    {
                        const auto otherPID     = otherPlayer.componentData.ID;
                        const auto connection   = otherPlayer.componentData.connection;

                        if (playerID != otherPID)
                            SendFrameState(playerID, state, connection);
                    }
                }
            });

        return out;
    }

    void SendFrameState(const MultiplayerPlayerID_t ID, const PlayerFrameState& state, const ConnectionHandle connection)
    {
        PlayerUpdatePacket packet;
        packet.playerID = ID;
        packet.state    = state;

        net.Send(packet.header, connection);
    }

    bool EventHandler(Event evt) final
    {
        ProfileFunction();

        bool handled =
            eventMap.Handle(evt,
            [&](auto& evt)
            {
                HandleEvents(currentInputState, evt);
            });

        return false;
    }

    GraphicScene& GetScene() final
    {
        return gscene;
    }


    CameraHandle GetActiveCamera() const final
    {
        return GetCameraControllerCamera(localPlayer);
    }

    void AddPlayer(ConnectionHandle connection, MultiplayerPlayerID_t ID)
    {
        auto& gameObject    = base.framework.core.GetBlockMemory().allocate<GameObject>();
        auto test           = RemotePlayerData{ &gameObject, connection, ID };
        auto playerView     = gameObject.AddView<RemotePlayerView>(test);


        auto [triMesh, loaded] = FindMesh(playerModel);

        auto& renderSystem = base.framework.GetRenderSystem();

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), playerModel);

        gameObject.AddView<SceneNodeView<>>();
        gameObject.AddView<DrawableView>(triMesh, GetSceneNode(gameObject));

        gscene.AddGameObject(gameObject, GetSceneNode(gameObject));

        SetBoundingSphereFromMesh(gameObject);
    }


    void UpdateRemotePlayer(const PlayerFrameState& playerState, MultiplayerPlayerID_t player)
    {
        auto playerView = FindPlayer(player, remotePlayerComponent);

        if (playerView)
            playerView->Update(playerState);
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
    const GUID_t                    playerModel = 7894;

    FixedUpdate         fixedUpdate{ 60 };

    GraphicScene        gscene;
    PhysXSceneHandle    pscene;

    PlayerInputState    currentInputState;

    LocalPlayerComponent                localPlayerComponent;
    RemotePlayerComponent               remotePlayerComponent;

    CircularBuffer<ServerFrame, 240>	history;
    InputMap					        eventMap;

    GameObject&                         localPlayer;

    PacketHandlerVector                 packetHandlers;

    BaseState&      base;
    NetworkState&   net;
};


class ClientWorldStateMangager : public WorldStateMangagerInterface
{
public:
    ClientWorldStateMangager(ConnectionHandle IN_server, MultiplayerPlayerID_t IN_playerID, NetworkState& IN_net, BaseState& IN_base) :
        net                     { IN_net    },
        base                    { IN_base   },

        ID                      { IN_playerID   },
        server                  { IN_server     },

        localPlayerComponent    { IN_base.framework.core.GetBlockMemory() },
        remotePlayerComponent   { IN_base.framework.core.GetBlockMemory() },
        eventMap                { IN_base.framework.core.GetBlockMemory() },
        packetHandlers          { IN_base.framework.core.GetBlockMemory() },

        pscene                  { IN_base.physics.CreateScene() },
        localPlayer             { CreateThirdPersonCameraController(pscene, IN_base.framework.core.GetBlockMemory()) }
    {
        auto& allocator = IN_base.framework.core.GetBlockMemory();

        packetHandlers.push_back(
            CreatePacketHandler(
                PlayerUpdate,
                [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
                {
                    auto* updatePacket = reinterpret_cast<PlayerUpdatePacket*>(header);
                    UpdateRemotePlayer(updatePacket->state, updatePacket->playerID);
                },
                allocator));

        net.PushHandler(packetHandlers);

        eventMap.MapKeyToEvent(KEYCODES::KC_W, TPC_MoveForward);
        eventMap.MapKeyToEvent(KEYCODES::KC_S, TPC_MoveBackward);

        eventMap.MapKeyToEvent(KEYCODES::KC_A, TPC_MoveLeft);
        eventMap.MapKeyToEvent(KEYCODES::KC_D, TPC_MoveRight);
        eventMap.MapKeyToEvent(KEYCODES::KC_Q, TPC_MoveDown);
        eventMap.MapKeyToEvent(KEYCODES::KC_E, TPC_MoveUp);

        localPlayer.AddView<LocalPlayerView>();

        static const GUID_t sceneID = 1234;
        AddAssetFile("assets\\multiplayerAssets.gameres");

        FK_ASSERT(LoadScene(base.framework.core, gscene, sceneID));
    }


    void AddRemotePlayer(MultiplayerPlayerID_t playerID)
    {
        auto& gameObject    = base.framework.core.GetBlockMemory().allocate<GameObject>();
        auto test           = RemotePlayerData{ &gameObject, InvalidHandle_t, playerID };
        auto playerView     = gameObject.AddView<RemotePlayerView>(test);


        auto [triMesh, loaded] = FindMesh(playerModel);

        auto& renderSystem = base.framework.GetRenderSystem();

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), playerModel);

        gameObject.AddView<SceneNodeView<>>();
        gameObject.AddView<DrawableView>(triMesh, GetSceneNode(gameObject));

        gscene.AddGameObject(gameObject, GetSceneNode(gameObject));

        SetBoundingSphereFromMesh(gameObject);
    }


    void UpdateRemotePlayer(const PlayerFrameState& playerState, MultiplayerPlayerID_t player)
    {
        auto playerView = FindPlayer(player, remotePlayerComponent);

        if (playerView)
            playerView->Update(playerState);
    }

    WorldStateUpdate Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final
    {
        ProfileFunction();

        net.Update(core, dispatcher, dT);

        WorldStateUpdate out;

        fixedUpdate(dT,
            [&](auto dT)
            {
                currentInputState.mousedXY = base.renderWindow.mouseState.Normalized_dPos;
                UpdatePlayerState(localPlayer, currentInputState, dT);

                const PlayerFrameState frameState = GetPlayerFrameState(localPlayer);

                SendFrameState(ID, frameState, server);
            });

        return out;
    }


    void SendFrameState(const MultiplayerPlayerID_t ID, const PlayerFrameState& state, const ConnectionHandle connection)
    {
        PlayerUpdatePacket packet;
        packet.playerID = ID;
        packet.state    = state;

        net.Send(packet.header, connection);
    }


    bool EventHandler(Event evt) final
    {
        ProfileFunction();

        bool handled =
            eventMap.Handle(evt,
                [&](auto& evt)
                {
                    HandleEvents(currentInputState, evt);
                });

        return false;
    }


    GraphicScene& GetScene() final
    {
        return gscene;
    }


    CameraHandle GetActiveCamera() const final
    {
        return GetCameraControllerCamera(localPlayer);
    }


    struct LocalFrame
    {
        float3              playerPosition;
        PlayerInputState    input;
    };

    const ConnectionHandle          server;
    const MultiplayerPlayerID_t     ID;

    GUID_t                          playerModel = 7894;

    FixedUpdate                     fixedUpdate{ 60 };

    GraphicScene                    gscene;
    PhysXSceneHandle                pscene;

    PlayerInputState                currentInputState;

    LocalPlayerComponent            localPlayerComponent;
    RemotePlayerComponent           remotePlayerComponent;

    GameObject&                     localPlayer;

    CircularBuffer<LocalFrame, 240>	history;
    InputMap					    eventMap;

    PacketHandlerVector             packetHandlers;

    GraphicScene    scene;
    BaseState&      base;
    NetworkState&   net;
};


/************************************************************************************************/


class LocalGameState : public FrameworkState
{
public:
    LocalGameState(GameFramework& IN_framework, WorldStateMangagerInterface& IN_worldState, BaseState& IN_base) :
        FrameworkState  { IN_framework  },
        worldState      { IN_worldState },
        base            { IN_base }
    {
        //base.renderWindow.EnableCaptureMouse(true);
    }


    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
    {
        ProfileFunction();

        base.Update(core, dispatcher, dT);

        auto tasks = worldState.Update(core, dispatcher, dT);

        return tasks.update;
    }


    UpdateTask* Draw(UpdateTask* updateTask, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
    {
        ProfileFunction();


        frameGraph.Resources.AddBackBuffer(base.renderWindow.GetBackBuffer());
        frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

        CameraHandle activeCamera = worldState.GetActiveCamera();
        SetCameraAspectRatio(activeCamera, base.renderWindow.GetAspectRatio());

        auto& scene             = worldState.GetScene();
        auto& transforms        = QueueTransformUpdateTask(dispatcher);
        auto& cameras           = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
        auto& cameraConstants   = MakeHeapCopy(Camera::ConstantBuffer{}, core.GetTempMemory());

        if (updateTask)
            transforms.AddInput(*updateTask);

        cameras.AddInput(transforms);

        WorldRender_Targets targets = {
            base.renderWindow.GetBackBuffer(),
            base.depthBuffer
        };

        ClearVertexBuffer   (frameGraph, base.vertexBuffer);
        ClearBackBuffer     (frameGraph, targets.RenderTarget, {0.0f, 0.0f, 0.0f, 0.0f});
        ClearDepthBuffer    (frameGraph, base.depthBuffer, 1.0f);

        auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
        auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

        if (base.renderWindow.GetWH().Product() != 0)
        {
            DrawSceneDescription sceneDesc =
            {
                .camera = activeCamera,
                .scene  = scene,
                .dt     = dT,
                .t      = base.t,

                .gbuffer    = base.gbuffer,
                .reserveVB  = reserveVB,
                .reserveCB  = reserveCB,

                .debugDisplay   = DebugVisMode::Disabled,
                .BVHVisMode     = BVHVisMode::BVH,
                .debugDrawMode  = ClusterDebugDrawMode::BVH,

                .transformDependency    = transforms,
                .cameraDependency       = cameras,
            };

            auto& drawnScene = base.render.DrawScene(dispatcher, frameGraph, sceneDesc, targets, core.GetBlockMemory(), core.GetTempMemoryMT());

            base.streamingEngine.TextureFeedbackPass(
                dispatcher,
                frameGraph,
                activeCamera,
                base.renderWindow.GetWH(),
                drawnScene.PVS,
                reserveCB,
                reserveVB);
        }

        PresentBackBuffer(frameGraph, base.renderWindow);

        return nullptr;
    }


    void PostDrawUpdate(EngineCore& core, double dT) override
    {
        ProfileFunction();

        base.PostDrawUpdate(core, dT);
    }


    bool EventHandler(Event evt) override
    {
        return worldState.EventHandler(evt);
    }


    BaseState&                      base;
    WorldStateMangagerInterface&    worldState;
};


/************************************************************************************************/

// Render Lobby and contain chat
class LobbyState : public FrameworkState
{
public:
    LobbyState(GameFramework& framework, BaseState& IN_base, NetworkState& IN_host) :
        FrameworkState  { framework },
        base            { IN_base   },
        net             { IN_host   }
    {
        memset(inputBuffer, '\0', 512);
        chatHistory += "Lobby Created\n";
    }



    UpdateTask* Update  (             EngineCore&, UpdateDispatcher&, double dT) override;
    UpdateTask* Draw    (UpdateTask*, EngineCore&, UpdateDispatcher&, double dT, FrameGraph&)  override;

    void PostDrawUpdate (EngineCore&, double dT) override;

    bool EventHandler   (Event evt) override;

    void MessageRecieved(std::string& msg)
    {
        chatHistory += msg + '\n';
    }

    bool                                            host = false;

    struct Player
    {
        std::string             Name;
        MultiplayerPlayerID_t   ID;
    };

    std::function<void   (std::string)>  OnSendMessage;
    std::function<Player (uint idx)>     GetPlayer       = [](uint idx){ return Player{}; };
    std::function<size_t ()>             GetPlayerCount  = []{ return 0;};

    std::function<void ()>               OnGameStart     = [] {};
    std::function<void ()>               OnReady         = [] {};
    
    std::vector<Player> players;
    std::string         chatHistory;
    BaseState&          base;
    NetworkState&       net;

    char inputBuffer[512];
};


/************************************************************************************************/


class MenuState : public FrameworkState
{
public:
    MenuState(GameFramework& framework, BaseState& IN_base, NetworkState& IN_net);

    UpdateTask* Update      (             EngineCore&, UpdateDispatcher&, double dT) override;
    UpdateTask* Draw        (UpdateTask*, EngineCore&, UpdateDispatcher&, double dT, FrameGraph&)  override;

    void PostDrawUpdate     (EngineCore&, double dT) override;

    bool EventHandler(Event evt) override;

    enum class MenuMode
    {
        MainMenu,
        Join,
        JoinInProgress,
        Host
    } mode = MenuMode::MainMenu;


    char name[128];
    char lobbyName[128];
    char server[128];

    double  connectionTimer = 0;

    PacketHandlerVector packetHandlers;

    NetworkState&       net;
    BaseState&          base;
};


/************************************************************************************************/

#endif
