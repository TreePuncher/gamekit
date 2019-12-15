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


void GameState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
}


/************************************************************************************************/


void GameState::PreDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
}


/************************************************************************************************/


void GameState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
}


/************************************************************************************************/


void GameState::PostDrawUpdate(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
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


void LocalPlayerState::Update(EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
    base.Update(core, dispatcher, dT);

    if (auto [suzanne000, res] = FindGameObject(game.scene, "Suzanne.000"); res)
        Yaw(*suzanne000, pi * dT);
}


/************************************************************************************************/


void LocalPlayerState::PreDrawUpdate(EngineCore& core, UpdateDispatcher& Dispatcher, double dT)
{
}


/************************************************************************************************/


void LocalPlayerState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

    CameraHandle activeCamera = debugCamera;

    SetCameraAspectRatio(activeCamera, GetWindowAspectRatio(core));

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

    if constexpr (true)
    {
        auto& depthPass     = base.render.DepthPrePass(dispatcher, frameGraph, activeCamera, PVS, targets.DepthTarget, core.GetTempMemory());
        auto& lighting      = base.render.UpdateLightBuffers(dispatcher, frameGraph, activeCamera, scene, sceneDesc, core.GetTempMemory(), &debugDraw);
        auto& deferredPass  = base.render.RenderPBR_ForwardPlus(dispatcher, frameGraph, depthPass, activeCamera, targets, sceneDesc, base.t, core.GetTempMemory());
    }
    else
        base.render.RenderPBR_GBufferPass(dispatcher, frameGraph, activeCamera, PVS, base.gbuffer, base.depthBuffer, core.GetTempMemory());

    // Draw Skeleton overlay
    
    if (auto [gameObject, res] = FindGameObject(scene, "object1"); res)
    {
        auto Skeleton       = GetSkeleton(*gameObject);
        auto node           = GetSceneNode(*gameObject);

        if (!Skeleton)
            return;

        LineSegments lines  = BuildSkeletonLineSet(Skeleton, node, core.GetTempMemory());

        const auto cameraConstants = GetCameraConstants(activeCamera);
        const float4x4 V = cameraConstants.View;
        const float4x4 P = cameraConstants.Proj;

        for (auto& line : lines)
        {
            const auto tempA = P * (V * float4{ line.A, 1 });
            const auto tempB = P * (V * float4{ line.B, 1 });

            line.A = tempA.xyz() / tempA.w;
            line.B = tempB.xyz() / tempB.w;
        }

        DrawShapes(
            DRAW_LINE_PSO,
            frameGraph,
            base.vertexBuffer,
            base.constantBuffer,
            targets.RenderTarget,
            core.GetTempMemory(),
            SSLineShape{ lines });
    }
}


/************************************************************************************************/


void LocalPlayerState::PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    PresentBackBuffer(frameGraph, &core.Window);
}


/************************************************************************************************/


void LocalPlayerState::EventHandler(Event evt)
{
    eventMap.Handle(evt, [&](auto& evt)
        {
            debugCamera.HandleEvent(evt);
        });

    switch (evt.InputSource)
    {
        case Event::Keyboard:
        {
            switch (evt.mData1.mKC[0])
            {
            case KC_U: // Reload Shaders
            {
                if (evt.Action == Event::Release)
                {
                    std::cout << "Reloading Shaders\n";
                    framework.core.RenderSystem.QueuePSOLoad(FORWARDDRAW);
                    framework.core.RenderSystem.QueuePSOLoad(LIGHTPREPASS);
                }
            }   break;
            }
        }   break;
    }
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
