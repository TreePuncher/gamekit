/**********************************************************************

Copyright (c) 2018 Robert May

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

#ifndef LOBBYGUI_H_INCLUDED
#define LOBBYGUI_H_INCLUDED


#include "EngineCore.h"
#include "FrameGraph.h"
#include "GuiUtilities.h"
#include "Fonts.h"


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

        for (size_t I = 0; I < 25; ++I)
            localPlayerCards.push_back(&base.framework.core.GetBlockMemory().allocate<PowerCard>());

        for (size_t I = 0; I < 25; ++I)
            localPlayerCards.push_back(&base.framework.core.GetBlockMemory().allocate<FireBall>());
    }



    UpdateTask* Update  (             EngineCore&, UpdateDispatcher&, double dT) override;
    UpdateTask* Draw    (UpdateTask*, EngineCore&, UpdateDispatcher&, double dT, FrameGraph&)  override;

    void PostDrawUpdate (EngineCore&, double dT) override;

    bool EventHandler   (Event evt) override;

    void MessageRecieved(std::string& msg)
    {
        chatHistory += msg + '\n';
    }

    bool                                            host        = false;
    int                                             selection2  = 0;

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

    std::function<void()>                OnApplySpellbookChanges    = [] {};


    std::vector<CardInterface*> localPlayerCards;

    std::vector<Player> players;
    std::string         chatHistory;

    BaseState&          base;
    NetworkState&       net;

    char inputBuffer[512];
};

#endif
