#include "Client.h"
#include "MathUtils.h"


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


void ClientState::SendSpellbookUpdate(std::vector<CardInterface*>& spellbook)
{
    SpellBookUpdatePacket packet{ clientID };

    for (size_t I = 0; I < spellbook.size(); ++I)
        packet.spellBook[I] = spellbook[I]->CardID;

    net.Send(packet.header, server);
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


void PushClientState(const MultiplayerPlayerID_t playerID, const ConnectionHandle server, BaseState& base, NetworkState& net)
{
    auto& framework = base.framework;
    auto& client    = framework.PushState<ClientState>(playerID, server, base, net);
    auto& lobby     = framework.PushState<LobbyState>(base, net);

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

            AddAssetFile("assets\\multiplayerAssets.gameres");

            auto& worldState = framework_temp.core.GetBlockMemory().allocate<ClientWorldStateMangager>(client.server, client.clientID, net_temp, base_temp);
            auto& localState = framework_temp.PushState<LocalGameState>(worldState, base_temp);

            net_temp.HandleDisconnection = [&](ConnectionHandle connection) { framework.quit = true; };

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

    lobby.GetPlayerCount        = [&](){ return (uint)client.peers.size(); };

    client.RequestPlayerList();
}

