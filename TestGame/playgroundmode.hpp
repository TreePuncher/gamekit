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

    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
    {
        return base.Update(core, dispatcher, dT);
    }

    UpdateTask* Draw(UpdateTask* update, EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final override
    {
        return nullptr;
    }

    void PostDrawUpdate(EngineCore& core, double dT) final override
    {
        base.PostDrawUpdate(core, dT);
    }

    BaseState& base;
};
