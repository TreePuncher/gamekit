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


LocalPlayerState::LocalPlayerState(
    GameFramework&      IN_framework,
    BaseState&          IN_base,
    GameState&          IN_game) :
        FrameworkState	    { IN_framework							                            },
        game			    { IN_game								                            },
        base			    { IN_base								                            },
        eventMap		    { IN_framework.core.GetBlockMemory()	                            },
        netInputObjects	    { IN_framework.core.GetBlockMemory()	                            },
        thirdPersonCamera   { CreateThirdPersonCameraController(IN_game.pScene, IN_framework.core.GetBlockMemory())   }
{
    eventMap.MapKeyToEvent(KEYCODES::KC_W, TPC_MoveForward);
    eventMap.MapKeyToEvent(KEYCODES::KC_S, TPC_MoveBackward);
    eventMap.MapKeyToEvent(KEYCODES::KC_A, TPC_MoveLeft);
    eventMap.MapKeyToEvent(KEYCODES::KC_D, TPC_MoveRight);
    eventMap.MapKeyToEvent(KEYCODES::KC_Q, TPC_MoveDown);
    eventMap.MapKeyToEvent(KEYCODES::KC_E, TPC_MoveUp);
}


/************************************************************************************************/


void LocalPlayerState::Update(EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
    base.Update(core, dispatcher, dT);
}


/************************************************************************************************/


void LocalPlayerState::PreDrawUpdate(EngineCore& core, UpdateDispatcher& Dispatcher, double dT)
{
}


/************************************************************************************************/


