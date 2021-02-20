#include "MenuState.h"


/************************************************************************************************/


ClientState::ClientState(
    GameFramework&          framework,
    MultiplayerPlayerID_t   clientID,
    ConnectionHandle        server,
    BaseState&              IN_base,
    NetworkState&           IN_net) :
            FrameworkState  { framework },
            clientID        { clientID  },
            server          { server    },
            net             { IN_net    },
            base            { IN_base   },
            packetHandlers  { framework.core.GetBlockMemory() }
{
    packetHandlers.push_back(
        CreatePacketHandler(
            LobbyMessage,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                LobbyMessagePacket* pckt = reinterpret_cast<LobbyMessagePacket*>(header);
                std::string msg = pckt->message;

                OnMessageRecieved(msg);
            },
            framework.core.GetBlockMemory()
        ));

    packetHandlers.push_back(
        CreatePacketHandler(
            PlayerJoin,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                auto pckt               = reinterpret_cast<PlayerJoinEventPacket*>(header);
                std::string playerName  = pckt->PlayerName;

                Player p = {
                    .name   = playerName,
                    .ID     = pckt->playerID,
                };

                peers.push_back(p);
                OnPlayerJoin(peers.back());
            },
            framework.core.GetBlockMemory()
        ));

    packetHandlers.push_back(
        CreatePacketHandler(
            BeginGame,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                OnGameStart();
            },
            framework.core.GetBlockMemory()
        ));

    packetHandlers.push_back(
        CreatePacketHandler(
            RequestPlayerListResponse,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                FK_LOG_INFO("Player List Recieved!");

                auto playerList = static_cast<PlayerListPacket*>(header);

                const auto playerCount = playerList->playerCount;
                for (uint32_t I = 0; I < playerCount; ++I)
                {
                    Player player;
                    player.name = playerList->Players[I].name;
                    player.ID   = playerList->Players[I].ID;

                    if(player.ID != clientID)
                        peers.push_back(player);
                }
                
            },
            framework.core.GetBlockMemory()
                ));

    net.PushHandler(packetHandlers);
}


/************************************************************************************************/


UpdateTask* ClientState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    return net.Update(core, dispatcher, dT);
}


/************************************************************************************************/


void ClientState::SendChatMessage(std::string msg)
{
    LobbyMessagePacket packet(clientID, msg);

    net.Send(packet.Header, server);
}


/************************************************************************************************/


void ClientState::RequestPlayerList()
{
    RequestPlayerListPacket packet{ clientID };

    net.Send(packet.Header, server);
}


/************************************************************************************************/


UpdateTask* ClientState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);

    return nullptr;
}


/************************************************************************************************/


void ClientState::PostDrawUpdate(EngineCore&, double dT)
{

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


UpdateTask* LobbyState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.Update(core, dispatcher, dT);
    base.debugUI.Update(base.renderWindow, core, dispatcher, dT);

    net.Update(core, dispatcher, dT);

    if (framework.PushPopDetected())
        return nullptr;

    ImGui::NewFrame();

    // Player List

    ImGui::Begin("Lobby");

    int selected = 0;

    std::string playerStrings[16];
    const char* playerNames[16];
    const uint playerCount = GetPlayerCount();

    for (size_t I = 0; I < playerCount; I++) {
        auto player         = GetPlayer(I);
        playerStrings[I]    = player.Name;
        playerNames[I]      = playerStrings[I].c_str();
    }

    ImGui::ListBox("Players", &selected, playerNames, (int)playerCount);

    // Chat
    ImGui::InputTextMultiline("Chat Log", const_cast<char*>(chatHistory.c_str()), chatHistory.size(), ImVec2(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputText("type here...", inputBuffer, 512);

    if (ImGui::Button("Send"))
    {
        std::string msg = inputBuffer;
        chatHistory += inputBuffer;
        chatHistory += "\n";

        memset(inputBuffer, '\0', 512);

        if (OnSendMessage)
            OnSendMessage(msg);
    }

    if (ImGui::Button("StartGame"))
        OnGameStart();
    else if (ImGui::Button("Ready"))
        OnReady();

    ImGui::End();


    return nullptr;
}


/************************************************************************************************/


UpdateTask* LobbyState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);

    return nullptr;
}


