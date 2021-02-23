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
    HostWorldStateMangager(MultiplayerPlayerID_t IN_player, NetworkState& IN_net, BaseState& IN_base) :
        net                     { IN_net    },
        base                    { IN_base   },

        eventMap                { IN_base.framework.core.GetBlockMemory() },

        pscene                  { IN_base.physics.CreateScene() },
        gscene                  { IN_base.framework.core.GetBlockMemory() },

        localPlayerID           { IN_player },
        localPlayer             { CreateThirdPersonCameraController(pscene, IN_base.framework.core.GetBlockMemory()) },

        localPlayerComponent    { IN_base.framework.core.GetBlockMemory() },
        remotePlayerComponent   { IN_base.framework.core.GetBlockMemory() },

        packetHandlers          { IN_base.framework.core.GetBlockMemory() },

        floorCollider           { gameOjectPool.Allocate() },

        gameOjectPool           { IN_base.framework.core.GetBlockMemory(), 8096 },
        cubeShape               { IN_base.physics.CreateCubeShape({ 0.5f, 0.5f, 0.5f}) }
    {
        auto& allocator = IN_base.framework.core.GetBlockMemory();
        auto floorShape = base.physics.CreateCubeShape({ 100, 1, 100 });

        floorCollider.AddView<StaticBodyView>(pscene, floorShape, float3{0, -1.0f, 0});

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
        auto& allocator = base.framework.core.GetBlockMemory();

        localPlayer.Release();
        gscene.ClearScene();

    }

    WorldStateUpdate Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final
    {
        ProfileFunction();

        net.Update(core, dispatcher, dT);

        WorldStateUpdate out;

        struct HostWorldUpdate
        {

        };

        out.update =
            &dispatcher.Add<HostWorldUpdate>(
                [](UpdateDispatcher::UpdateBuilder& Builder, auto& data)
                {
                },
                [&, dT = dT](auto& data, iAllocator& threadAllocator)
                {
                    fixedUpdate(dT,
                        [&](auto dT)
                        {
                            currentInputState.mousedXY = base.renderWindow.mouseState.Normalized_dPos;
                            UpdatePlayerState(localPlayer, currentInputState, dT);


                            const PlayerFrameState localState = GetPlayerFrameState(localPlayer);

                            // Send Player Updates
                            for (auto& playerView : remotePlayerComponent)
                            {
                                const auto playerID = playerView.componentData.ID;
                                const auto state = playerView.componentData.GetFrameState();
                                const auto connection = playerView.componentData.connection;

                                SendFrameState(localPlayerID, localState, connection);

                                for (auto otherPlayer : remotePlayerComponent)
                                {
                                    const auto otherPID = otherPlayer.componentData.ID;
                                    const auto connection = otherPlayer.componentData.connection;

                                    if (playerID != otherPID)
                                        SendFrameState(playerID, state, connection);
                                }
                            }
                        });
                });

        auto& physicsUpdate = base.physics.Update(dispatcher, dT);
        out.update->AddOutput(physicsUpdate);

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
            [&](auto& evt) -> bool
            {
                return HandleEvents(currentInputState, evt);
            });

        return handled;
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


    void AddCube(float3 POS)
    {
        auto [triMesh, loaded]  = FindMesh(cube1X1X1);
        auto& renderSystem      = base.framework.GetRenderSystem();

        auto& gameObject = gameOjectPool.Allocate();

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), cube1X1X1);

        gameObject.AddView<RigidBodyView>(cubeShape, pscene, POS);

        gameObject.AddView<SceneNodeView<>>(GetRigidBodyNode(gameObject));
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
    const GUID_t                    cube1X1X1   = 7895;

    const PxShapeHandle cubeShape;

    FixedUpdate         fixedUpdate{ 60 };

    GraphicScene        gscene;
    PhysXSceneHandle    pscene;

    PlayerInputState    currentInputState;

    LocalPlayerComponent                localPlayerComponent;
    RemotePlayerComponent               remotePlayerComponent;

    CircularBuffer<ServerFrame, 240>	history;
    InputMap					        eventMap;

    ObjectPool<GameObject>              gameOjectPool;

    GameObject&                         localPlayer;
    GameObject&                         floorCollider;

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
