#include "MultiplayerGameState.h"
#include "Packets.h"

#include <imgui.h>
#include <implot.h>

using namespace FlexKit;


/************************************************************************************************/


void UpdateLocalPlayer(GameObject& gameObject, const PlayerInputState& currentInputState, double dT)
{
    Apply(gameObject,
        [&](LocalPlayerView&        local,
            PlayerView&             player,
            CameraControllerView&   camera)
        {
            const ThirdPersonCamera::KeyStates tpc_keyState =
            {
                .forward    = currentInputState.forward > 0.0,
                .backward   = currentInputState.backward > 0.0,
                .left       = currentInputState.left > 0.0,
                .right      = currentInputState.right > 0.0,
                .up         = currentInputState.up > 0.0,
                .down       = currentInputState.down > 0.0,
            };

            const auto cameraState = camera.GetData().GetFrameState();

            local->inputHistory.push_back({0, {}, {}, {}, {}, currentInputState, cameraState });
            camera->Update(currentInputState.mousedXY, tpc_keyState, dT);

            player->forward     = (GetCameraControllerForwardVector(gameObject) * float3(1, 0, 1)).normal();
            player->position    = GetCameraControllerHeadPosition(gameObject);
        });
}


/************************************************************************************************/


bool HandleEvents(PlayerInputState& keyState, Event evt)
{
    if (evt.InputSource == FlexKit::Event::Keyboard)
    {
        auto state = evt.Action == Event::Pressed ? 1.0f : 0.0f;

        switch (evt.mData1.mINT[0])
        {
        case TPC_MoveForward:
            keyState.forward = state;
            return true;
        case TPC_MoveBackward:
            keyState.backward = state;
            return true;
        case TPC_MoveLeft:
            keyState.left = state;
            return true;
        case TPC_MoveRight:
            keyState.right = state;
            return true;
        case TPC_MoveUp:
            keyState.up = state;
            return true;
        case TPC_MoveDown:
            keyState.down = state;
            return true;
        case PLAYER_ACTION1:
            if(evt.Action == Event::Pressed)
                keyState.events.push_back(PlayerInputState::Event::Action1);
            return true;
        case PLAYER_ACTION2:
            if (evt.Action == Event::Pressed)
                keyState.events.push_back(PlayerInputState::Event::Action1);
            return true;
        case PLAYER_ACTION3:
            if (evt.Action == Event::Pressed)
                keyState.events.push_back(PlayerInputState::Event::Action1);
            return true;
        case PLAYER_ACTION4:
            if (evt.Action == Event::Pressed)
                keyState.events.push_back(PlayerInputState::Event::Action1);
        default:
            return false;
        }
    }

    return false;
}



/************************************************************************************************/