/************************************************************************************************/


void LobbyState::PostDrawUpdate(EngineCore& core, double dT)
{
    base.PostDrawUpdate(core, dT);
}


/************************************************************************************************/


bool LobbyState::EventHandler(Event evt)
{
    switch (evt.InputSource)
    {
    case Event::E_SystemEvent:
    {
        switch (evt.Action)
        {
        case Event::InputAction::Resized:
        {
            const auto width    = (uint32_t)evt.mData1.mINT[0];
            const auto height   = (uint32_t)evt.mData2.mINT[0];

            base.Resize({ width, height });
        }   break;

        case Event::InputAction::Exit:
            framework.quit = true;
            break;
        default:
            break;
        }
    }   break;
    };

    return base.debugUI.HandleInput(evt);
}


/************************************************************************************************/


MenuState::MenuState(GameFramework& framework, BaseState& IN_base, NetworkState& IN_net) :
	FrameworkState	    { framework },
    net                 { IN_net },
    base                { IN_base },
    packetHandlers{ framework.core.GetBlockMemory() }
{
    memset(name,        '\0', sizeof(name));
    memset(lobbyName,   '\0', sizeof(lobbyName));
    memset(server,      '\0', sizeof(server));

    packetHandlers.push_back(
        CreatePacketHandler(
            UserPacketIDs::ClientDataRequest,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                auto request = reinterpret_cast<RequestClientDataPacket*>(header);
                ClientDataPacket clientPacket(request->playerID, name);

                FK_LOG_INFO("Joining Lobby ID: %u", request->playerID);

                net.Send(clientPacket.Header, packet->sender);

                auto& net_temp  = net;
                auto& base_temp = base;

                net.PopHandler();

                framework.PopState();

                auto& client    = framework.PushState<ClientState>(request->playerID, packet->sender, base_temp, net_temp);
                auto& lobby     = framework.PushState<LobbyState>(base_temp, net_temp);

                client.OnMessageRecieved    = [&](std::string msg) { lobby.MessageRecieved(msg); };
                client.OnPlayerJoin         =
                    [&](ClientState::Player& player)
                    {
                        LobbyState::Player lobbyPlayer = {
                            .Name   = player.name,
                            .ID     = player.ID,
                        };

                        lobby.players.push_back(lobbyPlayer);
                        lobby.chatHistory += "Player Joined\n";
                    };

                client.OnGameStart          =
                    [&]()
                    {
                        auto& framework_temp    = framework;
                        auto& net_temp          = net;
                        auto& base_temp         = base;

                        framework_temp.PopState();

                        auto& worldState = framework_temp.core.GetBlockMemory().allocate<ClientWorldStateMangager>(client.server, client.clientID, net_temp, base_temp);
                        auto& localState = framework_temp.PushState<LocalGameState>(worldState, base_temp);

                        for (auto& player : client.peers)
                            if(player.ID != client.clientID)
                                worldState.AddRemotePlayer(player.ID);
                    };

                lobby.OnSendMessage         = [&](std::string msg) { client.SendChatMessage(msg); };
                lobby.GetPlayer             =
                    [&](uint idx) -> LobbyState::Player
                    {
                        auto& peer = client.peers[idx];
                        
                        LobbyState::Player out{
                            .Name   = peer.name,
                            .ID     = peer.ID,
                        };

                        return out;
                    };

                lobby.GetPlayerCount        = [&](){ return client.peers.size(); };

                client.RequestPlayerList();
            }, framework.core.GetBlockMemory()));
}


/************************************************************************************************/


