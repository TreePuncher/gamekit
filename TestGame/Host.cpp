#include "Host.h"
#include <random>
#include <limits>
#include <regex>


using FlexKit::GameFramework;


/************************************************************************************************/


HostWorldStateMangager::HostWorldStateMangager(MultiplayerPlayerID_t IN_player, NetworkState& IN_net, BaseState& IN_base) :
    net                     { IN_net    },
    base                    { IN_base   },

    eventMap                { IN_base.framework.core.GetBlockMemory() },

    pendingRemoteUpdates    { IN_base.framework.core.GetBlockMemory() },

    localPlayerID           { IN_player },
    localPlayer             { world.AddLocalPlayer(IN_player) },

    gadgetComponent         { IN_base.framework.core.GetBlockMemory() },
    playerComponent         { IN_base.framework.core.GetBlockMemory() },
    localPlayerComponent    { IN_base.framework.core.GetBlockMemory() },
    remotePlayerComponent   { IN_base.framework.core.GetBlockMemory() },

    packetHandlers          { IN_base.framework.core.GetBlockMemory() },
    world                   { IN_base.framework.core }
{
    auto& allocator = IN_base.framework.core.GetBlockMemory();

    packetHandlers.push_back(
        CreatePacketHandler(
            PlayerUpdate,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                auto* updatePacket = reinterpret_cast<PlayerUpdatePacket*>(header);

                pendingRemoteUpdates.push_back(updatePacket->state);
            },
            allocator));

    net.PushHandler(packetHandlers);

    eventMap.MapKeyToEvent(KEYCODES::KC_W, TPC_MoveForward);
    eventMap.MapKeyToEvent(KEYCODES::KC_S, TPC_MoveBackward);

    eventMap.MapKeyToEvent(KEYCODES::KC_A, TPC_MoveLeft);
    eventMap.MapKeyToEvent(KEYCODES::KC_D, TPC_MoveRight);
    eventMap.MapKeyToEvent(KEYCODES::KC_Q, TPC_MoveDown);
    eventMap.MapKeyToEvent(KEYCODES::KC_E, TPC_MoveUp);

    eventMap.MapKeyToEvent(KEYCODES::KC_1, PLAYER_ACTION1);
    eventMap.MapKeyToEvent(KEYCODES::KC_2, PLAYER_ACTION2);
    eventMap.MapKeyToEvent(KEYCODES::KC_3, PLAYER_ACTION3);
    eventMap.MapKeyToEvent(KEYCODES::KC_4, PLAYER_ACTION4);

    SetControllerPosition(localPlayer, { -30, 5, -30 });

    CreateMultiplayerScene(world);
}


/************************************************************************************************/


HostWorldStateMangager::~HostWorldStateMangager()
{
    auto& allocator = base.framework.core.GetBlockMemory();

    for (auto handler : packetHandlers)
        allocator.release_allocation(*handler);

    packetHandlers.clear();

    localPlayer.Release();
    world.Release();
}


/************************************************************************************************/


WorldStateUpdate HostWorldStateMangager::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    ProfileFunction();

    net.Update(core, dispatcher, dT);

    WorldStateUpdate out;

    struct _ {};

    auto& worldUpdate =
        dispatcher.Add<_>(
            [](UpdateDispatcher::UpdateBuilder& Builder, auto& data)
            {
            },
            [&, dT = dT](auto& data, iAllocator& threadAllocator)
            {
                ProfileFunction();

                fixedUpdate(dT,
                    [&](auto dT)
                    {
                        ProfileFunction();

                        // Send Player Updates
                        PlayerFrameState localState = GetPlayerFrameState(localPlayer);
                        currentInputState.mousedXY  = base.renderWindow.mouseState.Normalized_dPos;
                        localState.inputState       = currentInputState;

                        UpdateLocalPlayer(localPlayer, currentInputState, dT);
                        world.UpdatePlayer(localState, dT);

                        for (auto& playerUpdate : pendingRemoteUpdates)
                            world.UpdateRemotePlayer(playerUpdate, dT);

                        // Process Player events
                        for (auto& player : playerComponent)
                        {
                            for (auto& evt : player.componentData.playerEvents)
                            {
                                switch (evt.mData1.mINT[0])
                                {
                                case (int)PlayerEvents::PlayerDeath:
                                {
                                    static_vector<Event> events{ evt };
                                    BroadcastEvent(events);

                                    gameEventHandler(evt); // broadcast evt globally
                                }   break;
                                }
                            }

                            player.componentData.playerEvents.clear();
                        }
                        

                        for (auto& playerView : remotePlayerComponent)
                        {
                            const auto playerID     = playerView.componentData.playerID;
                            const auto state        = playerView.componentData.GetFrameState();
                            const auto connection   = playerView.componentData.connection;

                            SendFrameState(localPlayerID, localState, connection);

                            for (auto otherPlayer : remotePlayerComponent)
                            {
                                const auto otherPID     = otherPlayer.componentData.playerID;
                                const auto connection   = otherPlayer.componentData.connection;

                                if (playerID != otherPID)
                                    SendFrameState(playerID, state, connection);
                            }
                        }

                        pendingRemoteUpdates.clear();
                        currentInputState.events.clear();
                    });
            });

    auto& physicsUpdate     = base.physics.Update(dispatcher, dT);
    auto& spellUpdate       = world.UpdateGadgets(dispatcher, world.objectPool, dT);

    spellUpdate.AddInput(worldUpdate);
    physicsUpdate.AddInput(worldUpdate);
    physicsUpdate.AddInput(spellUpdate);

    out.update = &physicsUpdate;

    return out;
}


