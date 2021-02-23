#include "Host.h"
#include <random>
#include <limits>


using FlexKit::GameFramework;


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
