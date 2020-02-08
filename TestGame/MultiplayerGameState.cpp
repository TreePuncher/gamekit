#include "MultiplayerGameState.h"
#include "Packets.h"


using namespace FlexKit;


/************************************************************************************************/


GameState::GameState(
            GameFramework&  IN_framework, 
            BaseState&		IN_base) :
                FrameworkState	{ IN_framework },

                pScene          { IN_base.physics.CreateScene() },
        
                frameID			{ 0										},
                base			{ IN_base								},
                scene			{ IN_framework.core.GetBlockMemory()	} {}


/************************************************************************************************/


GameState::~GameState()
{
    iAllocator* allocator = base.framework.core.GetBlockMemory();
    auto& entities = scene.sceneEntities;

    while(entities.size())
    {
        auto* entityGO = SceneVisibilityView::GetComponent()[entities.back()].entity;
        scene.RemoveEntity(*entityGO);

        entityGO->Release();
        allocator->free(entityGO);
    }

    scene.ClearScene();
    base.physics.ReleaseScene(pScene);
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
    eventMap.MapKeyToEvent(KEYCODES::KC_Q, OCE_MoveDown);
    eventMap.MapKeyToEvent(KEYCODES::KC_E, OCE_MoveUp);


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
    auto& pointLightGather  = scene.GetPointLights      (dispatcher, core.GetTempMemory());
    auto& transforms		= QueueTransformUpdateTask	(dispatcher);
    auto& cameras			= CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
    auto& orbitUpdate		= QueueOrbitCameraUpdateTask(dispatcher, debugCamera, framework.MouseState, dT);
    auto& cameraConstants	= MakeHeapCopy				(Camera::ConstantBuffer{}, core.GetTempMemory());
    auto& PVS				= GatherScene               (dispatcher, scene, activeCamera, core.GetTempMemory());

    /*
    orbitUpdate.AddOutput(transforms);
    orbitUpdate.AddOutput(cameras);

    transforms.AddOutput(cameras);
    transforms.AddOutput(pointLightGather);

    cameras.AddOutput(pointLightGather);
    cameras.AddOutput(PVS);
    */

    transforms.AddInput(orbitUpdate);

    cameras.AddInput(transforms);
    cameras.AddInput(orbitUpdate);

    PVS.AddInput(transforms);
    PVS.AddInput(cameras);

    pointLightGather.AddInput(transforms);
    pointLightGather.AddInput(cameras);

    WorldRender_Targets targets = {
        core.Window.backBuffer,
        base.depthBuffer
    };

    LighBufferDebugDraw debugDraw;
    debugDraw.constantBuffer = base.constantBuffer;
    debugDraw.renderTarget   = targets.RenderTarget;
    debugDraw.vertexBuffer	 = base.vertexBuffer;

    const SceneDescription sceneDesc = {
        activeCamera,
        pointLightGather,
        transforms,
        cameras,
        PVS,
    };

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    
    ClearBackBuffer(frameGraph, targets.RenderTarget, 0.0f);
    ClearDepthBuffer(frameGraph, base.depthBuffer, 1.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    switch(renderMode)
    {
    case RenderMode::ForwardPlus:
    {
        //auto& depthPass       = base.render.DepthPrePass(dispatcher, frameGraph, activeCamera, PVS, targets.DepthTarget, core.GetTempMemory());
        //auto& lighting        = base.render.UpdateLightBuffers(dispatcher, frameGraph, activeCamera, scene, sceneDesc, core.GetTempMemory(), &debugDraw);
        //auto& deferredPass    = base.render.RenderPBR_ForwardPlus(dispatcher, frameGraph, depthPass, activeCamera, targets, sceneDesc, base.t, base.irradianceMap, core.GetTempMemory());
    }   break;
    case RenderMode::Deferred:
    {
        AddGBufferResource(base.gbuffer, frameGraph);
        ClearGBuffer(base.gbuffer, frameGraph);
        base.render.RenderPBR_GBufferPass(
            dispatcher,
            frameGraph,
            sceneDesc,
            activeCamera,
            PVS,
            base.gbuffer,
            base.depthBuffer,
            reserveCB,
            core.GetTempMemory());

        base.render.RenderPBR_IBL_Deferred(
            dispatcher,
            frameGraph,
            sceneDesc,
            activeCamera,
            targets.RenderTarget,
            base.depthBuffer,
            base.irradianceMap,
            base.GGXMap,
            base.gbuffer,
            reserveCB,
            reserveVB,
            base.t,
            core.GetTempMemory());
        //base.render.RenderPBR_DeferredShade(dispatcher, frameGraph, sceneDesc, activeCamera, pointLightGather, base.gbuffer, base.depthBuffer, targets.RenderTarget, base.cubeMap, base.vertexBuffer, base.t, core.GetTempMemory());
    }   break;
    case RenderMode::ComputeTiledDeferred:
    {

        ComputeTiledDeferredShadeDesc desc =
        {
            pointLightGather,
            base.gbuffer,
            base.depthBuffer,
            targets.RenderTarget,
            activeCamera,
        };


        AddGBufferResource(base.gbuffer, frameGraph);
        ClearGBuffer(base.gbuffer, frameGraph);
        base.render.RenderPBR_GBufferPass(
            dispatcher,
            frameGraph,
            sceneDesc,
            activeCamera,
            PVS,
            base.gbuffer,
            base.depthBuffer,
            reserveCB,
            core.GetTempMemory());

        base.render.UpdateLightBuffers(
            dispatcher,
            frameGraph,
            activeCamera,
            scene,
            sceneDesc,
            reserveCB,
            core.GetTempMemory(),
            &debugDraw);

        base.render.RenderPBR_ComputeDeferredTiledShade(
            dispatcher,
            frameGraph,
            reserveCB,
            desc);
    }
    }
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

        FK_ASSERT(0);

        /*
        DrawShapes(
            DRAW_LINE_PSO,
            frameGraph,
            base.vertexBuffer,
            base.constantBuffer,
            targets.RenderTarget,
            core.GetTempMemory(),
            SSLineShape{ lines });
         */
    }

    framework.stats.objectsDrawnLastFrame = PVS.GetData().solid.size();
}


/************************************************************************************************/


void LocalPlayerState::PostDrawUpdate(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    framework.DrawDebugHUD(dT, base.vertexBuffer, frameGraph);
    PresentBackBuffer(frameGraph, &core.Window);
}


/************************************************************************************************/


bool LocalPlayerState::EventHandler(Event evt)
{
    bool handled = eventMap.Handle(evt, [&](auto& evt)
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
                    framework.core.RenderSystem.QueuePSOLoad(SHADINGPASS);
                    framework.core.RenderSystem.QueuePSOLoad(ENVIRONMENTPASS);
                    framework.core.RenderSystem.QueuePSOLoad(COMPUTETILEDSHADINGPASS);
                }
            }   return true;
            case KC_P: // Reload Shaders
            {
                if (evt.Action == Event::Release)
                {
                    if (renderMode == RenderMode::Deferred)
                        renderMode = RenderMode::ForwardPlus;
                    else
                        renderMode = RenderMode::Deferred;
                }
            }   return true;
            default:
                return handled;
            }
        }   break;
    }

    return handled;
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
