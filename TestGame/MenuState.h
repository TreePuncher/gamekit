#ifndef MENUSTATE_H
#define MENUSTATE_H

#include "GameFramework.h"
#include "CameraUtilities.h"
#include "BaseState.h"
#include "Packets.h"
#include "MultiplayerState.h"
#include "LobbyGUI.h"

#include "Host.h"
#include "Client.h"

using FlexKit::iAllocator;
using FlexKit::FrameworkState;
using FlexKit::GameFramework;
using FlexKit::EngineCore;
using FlexKit::Event;


/************************************************************************************************/


class MenuState : public FrameworkState
{
public:
    MenuState(GameFramework& framework, BaseState& IN_base, NetworkState& IN_net);

    UpdateTask* Update      (             EngineCore&, UpdateDispatcher&, double dT) override;
    UpdateTask* Draw        (UpdateTask*, EngineCore&, UpdateDispatcher&, double dT, FrameGraph&)  override;

    void PostDrawUpdate     (EngineCore&, double dT) override;

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

    double  connectionTimer = 0;

    PacketHandlerVector packetHandlers;

    NetworkState&       net;
    BaseState&          base;
};


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