UpdateTask* MenuState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.Update(core, dispatcher, dT);
    base.debugUI.Update(base.renderWindow, core, dispatcher, dT);

    ImGui::NewFrame();

    switch (mode)
    {
    case MenuMode::MainMenu:
        ImGui::Begin("Menu");

        if (ImGui::Button("Join"))
            mode = MenuMode::Join;

        if (ImGui::Button("Host"))
            mode = MenuMode::Host;

        if (ImGui::Button("Exit"))
            framework.quit = true;

        ImGui::End();
        break;
    case MenuMode::Join:
    {
        ImGui::Begin("Join");
        ImGui::InputText("Name", name, 128);
        ImGui::InputText("Server Address", server, 128);

        if (ImGui::Button("Join") && strnlen_s(name, 128) && strnlen_s(server, 128))
        {
            net.PushHandler(packetHandlers);
            net.Connect(server, 1337);

            mode = MenuMode::JoinInProgress;
        }

        ImGui::End();
    }   break;
    case MenuMode::JoinInProgress:
    {
        net.Update(core, dispatcher, dT);

        connectionTimer += dT;

        std::cout << connectionTimer << "\n";

        if (connectionTimer > 30.0)
        {
            net.Disconnect();
            net.PopHandler();
            mode = MenuMode::MainMenu;

            connectionTimer = 0.0;
        }
        ImGui::Begin("Joining...");
        ImGui::End();
    }   break;
    case MenuMode::Host:
    {   // Get Host name, start listing, push lobby state
        ImGui::Begin("Host");
        ImGui::InputText("Name", name, 128);
        ImGui::InputText("LobbyName", lobbyName, 128);

        if (ImGui::Button("Start") && strnlen_s(name, 128) && strnlen_s(lobbyName, 128))
        {
            GameInfo info;
            info.name       = name;
            info.lobbyName  = lobbyName;

            auto& framework_temp    = framework;
            auto& base_temp         = base;
            auto& net_temp          = net;

            framework_temp.PopState();

            auto& host  = framework_temp.PushState<HostState>(info, base_temp, net_temp);
            auto& lobby = framework_temp.PushState<LobbyState>(base_temp, net_temp);

            lobby.GetPlayer         = [&](uint idx) -> LobbyState::Player
                {
                    auto& player = host.players[idx];

                    const LobbyState::Player lobbyPlayer = {
                            .Name   = player.name,
                            .ID     = player.ID
                    };

                    return lobbyPlayer;
                };

            lobby.GetPlayerCount    = [&]                       { return host.players.size(); };
            lobby.OnSendMessage     = [&](std::string message)  { host.BroadCastMessage(message); };

            host.OnMessageRecieved  = [&](std::string message)  { lobby.MessageRecieved(message); };
            host.OnPlayerJoin       = [&](HostState::Player& player)
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

                    framework_temp.PopState();

                    host.SendGameStart();

                    auto& worldState = framework_temp.core.GetBlockMemory().allocate<HostWorldStateMangager>(host.hostID, net_temp, base_temp);
                    auto& localState = framework_temp.PushState<LocalGameState>(worldState, base_temp);

                    for (auto& player : host.players)
                        if(player.ID != host.hostID)
                            worldState.AddPlayer(player.connection, player.ID);
                };
        }

        ImGui::End();
    }   break;
    default:
        framework.quit = true;
    }

    return nullptr;
}


/************************************************************************************************/


UpdateTask* MenuState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{

    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();

    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);

    return nullptr;
}


/************************************************************************************************/


void MenuState::PostDrawUpdate(EngineCore& core, double dT)
{
    base.PostDrawUpdate(core, dT);
}


/************************************************************************************************/


bool MenuState::EventHandler(Event evt)
{
    switch (evt.InputSource)
    {
    case Event::E_SystemEvent:
    {
        switch (evt.Action)
        {
        case Event::InputAction::Resized:
        {
            const auto width    = (uint32_t)evt.mData1.mINT[0];
            const auto height   = (uint32_t)evt.mData2.mINT[0];

            base.Resize({ width, height });
        }   break;

        case Event::InputAction::Exit:
            framework.quit = true;
            break;
        default:
            break;
        }
    }   break;
    };

    return base.debugUI.HandleInput(evt);
}


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
