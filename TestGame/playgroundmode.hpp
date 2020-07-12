#pragma once

#include <Application.h>
#include "BaseState.h"

class PlayGroundMode : public FlexKit::FrameworkState
{
    PlayGroundMode(FlexKit::GameFramework& IN_framework, BaseState& IN_baseState) :
        FrameworkState  { IN_framework },
        base            { IN_baseState }
    {

    }

    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
    {
        base.Update(core, dispatcher, dT);
    }

    void Draw(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final override
    {
    }

    void PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
    {
        base.PostDrawUpdate(core, dispatcher, dT);
    }

    BaseState& base;
};
