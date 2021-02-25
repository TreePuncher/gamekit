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
        localPlayer             { CreatePlayer(
                                    PlayerDesc{
                                        .pscene = pscene,
                                        .gscene = gscene,
                                        .h      = 0.5f,
                                        .r      = 0.5f
                                    },
                                    IN_base.framework.core.RenderSystem,
                                    IN_base.framework.core.GetBlockMemory()) },

        floorCollider           { IN_base.framework.core.GetBlockMemory().allocate<GameObject>() }
    {
        auto& allocator = IN_base.framework.core.GetBlockMemory();
        auto floorShape = base.physics.CreateCubeShape({1000, 1, 1000});

        floorCollider.AddView<StaticBodyView>(pscene, floorShape);

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

        FK_ASSERT(LoadScene(base.framework.core, gscene, sceneID));

        auto& visibility = SceneVisibilityComponent::GetComponent();

        SetControllerPosition(localPlayer, { 30, 5, -30 });

        static std::regex pattern{"Cube"};
        for (auto& entity : gscene.sceneEntities)
        {
            auto& go    = *visibility[entity].entity;
            auto id     = GetStringID(go);

            if (id && std::regex_search(id, pattern))
            {
                auto meshHandle = GetTriMesh(go);
                auto mesh       = GetMeshResource(meshHandle);

                AABB aabb{
                    .Min = mesh->Info.Min,
                    .Max = mesh->Info.Max };

                const auto mipPoint = aabb.MidPoint();
                const auto dim      = aabb.Dim() / 2.0f;
                const auto pos      = GetWorldPosition(go);

                PxShapeHandle shape = IN_base.physics.CreateCubeShape(dim);

                go.AddView<StaticBodyView>(pscene, shape, pos);
            }
        }
    }

    ~ClientWorldStateMangager(){}


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

        struct WorldUpdate
        {

        };

        WorldStateUpdate out;
        out.update =
            &dispatcher.Add<WorldUpdate>(
                [](UpdateDispatcher::UpdateBuilder& Builder, auto& data)
                {
                },
                [&, dT = dT](auto& data, iAllocator& threadAllocator)
                {
                    fixedUpdate(dT,
                        [this, dT = dT](auto dT)
                        {
                            FK_LOG_INFO("Client World State Update");
                            currentInputState.mousedXY = base.renderWindow.mouseState.Normalized_dPos;
                            UpdatePlayerState(localPlayer, currentInputState, dT);

                            const PlayerFrameState frameState = GetPlayerFrameState(localPlayer);

                            SendFrameState(ID, frameState, server);
                        });
                }
            );

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

        return eventMap.Handle(evt,
                [&](auto& evt) -> bool
                {
                    return HandleEvents(currentInputState, evt);
                });
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

    GUID_t                          playerModel = 7896;

    FixedUpdate                     fixedUpdate{ 60 };

    GraphicScene                    gscene;
    PhysXSceneHandle                pscene;

    PlayerInputState                currentInputState;

    LocalPlayerComponent            localPlayerComponent;
    RemotePlayerComponent           remotePlayerComponent;

    GameObject&                     localPlayer;
    GameObject&                     floorCollider;


    CircularBuffer<LocalFrame, 240>	history;
    InputMap					    eventMap;

    PacketHandlerVector             packetHandlers;

    GraphicScene    scene;
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

    void SendSpellbookUpdate(std::vector<CardInterface*>& spellbook);

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