LocalGameState::LocalGameState(GameFramework& IN_framework, WorldStateMangagerInterface& IN_worldState, BaseState& IN_base) :
        FrameworkState      { IN_framework  },
        base                { IN_base },
        worldState          { IN_worldState },
        testParticleSystem  { IN_framework.core.GetBlockMemory() },
        emitters            { IN_framework.core.GetBlockMemory() },
        testAnimation       { IN_worldState.CreateGameObject() },
        particleEmitter     { IN_worldState.CreateGameObject() },
        IKTarget            { IN_worldState.CreateGameObject() },

        testAnimationResource   { LoadAnimation("TestRigAction", IN_framework.core.GetBlockMemory()) }
{
    //base.renderWindow.ToggleMouseCapture();
    auto& renderSystem = framework.core.RenderSystem;

    renderSystem.RegisterPSOLoader(INSTANCEPARTICLEDPASS,       { &renderSystem.Library.RSDefault, CreateParticleMeshInstancedPSO });
    renderSystem.RegisterPSOLoader(INSTANCEPARTICLEDEPTHDPASS,  { &renderSystem.Library.RSDefault, CreateParticleMeshInstancedDepthPSO });

    worldState.SetOnGameEventRecieved(
        [&](Event evt)
        {
            EventHandler(evt);
        }
    );


    particleEmitter.AddView<SceneNodeView<>>();
    auto& emitterView       = particleEmitter.AddView<ParticleEmitterView>(ParticleEmitterData{ &testParticleSystem, GetSceneNode(particleEmitter) });
    auto& emitterProperties = emitterView.GetData().properties;

    emitterProperties.emissionSpread    = 1.0f;
    emitterProperties.minEmissionRate   = 0;
    emitterProperties.maxEmissionRate   = 1000;

    Translate(particleEmitter, { 0, 10, 0 });

    auto& scene = worldState.GetScene();

    playerCharacterModel    = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), 8000);
    auto model              = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), 7894);

    auto& ikNodeView    = IKTarget.AddView<SceneNodeView<>>();
    auto& ikBrushView   = IKTarget.AddView<BrushView>(model, GetSceneNode(IKTarget));
    auto& ikTargetView  = IKTarget.AddView<FABRIKTargetView>(FABRIKTarget{ GetSceneNode(IKTarget), (iAllocator*)framework.core.GetBlockMemory() });

    Translate(IKTarget, { 0, 6.0f, 0 });

    testAnimation.AddView<SceneNodeView<>>();
    auto& brushView     = testAnimation.AddView<BrushView>(playerCharacterModel, GetSceneNode(testAnimation));
    auto& skeletonView  = testAnimation.AddView<SkeletonView>(playerCharacterModel, 8001);
    auto& IKController  = testAnimation.AddView<FABRIKView>(testAnimation);
    //auto& animatorView    = testAnimation.AddView<AnimatorView>(testAnimation);

    SetTransparent(testAnimation, true);
    brushView.SetTransparent(true);

    auto& materials = MaterialComponent::GetComponent();
    auto testAnimationMaterial = materials.CreateMaterial();
    materials.Add2Pass(testAnimationMaterial, PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
    testAnimation.AddView<MaterialComponentView>(testAnimationMaterial);
    SetMaterialHandle(testAnimation, testAnimationMaterial);


    auto parentMaterial     = materials.CreateMaterial();
    auto defaultMaterial    = materials.CreateMaterial(parentMaterial);

    materials.Add2Pass(parentMaterial, PassHandle{ GetCRCGUID(OIT_MCGUIRE) });

    materials.SetProperty(parentMaterial,   GetCRCGUID(PBR_ALBEDO),     float4{ 0.5f, 0.0f, 0.5f, 0.1f });
    materials.SetProperty(parentMaterial,   GetCRCGUID(PBR_SPECULAR),   float4{ 0.9f, 0.9f, 0.9f, 0.0f });

    for (size_t Y = 0; Y < 0; Y++)
        for (size_t X = 0; X < 20; X++)
        {
            auto& transparentObject = worldState.CreateGameObject();
            auto& sceneNodeView     = transparentObject.AddView<SceneNodeView<>>(float3{ 10, 0, 0 } + float3{ X * 1.0f,  0.0f, Y * 1.0f });
            auto& brushView         = transparentObject.AddView<BrushView>(playerCharacterModel,GetSceneNode(transparentObject));
            auto& materialView      = transparentObject.AddView<MaterialComponentView>(defaultMaterial);

            materialView.SetProperty(GetCRCGUID(PBR_ALBEDO), float4{ 1.0f / 20 * X, 1.0f / 20 * X, 1.0f / 40 * X * Y, 0.1f });

            SetMaterialHandle(transparentObject, materialView.handle);

            scene.AddGameObject(
                transparentObject,
                GetSceneNode(transparentObject));

            SetBoundingSphereFromMesh(transparentObject);
        }

    IKController.AddTarget(IKTarget);
    IKController.SetEndEffector(skeletonView.FindJoint("EndEffector"));
    //animatorView.Play(*testAnimationResource, true);

    brushView.GetBrush().Transparent    = true;
    brushView.GetBrush().Skinned        = true;

    scene.AddGameObject(testAnimation, GetSceneNode(testAnimation));
    scene.AddGameObject(IKTarget, GetSceneNode(IKTarget));

    SetBoundingSphereFromMesh(testAnimation);

    SetWorldPosition(particleEmitter, float3{ 0.0f, 40, 0.0f });
    Pitch(particleEmitter, float(pi / 2.0f));
}


/************************************************************************************************/


LocalGameState::~LocalGameState()
{
    base.framework.core.GetBlockMemory().release_allocation(worldState);
}


/************************************************************************************************/


