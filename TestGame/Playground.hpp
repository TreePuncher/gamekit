#pragma once
#include "GameFramework.h"
#include "BaseState.h"
#include "WorldRender.h"

using FlexKit::iAllocator;
using FlexKit::FrameworkState;
using FlexKit::GameFramework;
using FlexKit::EngineCore;
using FlexKit::Event;

class PlaygroundState : public FrameworkState
{
public:
    PlaygroundState(GameFramework& framework, BaseState& IN_base) :
        FrameworkState  { framework },
        base            { IN_base },
        render          { framework.GetRenderSystem(), IN_base.streamingEngine, framework.core.GetBlockMemory() }
    {

    }


    ~PlaygroundState()
    {

    }


    UpdateTask* Update  (EngineCore&, UpdateDispatcher&, double dT) override
    {
        return nullptr;
    }


    UpdateTask* Draw    (UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)  override
    {
        auto outputs = render.DrawScene(dispatcher, frameGraph, );
        
        return &outputs.passes;
    }


    void PostDrawUpdate (EngineCore&, double dT) override
    {
    }


    bool EventHandler   (Event evt) override
    {
        return false;
    }

    BaseState&              base;
    FlexKit::WorldRender    render;
};
