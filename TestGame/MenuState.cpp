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

                HostState::Player p = {
                    .playerName = playerName,
                    .playerID   = pckt->playerID,
                };

                peers.push_back(p);
                OnPlayerJoin(peers.back());
            },
            framework.core.GetBlockMemory()
        ));

    net.PushHandler(packetHandlers);
}


/************************************************************************************************/


void ClientState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    net.Update(core, dispatcher, dT);
}


/************************************************************************************************/


void ClientState::SendChatMessage(std::string msg)
{
    LobbyMessagePacket packet(clientID, msg);

    net.Send(packet.Header, server);
}


/************************************************************************************************/


void ClientState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);
}


/************************************************************************************************/


void ClientState::PostDrawUpdate(EngineCore&, UpdateDispatcher&, double dT)
{

}


/************************************************************************************************/


HostState::HostState(GameFramework& framework, GameInfo IN_info, BaseState& IN_base, NetworkState& IN_net) :
    FrameworkState  { framework },
    net             { IN_net },
    base            { IN_base },
    info            { IN_info },
    handler         { framework.core.GetBlockMemory() }
{
    net.HandleNewConnection = [&](ConnectionHandle connection) { HandleIncomingConnection(connection); };
    net.HandleDisconnection = [&](ConnectionHandle connection) { HandleDisconnection(connection); };

    Player host{
        .playerName = info.name,
        .playerID   = GeneratePlayerID()
    };

    playerList.push_back(host);

    // Register Packet Handlers

    handler.push_back(
        CreatePacketHandler(
            ClientDataRequestResponse,
            [&](UserPacketHeader* incomingPacket, Packet* packet, NetworkState* network)
            {
                const ClientDataPacket* clientData = reinterpret_cast<ClientDataPacket*>(incomingPacket);

                const std::string           playerName  { clientData->playerName };
                const PlayerJoinEventPacket joinEvent   { clientData->playerID };

                for (auto& player : playerList)
                {
                    if (clientData->playerID == player.playerID)
                        player.playerName = playerName;
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

                for (auto& player : playerList)
                {
                    if (player.playerID != msgPacket->playerID)
                    {
                        LobbyMessagePacket outMessage{ player.playerID, msg };
                        net.Send(outMessage.Header, player.connection);
                    }
                }

                if (OnMessageRecieved)
                    OnMessageRecieved(msg);
            },
            framework.core.GetBlockMemory()));

    net.PushHandler(handler);

    net.Startup(1337);
}


/************************************************************************************************/


void HostState::HandleIncomingConnection(ConnectionHandle connectionhandle)
{
    Player host{
        .playerName = "Incoming",
        .playerID   = GeneratePlayerID(),
        .connection = connectionhandle
    };

    playerList.push_back(host);

    RequestClientDataPacket packet(host.playerID);
    net.Send(packet.header, connectionhandle);

    if (OnPlayerJoin)
        OnPlayerJoin(playerList.back());

}

/************************************************************************************************/


void HostState::HandleDisconnection(ConnectionHandle connection)
{
    playerList.erase(
        std::remove_if(
            playerList.begin(), playerList.end(),
            [&](auto& player)
            {
                return player.connection == connection;
            }));
}


/************************************************************************************************/


void HostState::Update(EngineCore&core, UpdateDispatcher& dispatcher, double dT)
{
    net.Update(core, dispatcher, dT);
}


/************************************************************************************************/


void HostState::BroadCastMessage(std::string msg)
{
    for (auto& player : playerList)
    {
        LobbyMessagePacket msgPkt{ player.playerID, msg };


        net.Send(msgPkt.Header, player.connection);
    }
}


/************************************************************************************************/


void LobbyState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.Update(core, dispatcher, dT);
    base.debugUI.Update(base.renderWindow, core, dispatcher, dT);

    net.Update(core, dispatcher, dT);

    ImGui::NewFrame();

    // Player List

    ImGui::Begin("Lobby");

    int selected = 0;
    const char* players[16];
    for (size_t I = 0; I < playerList.size(); I++)
        players[I] = playerList[I].playerName.c_str();

    ImGui::ListBox("Players", &selected, players, playerList.size());

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

    ImGui::End();
}


/************************************************************************************************/


void LobbyState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);
}


/************************************************************************************************/


void LobbyState::PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.PostDrawUpdate(core, dispatcher, dT);
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
    memset(name, '\0',      sizeof(name));
    memset(lobbyName, '\0', sizeof(lobbyName));
    memset(server, '\0',    sizeof(server));

    packetHandlers.push_back(
        CreatePacketHandler(
            UserPacketIDs::ClientDataRequest,
            [&](UserPacketHeader* header, Packet* packet, NetworkState* network)
            {
                FK_LOG_INFO("Joining Lobby");

                auto request = reinterpret_cast<RequestClientDataPacket*>(header);
                ClientDataPacket clientPacket(request->playerID, name);

                net.Send(clientPacket.Header, packet->sender);

                auto& net_temp  = net;
                auto& base_temp = base;

                net.PopHandler();

                framework.PopState();

                auto& client    = framework.PushState<ClientState>(request->playerID, packet->sender, base_temp, net_temp);
                auto& lobby     = framework.PushState<LobbyState>(base_temp, net_temp);

                client.OnPlayerJoin         = [&](HostState::Player& player){ lobby.chatHistory += "Player Joined\n"; };
                client.OnMessageRecieved    = [&](std::string msg) { lobby.MessageRecieved(msg); };
                lobby.OnSendMessage         = [&](std::string msg) { client.SendChatMessage(msg); };

            }, framework.core.GetBlockMemory()));
}


/************************************************************************************************/


void MenuState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
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

            lobby.GetPlayer         = [&](uint idx) { return host.playerList[idx]; };
            lobby.GetPlayerCount    = [&] {return host.playerList.size(); };

            host.OnPlayerJoin       = [&](HostState::Player& player){
                lobby.chatHistory  += "Player Joining...\n";
            };

            host.OnMessageRecieved  = [&](std::string message) { lobby.MessageRecieved(message); };
            lobby.OnSendMessage     = [&](std::string message) { host.BroadCastMessage(message); };
        }

        ImGui::End();
    }   break;
    default:
        framework.quit = true;
    }

}


/************************************************************************************************/


void MenuState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{

    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);
}


/************************************************************************************************/


void MenuState::PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.PostDrawUpdate(core, dispatcher, dT);
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


/************************************************************************************************/
