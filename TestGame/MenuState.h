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

    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override;


    struct Player
    {
        std::string             playerName;
        MultiplayerPlayerID_t   playerID;
        ConnectionHandle        connection;
    };

    std::vector<Player> playerList;

    std::function<void (std::string)>       OnMessageRecieved   = [](std::string msg) {};
    std::function<void (Player& player)>    OnPlayerJoin        = [](Player& player) {};

    Vector<PacketHandler*> handler;

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

    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override;
    void Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)  override;
    void PostDrawUpdate(EngineCore&, UpdateDispatcher&, double dT) override;

    std::function<void()>                           OnPlayerListReceived    = [](){};
    std::function<void(std::string)>                OnMessageRecieved       = [](std::string msg) {};
    std::function<void(HostState::Player& player)>  OnPlayerJoin            = [](HostState::Player& player) {};

    std::vector<HostState::Player>  peers;
    MultiplayerPlayerID_t           clientID;
    ConnectionHandle                server;
    PacketHandlerVector             packetHandlers;
    NetworkState& net;
    BaseState& base;
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



    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override;
    void Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)  override;

    void PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT) override;
    bool EventHandler(Event evt) override;

    void MessageRecieved(std::string& msg)
    {
        chatHistory += msg + '\n';
    }

    std::function<void              (std::string)>  OnSendMessage;
    std::function<HostState::Player (uint idx)>     GetPlayer       = [](uint idx){ return HostState::Player{};};
    std::function<size_t            ()>             GetPlayerCount  = []{ return 0;};
    
    std::vector<HostState::Player>      playerList;
    std::string                         chatHistory;
    BaseState&                          base;
    NetworkState&                       net;

    char inputBuffer[512];
};


/************************************************************************************************/


class MenuState : public FrameworkState
{
public:
    MenuState(GameFramework& framework, BaseState& IN_base, NetworkState& IN_net);

    void Update         (EngineCore&, UpdateDispatcher&, double dT) override;
    void Draw           (EngineCore&, UpdateDispatcher&, double dT, FrameGraph&)  override;
    void PostDrawUpdate (EngineCore&, UpdateDispatcher&, double dT) override;

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

    PacketHandlerVector packetHandlers;

    NetworkState&       net;
    BaseState&          base;
};


/************************************************************************************************/

#endif
