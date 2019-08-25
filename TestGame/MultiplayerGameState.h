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
using		NetComponentHandle					= FlexKit::Handle_t<32, NetObjectComponentID>;
using		NetObjectID							= size_t;

struct NetObject
{
	GameObject* gameObject;
	NetObjectID	netID;
};

using NetObjectReplicationComponent = FlexKit::BasicComponent_t<NetObject, NetComponentHandle, NetObjectComponentID>;


/************************************************************************************************/


constexpr	FlexKit::ComponentID	NetInputEventsComponentID	= GetTypeGUID(NetInputEventsComponentID);
using								NetInputComponentHandle		= FlexKit::Handle_t<32, NetInputEventsComponentID>;

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
	GameState(FlexKit::GameFramework* IN_framework, BaseState& base);

	~GameState();

	GameState				(const GameState&) = delete;
	GameState& operator =	(const GameState&) = delete;

	bool Update			(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) final;
	bool PreDrawUpdate	(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) final;
	bool Draw			(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph) final;
	bool PostDrawUpdate	(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph) final;

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
	LocalPlayerState(FlexKit::GameFramework* IN_framework, BaseState& IN_base, GameState& IN_game) : 
		FrameworkState	{ IN_framework							},
		game			{ IN_game								},
		base			{ IN_base								},
		eventMap		{ IN_framework->core->GetBlockMemory()	},
		netInputObjects	{ IN_framework->core->GetBlockMemory()	},
		netObjects		{ IN_framework->core->GetBlockMemory()	}
	{
		eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward);
		eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward);
		eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft);
		eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight);
	}


	/************************************************************************************************/


	virtual ~LocalPlayerState() override
	{
	}

	/************************************************************************************************/


	bool Update			(EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT) final
	{
		return true;
	}


	/************************************************************************************************/


	bool PreDrawUpdate	(EngineCore* core, FlexKit::UpdateDispatcher& Dispatcher, double dT) final
	{
		return true;
	}


	/************************************************************************************************/


	bool Draw			(EngineCore* core, FlexKit::UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph) final
	{
		frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

		CameraHandle activeCamera = debugCamera;

		auto& scene				= game.scene;
		auto& transforms		= QueueTransformUpdateTask	(dispatcher);
		auto& cameras			= CameraComponent::GetComponent().QueueCameraUpdate(dispatcher, transforms);
		auto& orbitUpdate		= QueueOrbitCameraUpdateTask(dispatcher, transforms, cameras, debugCamera, framework->MouseState, dT);
		auto& cameraConstants	= MakeHeapCopy				(Camera::ConstantBuffer{}, core->GetTempMemory());
		auto& PVS				= GetGraphicScenePVSTask	(dispatcher, scene, activeCamera, core->GetTempMemory());
		auto& textureStreams	= base.streamingEngine.update(dispatcher);

		FlexKit::WorldRender_Targets targets = {
			GetCurrentBackBuffer(&core->Window),
			base.depthBuffer
		};

		LighBufferDebugDraw debugDraw;
		debugDraw.constantBuffer = base.constantBuffer;
		debugDraw.renderTarget   = targets.RenderTarget;
		debugDraw.vertexBuffer	 = base.vertexBuffer;

		SceneDescription sceneDesc;
		sceneDesc.lights			= &scene.GetPointLights(dispatcher, core->GetTempMemory());
		sceneDesc.transforms		= &transforms;
		sceneDesc.cameras			= &cameras;
		sceneDesc.PVS				= &PVS;
	
		base.render.updateLightBuffers(dispatcher, activeCamera, scene, frameGraph, sceneDesc, core->GetTempMemory(), &debugDraw);

		ClearVertexBuffer(frameGraph, base.vertexBuffer);
		ClearVertexBuffer(frameGraph, base.textBuffer);

		ClearBackBuffer(frameGraph, targets.RenderTarget, 0.0f);
		ClearDepthBuffer(frameGraph, base.depthBuffer, 1.0f);

		base.render.RenderDrawabledPBR_ForwardPLUS(dispatcher, PVS.GetData().solid, activeCamera, targets, frameGraph, sceneDesc, core->GetTempMemory());

		return true;
	}


	/************************************************************************************************/


	bool EventHandler(Event evt) final
	{
		eventMap.Handle(evt, [&](auto& evt)
			{
				debugCamera.HandleEvent(evt);
			});

		return true;
	}



private:	/************************************************************************************************/

	NetObjectInputComponent			netInputObjects;
	NetObjectReplicationComponent	netObjects;
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
	RemotePlayerState(FlexKit::GameFramework* IN_framework, BaseState& IN_base) :
        FrameworkState  { IN_framework  },
        base            { IN_base       }  {}

	BaseState& base;
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