UpdateTask* LocalGameState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    ProfileFunction();

    base.Update(core, dispatcher, dT);

    auto tasks = worldState.Update(core, dispatcher, dT);

    static float t = 0.0f;
    SetWorldPosition(particleEmitter, float3{ 100.0f * sin(t), 20, 100.0f * cos(t) });
    SetWorldPosition(IKTarget, float3{ 2.0f * cos(t), 4.0f * sin(t / 2.0f) + 8.0f, 4.0f * sin(t) });

    t += dT;

    return tasks.update;
}


/************************************************************************************************/


UpdateTask* LocalGameState::Draw(UpdateTask* updateTask, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    ProfileFunction();

    frameGraph.Resources.AddBackBuffer(base.renderWindow.GetBackBuffer());
    frameGraph.Resources.AddDepthBuffer(base.depthBuffer.Get());

    const CameraHandle activeCamera = worldState.GetActiveCamera();
    SetCameraAspectRatio(activeCamera, base.renderWindow.GetAspectRatio());

    auto& scene             = worldState.GetScene();
    auto& transforms        = QueueTransformUpdateTask(dispatcher);
    auto& cameras           = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
    auto& cameraConstants   = MakeHeapCopy(Camera::ConstantBuffer{}, core.GetTempMemory());

    if (updateTask)
        transforms.AddInput(*updateTask);

    auto tasks = worldState.DrawTasks(core, dispatcher, dT);

    for (auto task : tasks)
        cameras.AddOutput(*task);

    cameras.AddInput(transforms);

    auto& emitterTask            = UpdateParticleEmitters(dispatcher, dT);
    auto& particleSystemUpdate   = testParticleSystem.Update(dT, core.Threads, dispatcher);

    emitterTask.AddInput(transforms);
    emitterTask.AddOutput(particleSystemUpdate);

    WorldRender_Targets targets = {
        base.renderWindow.GetBackBuffer(),
        base.depthBuffer
    };

    ClearVertexBuffer   (frameGraph, base.vertexBuffer);
    ClearBackBuffer     (frameGraph, targets.RenderTarget, {0.0f, 0.0f, 0.0f, 0.0f});
    ClearDepthBuffer    (frameGraph, base.depthBuffer.Get(), 1.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    if (base.renderWindow.GetWH().Product() != 0)
    {
        auto& renderSystem = base.framework.core.RenderSystem;

        auto [triMesh, loaded] = FindMesh(7894);

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), 7894);


        DrawSceneDescription sceneDesc =
        {
            .camera = activeCamera,
            .scene  = scene,
            .dt     = dT,
            .t      = base.t,

            .gbuffer    = base.gbuffer,
            .reserveVB  = reserveVB,
            .reserveCB  = reserveCB,

            .debugDisplay   = DebugVisMode::Disabled,
            .BVHVisMode     = BVHVisMode::BVH,
            .debugDrawMode  = ClusterDebugDrawMode::BVH,

            .transformDependency    = transforms,
            .cameraDependency       = cameras,
            .additionalGbufferPasses  = {
                [&]()
                {
                    auto& particlePass  = testParticleSystem.DrawInstanceMesh(
                            testParticleSystem,
                            particleSystemUpdate,
                            dispatcher,
                            frameGraph,
                            triMesh,
                            reserveCB,
                            reserveVB,
                            activeCamera,
                            base.gbuffer,
                            targets.DepthTarget.Get());
                }},
            .additionalShadowPasses = {
                    [&, triMesh = triMesh](
                        ReserveConstantBufferFunction&  reserveCB,
                        ReserveVertexBufferFunction&    reserveVB,
                        ConstantBufferDataSet*          passConstants,
                        ResourceHandle*                 depthTargets,
                        const size_t                    targetCount,
                        const ResourceHandler&          resources,
                        Context&                        ctx,
                        iAllocator&                     allocator)
                    {
                        testParticleSystem.DrawInstanceMeshShadowPass(
                            triMesh,
                            reserveCB,
                            reserveVB,
                            passConstants,
                            depthTargets,
                            targetCount,
                            resources,
                            ctx,
                            allocator);
                    }
                }
        };

        auto drawnScene = base.render.DrawScene(
                                dispatcher,
                                frameGraph,
                                sceneDesc,
                                targets,
                                core.GetBlockMemory(),
                                core.GetTempMemoryMT());

        auto& feedbackPass = base.streamingEngine.TextureFeedbackPass(
                                dispatcher,
                                frameGraph,
                                activeCamera,
                                base.renderWindow.GetWH(),
                                drawnScene.passes,
                                drawnScene.skinnedDraws,
                                reserveCB,
                                reserveVB);
    }


    if(0)
    {
        // Draw Skeleton overlay
        const auto Skeleton = GetSkeleton(testAnimation);
        const auto pose     = GetPoseState(testAnimation);
        const auto node     = GetSceneNode(testAnimation);


        static double T = 0.0f;
        const size_t jointCount = GetJointCount(testAnimation);
        for (size_t I = 0; I < 0; I++)
        {
            auto jointPose = GetJointPose(testAnimation, JointHandle{ I });

            const float i = sin(T) / 2.0f + 0.5f;
            jointPose.r = Qlerp( Quaternion{ 0, 0, 0 }, Quaternion{ 0, 90, 0 }, i );

            SetJointPose(testAnimation, JointHandle{ I }, jointPose);
            //RotateJoint(testAnimation, JointHandle{ I }, Quaternion{ 0.0f, float(45.0f * dT), 0.0f });
        }

        T += dT;

        if (!Skeleton)
            return nullptr;

        const auto PV       = GetCameraConstants(activeCamera).PV;
        LineSegments lines  = DEBUG_DrawPoseState(*pose, node, core.GetTempMemory());

        Apply(testAnimation,
            [&](FABRIKView& view)
            {
                auto& jointPositions = view->Debug;

                if(jointPositions.size())
                    for (size_t J = 0; J < 3; ++J)
                    {
                        float3 A = jointPositions[J];
                        float3 B = jointPositions[J + 1];

                        LineSegment line;
                        line.A          = A;
                        line.AColour    = float3(1, 1, 1);
                        line.B          = B;
                        line.BColour    = float3(1, 1, 1);

                        lines.push_back(line);
                    }
            });

        // Transform to Device Coords
        for (auto& line : lines)
        {
            const auto tempA = PV * float4{ line.A, 1 };
            const auto tempB = PV * float4{ line.B, 1 };

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

    PresentBackBuffer(frameGraph, base.renderWindow);

    return nullptr;
}


/************************************************************************************************/


void LocalGameState::PostDrawUpdate(EngineCore& core, double dT)
{
    ProfileFunction();

    base.PostDrawUpdate(core, dT);
}


/************************************************************************************************/


bool LocalGameState::EventHandler(Event evt)
{
    if (worldState.EventHandler(evt))
        return true;

    switch (evt.InputSource)
    {
        case Event::E_SystemEvent:
        {
            switch (evt.Action)
            {
            case Event::InputAction::Resized:
            {
                const auto width    = (uint32_t)evt.mData1.mINT[0];
                const auto height   = (uint32_t)evt.mData2.mINT[0];
                base.Resize({ width, height });
            }   return true;
            case Event::InputAction::Exit:
                framework.quit = true;
                return true;
            default:
                break;
            }
        }   break;

        case Event::Keyboard:
        {
            if (evt.Action == Event::Release)
                switch (evt.mData1.mKC[0])
                {
                case KC_M:
                    base.renderWindow.ToggleMouseCapture();
                    return true;
                case KC_R:
                    /*
                    framework.GetRenderSystem().QueuePSOLoad(SHADINGPASS);
                    framework.GetRenderSystem().QueuePSOLoad(CREATECLUSTERLIGHTLISTS);
                    framework.GetRenderSystem().QueuePSOLoad(CREATECLUSTERS);
                    framework.GetRenderSystem().QueuePSOLoad(OITBLEND);
                    framework.GetRenderSystem().QueuePSOLoad(OITDRAW);
                    */

                    
                    framework.GetRenderSystem().QueuePSOLoad(VXGI_DRAWVOLUMEVISUALIZATION);
                    framework.GetRenderSystem().QueuePSOLoad(VXGI_CLEANUPVOXELVOLUMES);
                    framework.GetRenderSystem().QueuePSOLoad(VXGI_SAMPLEINJECTION);

                    return true;
                case KC_ESC:
                    framework.quit = true;
                    return true;
                };
        }   break;

        case Event::InputType::Local:
        {
            switch (evt.mData1.mINT[0])
            {
            case (int)PlayerEvents::PlayerDeath:
                // TODO: Game over screen
                framework.quit = true;
                break;
            }
        }break;

    }

    return false;
}


/************************************************************************************************/


void LocalGameState::OnGameEnd()
{

}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 Robert May

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