/************************************************************************************************/


Vector<UpdateTask*> HostWorldStateMangager::DrawTasks(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    Vector<UpdateTask*> tasks{ core.GetTempMemory() };

    return tasks;
}


/************************************************************************************************/


void HostWorldStateMangager::SendFrameState(const MultiplayerPlayerID_t ID, const PlayerFrameState& state, const ConnectionHandle connection)
{
    PlayerUpdatePacket packet;
    packet.playerID = ID;
    packet.state    = state;

    net.Send(packet.header, connection);
}


/************************************************************************************************/


bool HostWorldStateMangager::EventHandler(Event evt)
{
    ProfileFunction();

    return
        eventMap.Handle(evt,
        [&](auto& evt) -> bool
        {
            return HandleEvents(currentInputState, evt);
        });
}


/************************************************************************************************/


Scene& HostWorldStateMangager::GetScene()
{
    return world.scene;
}


/************************************************************************************************/


CameraHandle HostWorldStateMangager::GetActiveCamera() const 
{
    return GetCameraControllerCamera(localPlayer);
}


/************************************************************************************************/


void HostWorldStateMangager::AddPlayer(ConnectionHandle connection, MultiplayerPlayerID_t ID)
{
    auto& remotePlayer = world.AddRemotePlayer(ID);

    Apply(
        remotePlayer,
        [&](RemotePlayerView& remote)
        {
            remote.GetData().connection = connection;
        });
}


/************************************************************************************************/


HostState::HostState(GameFramework& framework, GameInfo IN_info, BaseState& IN_base, NetworkState& IN_net) :
    FrameworkState  { framework },
    net             { IN_net },
    base            { IN_base },
    info            { IN_info },
    handler         { framework.core.GetBlockMemory() },
    hostID          { GeneratePlayerID() }
{
    net.HandleNewConnection = [&](ConnectionHandle connection) { HandleIncomingConnection(connection); };
    net.HandleDisconnection = [&](ConnectionHandle connection) { HandleDisconnection(connection); };

    Player host{
        .name = info.name,
        .ID   = hostID
    };

    players.push_back(host);

    // Register Packet Handlers
    handler.push_back(
        CreatePacketHandler(
            ClientDataRequestResponse,
            [&](UserPacketHeader* incomingPacket, Packet* packet, NetworkState* network)
            {
                const ClientDataPacket* clientData = reinterpret_cast<ClientDataPacket*>(incomingPacket);

                const std::string           name        { clientData->playerName };
                const PlayerJoinEventPacket joinEvent   { clientData->playerID };

                for (auto& player : players)
                {
                    if (clientData->playerID == player.ID)
                        player.name = name;
                    else
                        net.Send(joinEvent.Header, player.connection);
                }
            },
            framework.core.GetBlockMemory()));

    handler.push_back(
        CreatePacketHandler(
            LobbyMessage,
            [&](UserPacketHeader* incomingPacket, Packet* packet, NetworkState* network)
            {
                LobbyMessagePacket* msgPacket = reinterpret_cast<LobbyMessagePacket*>(incomingPacket);

                std::string msg = msgPacket->message;

                for (auto& player : players)
                {
                    if (player.ID != msgPacket->playerID)
                    {
                        LobbyMessagePacket outMessage{ player.ID, msg };
                        net.Send(outMessage.Header, player.connection);
                    }
                }

                if (OnMessageRecieved)
                    OnMessageRecieved(msg);
            },
            framework.core.GetBlockMemory()));

    handler.push_back(
        CreatePacketHandler(
            RequestPlayerListPacket::PacketID(),
            [&](UserPacketHeader* incomingPacket, Packet* packet, NetworkState* network)
            {
                RequestPlayerListPacket* request = reinterpret_cast<RequestPlayerListPacket*>(incomingPacket);

                auto player = GetPlayer(request->playerID);

                if(player)
                    SendPlayerList(player->connection);
            },
            framework.core.GetBlockMemory()));

    net.PushHandler(handler);
    net.Startup(1337);
}


/************************************************************************************************/