void LocalPlayerState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

    CameraHandle activeCamera = GetCameraControllerCamera(thirdPersonCamera);

    SetCameraAspectRatio(activeCamera, GetWindowAspectRatio(core));

    auto& scene				= game.scene;
    auto& pointLightGather  = scene.GetPointLights      (dispatcher, core.GetTempMemory());
    auto& transforms		= QueueTransformUpdateTask	(dispatcher);
    auto& cameras			= CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
    auto& cameraConstants	= MakeHeapCopy				(Camera::ConstantBuffer{}, core.GetTempMemory());
    auto& PVS				= GatherScene               (dispatcher, scene, activeCamera, core.GetTempMemory());
    auto& skinnedObjects    = GatherSkinned             (dispatcher, scene, activeCamera, core.GetTempMemory());
    auto& updatedPoses      = UpdatePoses               (dispatcher, skinnedObjects, core.GetTempMemory());
    auto& cameraControllers = UpdateThirdPersonCameraControllers(dispatcher, framework.MouseState.Normalized_dPos, dT);

    transforms.AddInput(cameraControllers);

    cameras.AddInput(cameraControllers);
    cameras.AddInput(transforms);

    PVS.AddInput(transforms);
    PVS.AddInput(cameras);

    skinnedObjects.AddInput(cameras);
    updatedPoses.AddInput(skinnedObjects);

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
        skinnedObjects
    };

    frameGraph.AddRenderTarget(base.temporaryBuffers[0]);
    frameGraph.AddRenderTarget(base.temporaryBuffers[1]);
    frameGraph.AddRenderTarget(base.temporaryBuffers_2Channel[0]);
    frameGraph.AddRenderTarget(base.temporaryBuffers_2Channel[1]);
    frameGraph.AddRenderTarget(base.temporaryBuffers[1]);

    ClearVertexBuffer   (frameGraph, base.vertexBuffer);
    ClearBackBuffer     (frameGraph, base.temporaryBuffers[0], 0.0f);
    ClearDepthBuffer    (frameGraph, base.depthBuffer, 1.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    if(core.Window.WH.Product() != 0)
    {
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
                base.gbuffer,
                base.depthBuffer,
                reserveCB,
                core.GetTempMemory());

            base.render.RenderPBR_IBL_Deferred(
                dispatcher,
                frameGraph,
                sceneDesc,
                activeCamera,
                base.temporaryBuffers[0],
                base.depthBuffer,
                base.irradianceMap,
                base.GGXMap,
                base.gbuffer,
                reserveCB,
                reserveVB,
                base.t,
                core.GetTempMemory());

            base.render.BilateralBlur(
                frameGraph,
                base.temporaryBuffers[0],
                base.temporaryBuffers[1],
                base.temporaryBuffers_2Channel[0],
                base.temporaryBuffers_2Channel[1],
                targets.RenderTarget,
                base.gbuffer,
                base.depthBuffer,
                reserveCB,
                reserveVB,
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
            auto Skeleton = GetSkeleton(*gameObject);
            auto pose     = GetPoseState(*gameObject);
            auto node     = GetSceneNode(*gameObject);

            //RotateJoint(*gameObject, JointHandle(0), Quaternion{ 0, T, 0 });

            for (size_t I = 0; I < 5; I++)
            {
                JointHandle joint{ I };

                auto jointPose = GetJointPose(*gameObject, joint);
                jointPose.r = Quaternion{ 0, 0, (float)(T) * 90 };
                SetJointPose(*gameObject, joint, jointPose);
            }

            T += dT;

            if (!Skeleton)
                return;

            LineSegments lines = DEBUG_DrawPoseState(*pose, node, core.GetTempMemory());
            //LineSegments lines = BuildSkeletonLineSet(Skeleton, node, core.GetTempMemory());

            const auto cameraConstants = GetCameraConstants(activeCamera);

            for (auto& line : lines)
            {
                const auto tempA = cameraConstants.PV * float4{ line.A, 1 };
                const auto tempB = cameraConstants.PV * float4{ line.B, 1 };

                if (tempA.w <= 0 || tempB.w <= 0)
                {
                    line.A = { 0, 0, 0 };
                    line.B = { 0, 0, 0 };
                }
                else
                {
                    line.A = tempA.xyz() / tempA.w;
                    line.B = tempB.xyz() / tempB.w;
                }
            }

            DrawShapes(
                DRAW_LINE_PSO,
                frameGraph,
                reserveVB,
                reserveCB,
                targets.RenderTarget,
                core.GetTempMemory(),
                LineShape{ lines });
        }
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
            HandleEvents(thirdPersonCamera, evt);
        });

    switch (evt.InputSource)
    {
        case Event::E_SystemEvent:
        {
            switch (evt.Action)
            {
            case Event::InputAction::Resized:
            {
                const auto width  = (uint32_t)evt.mData1.mINT[0];
                const auto height = (uint32_t)evt.mData2.mINT[0];
                base.Resize({ width, height });
            }   break;
            default:
                break;
            }
        }   break;
        case Event::Keyboard:
        {
            switch (evt.mData1.mKC[0])
            {
            case KC_SPACE: // Reload Shaders
            {
                if (evt.Action == Event::Pressed)
                {
                    auto pos        = GetCameraControllerHeadPosition(thirdPersonCamera);
                    auto forward    = GetCameraControllerForwardVector(thirdPersonCamera);

                    // Load Model
                    const AssetHandle model = 1002;
                    auto [triMesh, loaded] = FindMesh("Cube1x1x1");

                    auto& allocator = framework.core.GetBlockMemory();

                    auto  rigidBody = base.physics.CreateRigidBodyCollider(game.pScene, PxShapeHandle{ 1 }, pos + forward * 20);
                    auto& dynamicBox = allocator.allocate<GameObject>();

                    dynamicBox.AddView<RigidBodyView>(rigidBody, game.pScene);
                    auto dynamicNode = GetRigidBodyNode(dynamicBox);
                    dynamicBox.AddView<DrawableView>(triMesh, dynamicNode);
                    game.scene.AddGameObject(dynamicBox, dynamicNode);

                    ApplyForce(dynamicBox, forward * 100);

                    SetMaterialParams(
                        dynamicBox,
                        float3(0.5f, 0.5f, 0.5f), // albedo
                        (rand() % 1000) / 1000.0f, // specular
                        1.0f, // IOR
                        (rand() % 1000) / 1000.0f,
                        (rand() % 1000) / 1000.0f,
                        0.0f);
                }
            }   break;
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
                    framework.core.RenderSystem.QueuePSOLoad(BILATERALBLURPASSHORIZONTAL);
                    framework.core.RenderSystem.QueuePSOLoad(BILATERALBLURPASSVERTICAL);

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
