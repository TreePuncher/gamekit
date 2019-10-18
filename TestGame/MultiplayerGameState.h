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

	GameState				(const GameState&) = delete;
	GameState& operator =	(const GameState&) = delete;

	bool Update			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool PreDrawUpdate	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool Draw			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;
	bool PostDrawUpdate	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;

	GraphicScene	scene;
	BaseState&		base;
	size_t			frameID;
};


// Takes inputs,
// passes inputs to GameState
// timeline
class LocalPlayerState : public FlexKit::FrameworkState
{
public:
    LocalPlayerState(FlexKit::GameFramework& IN_framework, BaseState& IN_base, GameState& IN_game);

	virtual ~LocalPlayerState() final override {}

	bool Update			(EngineCore& core, UpdateDispatcher& Dispatcher, double dT) final override;
	bool PreDrawUpdate	(EngineCore& core, UpdateDispatcher& Dispatcher, double dT) final override;
    bool Draw           (EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) final override;
    bool EventHandler   (Event evt) final override;


private:	/************************************************************************************************/

	NetObjectInputComponent			netInputObjects;
	InputMap						eventMap;
	OrbitCameraBehavior				debugCamera;
	BaseState&						base;
	GameState&						game;
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
        FrameworkState  { IN_framework  },
        base            { IN_base       }  {}

	BaseState& base;
};


/************************************************************************************************/


struct GameLoadSceneStateDefaultAction
{
    void operator()(EngineCore& core, UpdateDispatcher& dispatcher, double dT) {}
};

struct GameLoadSceneStateUpdateDefaultAction
{
    bool operator()(EngineCore& core, UpdateDispatcher& dispatcher, double dT) { return true; }
};


struct GameLoadSceneStateFailureDefaultAction
{
    void operator()() {}
};

// Loads Scene and sends server notification on completion
template<
    typename TY_OnCompletion    = GameLoadSceneStateDefaultAction,
    typename TY_OnUpdate        = GameLoadSceneStateUpdateDefaultAction,
    typename TY_OnFailure       = GameLoadSceneStateFailureDefaultAction>
class GameLoadSceneState : public FrameworkState
{
public:
    GameLoadSceneState(
        GameFramework&  IN_framework,
        BaseState&      IN_base,
        GraphicScene&   IN_scene,
        const char*     IN_sceneName,
        TY_OnCompletion IN_OnCompletion = GameLoadSceneStateDefaultAction{},
        TY_OnUpdate     IN_OnUpdate     = GameLoadSceneStateUpdateDefaultAction{},
        TY_OnFailure    IN_OnFailure    = GameLoadSceneStateFailureDefaultAction{}) :
            FrameworkState  { IN_framework      },
            base            { IN_base           },
            scene           { IN_scene          },
            OnCompletion    { IN_OnCompletion   },
            OnUpdate        { IN_OnUpdate       }
    {
        iAllocator* allocator = IN_framework.core.GetBlockMemory();

        auto& task      = CreateLambdaWork(
            [&, &core   = base.framework.core]
            {
                if(!LoadScene(core, &scene, IN_sceneName))
                    OnFailure();

                completed = true;
            },
            allocator);

        framework.core.Threads.AddWork(task, allocator);
    }


    /************************************************************************************************/


    virtual ~GameLoadSceneState() override {}


    /************************************************************************************************/


    bool Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT) final override
    {
        beginGame = OnUpdate(core, dispatcher, dT);

        if (completed && beginGame)
        {
            auto& temp_OnCompletion = std::move(OnCompletion); // Must be moved to stack before class is popped off
            framework.PopState();
            temp_OnCompletion(core, dispatcher, dT);
        }


        return true;
    }


    /************************************************************************************************/


    bool EventHandler(Event evt) override
    {
        return true;
    }

    bool Completed()    { return completed; }
    bool Begin()        { return beginGame; }

    void BeginGame()    { beginGame = true; }

private:

    bool            completed = false;
    bool            beginGame = false;
    BaseState&      base;
    GraphicScene&   scene;
    TY_OnCompletion OnCompletion;
    TY_OnUpdate     OnUpdate;
    TY_OnFailure    OnFailure;
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