HostState::~HostState()
{
    for (auto h : handler)
        framework.core.GetBlockMemory().release_allocation(*h);

    handler.clear();
}


/************************************************************************************************/


void HostState::HandleIncomingConnection(ConnectionHandle connectionhandle)
{
    Player host{
        .name       = "Incoming",
        .ID         = GetNewID(),
        .connection = connectionhandle
    };

    players.push_back(host);

    RequestClientDataPacket packet(host.ID);
    net.Send(packet.header, connectionhandle);

    if (OnPlayerJoin)
        OnPlayerJoin(players.back());

}


/************************************************************************************************/


void HostState::HandleDisconnection(ConnectionHandle connection)
{
    players.erase(
        std::remove_if(
            players.begin(), players.end(),
            [&](auto& player)
            {
                return player.connection == connection;
            }));
}


/************************************************************************************************/


UpdateTask* HostState::Update(EngineCore&core, UpdateDispatcher& dispatcher, double dT)
{
    net.Update(core, dispatcher, dT);

    return nullptr;
}


/************************************************************************************************/


MultiplayerPlayerID_t HostState::GetNewID() const
{
    while (true)
    {
        const auto newID = GeneratePlayerID();

        if (std::find_if(std::begin(players), std::end(players),
            [&](auto& player) -> bool { return player.ID == newID; }) == players.end())
            return newID;
    }
}


/************************************************************************************************/


HostState::Player* HostState::GetPlayer(MultiplayerPlayerID_t ID)
{
    if (auto res = std::find_if(
            std::begin(players),
            std::end(players),
            [&](auto& player) -> bool { return player.ID == ID; });
            res != players.end())

        return &(*res);
    else
        return nullptr;
}


/************************************************************************************************/


void HostState::BroadCastMessage(std::string msg)
{
    for (auto& player : players)
    {
        LobbyMessagePacket msgPkt{ player.ID, msg };


        net.Send(msgPkt.Header, player.connection);
    }
}


/************************************************************************************************/


void HostState::SendPlayerList(ConnectionHandle dest)
{
    auto& temp  = framework.core.GetTempMemory();
    auto packet = PlayerListPacket::Create(players.size(), temp);


    for (size_t I = 0; I < players.size(); I++)
    {
        strcpy(packet->Players[I].name, players[I].name.c_str());

        packet->Players[I].ready    = false;
        packet->Players[I].ID       = players[I].ID;

        strncpy(packet->Players[I].name, players[I].name.c_str(), sizeof(packet->Players[I].name));
    }

    net.Send(*packet, dest);
}


/************************************************************************************************/


void HostState::SendGameStart()
{
    StartGamePacket startGamePacket;

    for (size_t I = 0; I < players.size(); I++)
        net.Send(startGamePacket, players[I].connection);
}


/************************************************************************************************/


void PushHostState(GameInfo& info, GameFramework& framework, BaseState& base, NetworkState& net)
{
    AddAssetFile("assets\\multiplayerAssets.gameres");

    auto& host  = framework.PushState<HostState>(info, base, net);
    auto& lobby = framework.PushState<LobbyState>(base, net);

    lobby.host  = true;

    lobby.GetPlayer         =
        [&](uint idx) -> LobbyState::Player
        {
            auto& player = host.players[idx];

            const LobbyState::Player lobbyPlayer = {
                    .Name   = player.name,
                    .ID     = player.ID
            };

            return lobbyPlayer;
        };

    lobby.GetPlayerCount    = [&]                       { return (uint)host.players.size(); };
    lobby.OnSendMessage     = [&](std::string message)  { host.BroadCastMessage(message); };

    host.OnMessageRecieved  = [&](std::string message)  { lobby.MessageRecieved(message); };
    host.OnPlayerJoin       =
        [&](HostState::Player& player)
        {
            const LobbyState::Player lobbyPlayer = {
                .Name   = player.name,
                .ID     = player.ID
            };

            lobby.players.push_back(lobbyPlayer);
            lobby.chatHistory  += "Player Joining...\n";
        };

    lobby.OnGameStart =
        [&]
        {
            auto& net_temp  = net;
            auto& base_temp = base;

            framework.PopState();

            host.SendGameStart();

            auto& worldState = framework.core.GetBlockMemory().allocate<HostWorldStateMangager>(host.hostID, net_temp, base_temp);
            auto& localState = framework.PushState<LocalGameState>(worldState, base_temp);

            net_temp.HandleDisconnection =
                [&](ConnectionHandle connection)
                {
                    for(auto& player : worldState.remotePlayerComponent)
                    {
                        if (connection == player.componentData.connection)
                        {
                            player.componentData.gameObject->Release();
                            return;
                        }
                    }
                };

            for (auto& player : host.players)
                if(player.ID != host.hostID)
                    worldState.AddPlayer(player.connection, player.ID);
        };
}


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
