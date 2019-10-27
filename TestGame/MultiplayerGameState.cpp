#include "MultiplayerGameState.h"
#include "Packets.h"


using namespace FlexKit;


/************************************************************************************************/


GameState::GameState(
			GameFramework&  IN_framework, 
			BaseState&		IN_base) :
		        FrameworkState	{ IN_framework },
		
		        frameID			{ 0										},
		        base			{ IN_base								},
		        scene			{ IN_framework.core.GetBlockMemory()	}
{}


/************************************************************************************************/


GameState::~GameState()
{
	iAllocator* allocator = base.framework.core.GetBlockMemory();
	auto entities = scene.sceneEntities;

	for (auto entity : entities)
	{
		auto entityGO = SceneVisibilityView::GetComponent()[entity].entity;
		scene.RemoveEntity(*entityGO);

		entityGO->Release();
		allocator->free(entityGO);
	}

	scene.ClearScene();
}


/************************************************************************************************/


bool GameState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool GameState::PreDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool GameState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	return true;
}


/************************************************************************************************/


bool GameState::PostDrawUpdate(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	return true;
}


/************************************************************************************************/


LocalPlayerState::LocalPlayerState(GameFramework& IN_framework, BaseState& IN_base, GameState& IN_game) :
		FrameworkState	{ IN_framework							},
		game			{ IN_game								},
		base			{ IN_base								},
		eventMap		{ IN_framework.core.GetBlockMemory()	},
		netInputObjects	{ IN_framework.core.GetBlockMemory()	}
{
	eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward);
	eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward);
	eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft);
	eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight);

    debugCamera.TranslateWorld({0, 10, 0});
}


/************************************************************************************************/


bool LocalPlayerState::Update(EngineCore& core, FlexKit::UpdateDispatcher& Dispatcher, double dT)
{
    debugCamera.Yaw(pi * dT / 2);

	return true;
}


/************************************************************************************************/


bool LocalPlayerState::PreDrawUpdate(EngineCore& core, UpdateDispatcher& Dispatcher, double dT)
{
    return true;
}


/************************************************************************************************/


bool LocalPlayerState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

	CameraHandle activeCamera = debugCamera;

	auto& scene				= game.scene;
	auto& transforms		= QueueTransformUpdateTask	(dispatcher);
	auto& cameras			= CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
	auto& orbitUpdate		= QueueOrbitCameraUpdateTask(dispatcher, transforms, cameras, debugCamera, framework.MouseState, dT);
	auto& cameraConstants	= MakeHeapCopy				(Camera::ConstantBuffer{}, core.GetTempMemory());
	auto& PVS				= GatherScene               (dispatcher, scene, activeCamera, core.GetTempMemory());
	auto& textureStreams	= base.streamingEngine.update(dispatcher);

	WorldRender_Targets targets = {
		GetCurrentBackBuffer(core.Window),
		base.depthBuffer
	};

	LighBufferDebugDraw debugDraw;
	debugDraw.constantBuffer = base.constantBuffer;
	debugDraw.renderTarget   = targets.RenderTarget;
	debugDraw.vertexBuffer	 = base.vertexBuffer;

	const SceneDescription sceneDesc = {
	    scene.GetPointLights(dispatcher, core.GetTempMemory()),
	    transforms,
	    cameras,
	    PVS,
    };

	ClearVertexBuffer(frameGraph, base.vertexBuffer);
	ClearVertexBuffer(frameGraph, base.textBuffer);

	ClearBackBuffer(frameGraph, targets.RenderTarget, 0.0f);
	ClearDepthBuffer(frameGraph, base.depthBuffer, 1.0f);

    base.render.updateLightBuffers(dispatcher, frameGraph, activeCamera, scene, sceneDesc, core.GetTempMemory(), &debugDraw);
	base.render.RenderDrawabledPBR_ForwardPLUS(dispatcher, frameGraph, PVS.GetData().solid, activeCamera, targets, sceneDesc, core.GetTempMemory());

    PresentBackBuffer(frameGraph, &core.Window);

	return true;
}


/************************************************************************************************/


bool LocalPlayerState::EventHandler(Event evt)
{
	eventMap.Handle(evt, [&](auto& evt)
		{
			debugCamera.HandleEvent(evt);
		});

	return true;
}


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
