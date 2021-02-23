#include "MenuState.h"


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

            lobby.GetPlayerCount    = [&]                       { return host.players.size(); };
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
