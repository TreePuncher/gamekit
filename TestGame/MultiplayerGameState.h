#ifndef MULTIPLAYERGAMESTATE_H
#define MULTIPLAYERGAMESTATE_H

#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Components.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\ResourceHandles.h"

#include "BaseState.h"
#include "Gameplay.h"
#include "MultiplayerState.h"


/************************************************************************************************/


class NetworkState;
class PacketHandler;


/************************************************************************************************/


constexpr	ComponentID NetObjectComponentID	= GetTypeGUID(NetObjectComponentID);
using		NetComponentHandle					= Handle_t<32, NetObjectComponentID>;
using		NetObjectID							= size_t;

struct NetObject
{
	GameObject* gameObject;
	NetObjectID	netID;
};

using NetObjectReplicationComponent = BasicComponent_t<NetObject, NetComponentHandle, NetObjectComponentID>;


/************************************************************************************************/


constexpr	ComponentID	NetInputEventsComponentID	= GetTypeGUID(NetInputEventsComponentID);
using					NetInputComponentHandle		= Handle_t<32, NetInputEventsComponentID>;

struct NetInputFrame
{
	bool forward	= false;
	bool backward	= false;
	bool left		= false;
	bool right		= false;
};

struct NetStateFrame
{
	float3	position;
	size_t	frameID;
};

struct NetInput
{
	GameObject*							gameObject;
	CircularBuffer<NetInputFrame, 60>	inputFrames;
	CircularBuffer<NetInputFrame, 60>	stateFrames;
};

using NetObjectInputComponent = FlexKit::BasicComponent_t<NetInput, NetComponentHandle, NetObjectComponentID>;


/************************************************************************************************/

// current frame of game state
class GameState final : public FlexKit::FrameworkState
{
public:
    GameState(GameFramework& IN_framework, BaseState& base);

    ~GameState();

    GameState(const GameState&) = delete;
    GameState& operator =	(const GameState&) = delete;

    void Update         (EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final;
    void PreDrawUpdate  (EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final;
    void Draw           (EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;
    void PostDrawUpdate (EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;

    GraphicScene	    scene;
    PhysXSceneHandle    pScene;
    BaseState&          base;
    size_t			    frameID;
};


// Takes inputs,
// passes inputs to GameState
// timeline
class LocalPlayerState : public FlexKit::FrameworkState
{
public:
    LocalPlayerState(FlexKit::GameFramework& IN_framework, BaseState& IN_base, GameState& IN_game);

    virtual ~LocalPlayerState() final override {}

    void Update         (EngineCore& core, UpdateDispatcher& Dispatcher, double dT) final override;
    void PreDrawUpdate  (EngineCore& core, UpdateDispatcher& Dispatcher, double dT) final override;
    void Draw           (EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) final override;
    void PostDrawUpdate (EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) final override;
    bool EventHandler   (Event evt) final override;


private:	/************************************************************************************************/


    enum class RenderMode
    {
        ForwardPlus,
        Deferred,
        ComputeTiledDeferred
    }   renderMode = RenderMode::Deferred;

    NetObjectInputComponent		netInputObjects;
    InputMap					eventMap;
    OrbitCameraBehavior			debugCamera;
    BaseState&                  base;
    GameState&                  game;
};


/************************************************************************************************/


// Takes inputs,
// passes inputs to remote GameState and a local GameState
// net object replication
// handles latency mitigation
class RemotePlayerState : public FlexKit::FrameworkState
{
public:
    RemotePlayerState(GameFramework& IN_framework, BaseState& IN_base) :
        FrameworkState{ IN_framework },
        base{ IN_base }  {}

    BaseState& base;
};


/************************************************************************************************/


struct GameLoadScreenStateDefaultAction
{
    void operator()(EngineCore& core, UpdateDispatcher& dispatcher, double dT) {}
};


struct GameLoadScreenStateDefaultDraw
{
    void operator()(EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, double dT)
    {
    }
};



// Loads Scene and sends server notification on completion
template<
    typename TY_OnUpdate    = GameLoadScreenStateDefaultAction,
    typename TY_OnDraw      = GameLoadScreenStateDefaultDraw>
class GameLoadScreenState : public FrameworkState
{
public:
    GameLoadScreenState(
        GameFramework&  IN_framework,
        BaseState&      IN_base,
        TY_OnUpdate     IN_OnUpdate     = GameLoadScreenStateDefaultAction{},
        TY_OnDraw       IN_OnDraw       = GameLoadScreenStateDefaultDraw{}) :
            FrameworkState  { IN_framework      },
            base            { IN_base           },
            OnUpdate        { IN_OnUpdate       },
            OnDraw          { IN_OnDraw         } {}


    /************************************************************************************************/


    virtual ~GameLoadScreenState() override {}


    /************************************************************************************************/


    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
    {
        OnUpdate(core, dispatcher, dT);
    }


    void Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) final override
    {
        WorldRender_Targets targets = {
            core.Window.backBuffer,
            base.depthBuffer
        };

        frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

        ClearVertexBuffer(frameGraph, base.vertexBuffer);

        ClearBackBuffer(frameGraph, targets.RenderTarget, 0.0f);
        ClearDepthBuffer(frameGraph, base.depthBuffer, 1.0f);

        OnDraw(core, dispatcher, frameGraph, dT);

        PrintTextFormatting Format = PrintTextFormatting::DefaultParams();
        Format.Scale = { 1.0f, 1.0f };

        DrawSprite_Text(
            "Loading...",
            frameGraph,
            *framework.DefaultAssets.Font,
            base.vertexBuffer,
            core.Window.backBuffer,
            core.GetTempMemory(),
            Format);

        PresentBackBuffer(frameGraph, &core.Window);
    }


    /************************************************************************************************/

private:

    BaseState&    base;
    TY_OnUpdate   OnUpdate;
    TY_OnDraw     OnDraw;
};


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


#endif
