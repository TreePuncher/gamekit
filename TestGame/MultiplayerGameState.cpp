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
                .x = currentInputState.X,
                .y = currentInputState.Y,
            };

            const auto cameraState = camera.GetData().GetFrameState();

            local->inputHistory.push_back({0, {}, {}, {}, {}, currentInputState, cameraState });
            camera->UpdateCharacter(currentInputState.mousedXY, tpc_keyState, dT);
            camera->UpdateCamera(currentInputState.mousedXY, tpc_keyState, dT);

            player->forward     = (GetCameraControllerForwardVector(gameObject) * float3(1, 0, 1)).normal();
            player->position    = GetCameraControllerHeadPosition(gameObject);
        });

    /*
    Apply(gameObject,
        [&](OrbitCameraBehavior& orbitCamera)
        {
            orbitCamera.Yaw(currentInputState.mousedXY.x);
            orbitCamera.Pitch(currentInputState.mousedXY.y);

            auto forward    = orbitCamera.GetForwardVector();
            auto right      = orbitCamera.GetRightVector();

            if (currentInputState.forward > 0.0)
                orbitCamera.TranslateWorld(forward * dT * 100);

            if (currentInputState.right > 0.0)
                orbitCamera.TranslateWorld(right * dT * 100);

            if (currentInputState.backward > 0.0)
                orbitCamera.TranslateWorld(-forward * dT * 100);

            if (currentInputState.left > 0.0)
                orbitCamera.TranslateWorld(-right * dT * 100);

            if (currentInputState.up)
                orbitCamera.TranslateWorld(float3(0, dT * 100, 0));

            if (currentInputState.down)
                orbitCamera.TranslateWorld(float3(0, dT * -100, 0));

        });
    */
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
            keyState.Y = state;
            return true;
        case TPC_MoveBackward:
            keyState.Y = -state;
            return true;
        case TPC_MoveLeft:
            keyState.X = -state;
            return true;
        case TPC_MoveRight:
            keyState.X = state;
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


LocalGameState::LocalGameState(GameFramework& IN_framework, WorldStateMangagerInterface& IN_worldState, BaseState& IN_base)
    : FrameworkState      { IN_framework  }
    , base                { IN_base }
    , worldState          { IN_worldState }
    , testParticleSystem  { IN_framework.core.GetBlockMemory() }
    , emitters            { IN_framework.core.GetBlockMemory() }
    //,   testAnimation       { IN_worldState.CreateGameObject() }
    , particleEmitter     { IN_worldState.CreateGameObject() }
    //,   IKTarget            { IN_worldState.CreateGameObject() }
    , runOnceDrawEvents   { IN_framework.core.GetBlockMemory() }//
    //,   testAnimationResource   { LoadAnimation("TestRigAction", IN_framework.core.GetBlockMemory()) }
{
    //base.PixCapture();
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
    auto& scene         = worldState.GetScene();
    auto& allocator     = IN_framework.core.GetTempMemory();
    
    auto res = scene.Query(allocator, Mut<SceneNodeView<>>{}, StringQuery{ "Light", "smolina" });

    pointLight1 = FlexKit::FindGameObject(worldState.GetScene(), "Light").value_or(nullptr);
    smolina     = FlexKit::FindGameObject(worldState.GetScene(), "smolina").value_or(nullptr);


    particleEmitter.AddView<SceneNodeView<>>();
    auto& emitterView       = particleEmitter.AddView<ParticleEmitterView>(ParticleEmitterData{ &testParticleSystem, GetSceneNode(particleEmitter) });
    auto& emitterProperties = emitterView.GetData().properties;

    emitterProperties.emissionSpread    = 1.0f;
    emitterProperties.minEmissionRate   = 0;
    emitterProperties.maxEmissionRate   = 1000;

    Translate(particleEmitter, { 0, 10, 0 });


    auto pointLightSearch = scene.Query(framework.core.GetTempMemory(), PointLightQuery{});

    for (auto& query : pointLightSearch)
    {
        auto& [pl] = query.value();
        pl.SetIntensity(pl.GetIntensity() * 10.0f);
    }

    //playerCharacterModel    = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), CharacterModelAsset);
    //auto model              = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), PlaceHolderAsset);

    //auto& ikNodeView    = IKTarget.AddView<SceneNodeView<>>();
    //auto& ikBrushView   = IKTarget.AddView<BrushView>(model, GetSceneNode(IKTarget));
    //auto& ikTargetView  = IKTarget.AddView<FABRIKTargetView>(FABRIKTarget{ GetSceneNode(IKTarget), (iAllocator*)framework.core.GetBlockMemory() });

    //Translate(IKTarget, { 0, 6.0f, 0 });

    //testAnimation.AddView<SceneNodeView<>>();
    //auto& brushView     = testAnimation.AddView<BrushView>(playerCharacterModel, GetSceneNode(testAnimation));
    //auto& skeletonView  = testAnimation.AddView<SkeletonView>(playerCharacterModel, CharacterSkeletonAsset);
    //auto& IKController  = testAnimation.AddView<FABRIKView>();
    //auto& animatorView    = testAnimation.AddView<AnimatorView>(testAnimation);

    //SetTransparent(testAnimation, true);
    //brushView.SetTransparent(true);

    //auto& materials = MaterialComponent::GetComponent();
    //auto defaultPBRMaterial = materials.CreateMaterial();
    //materials.Add2Pass(defaultPBRMaterial, PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
    //testAnimation.AddView<MaterialComponentView>(defaultPBRMaterial);

    //IKTarget.AddView<MaterialComponentView>(defaultPBRMaterial);

    //auto parentMaterial     = materials.CreateMaterial();
    //auto defaultMaterial    = materials.CreateMaterial(parentMaterial);

    //materials.Add2Pass(parentMaterial, PassHandle{ GetCRCGUID(OIT_MCGUIRE) });

    //materials.SetProperty(parentMaterial, GetCRCGUID(PBR_ALBEDO),     float4{ 0.5f, 0.0f, 0.5f, 0.1f });
    //materials.SetProperty(parentMaterial, GetCRCGUID(PBR_SPECULAR),   float4{ 0.9f, 0.9f, 0.9f, 0.0f });

    /*
    for (size_t Y = 0; Y < 0; Y++)
        for (size_t X = 0; X < 20; X++)
        {
            auto& transparentObject = worldState.CreateGameObject();
            auto& sceneNodeView     = transparentObject.AddView<SceneNodeView<>>(float3{ 10, 0, 0 } + float3{ X * 1.0f,  0.0f, Y * 1.0f });
            auto& brushView         = transparentObject.AddView<BrushView>(playerCharacterModel,GetSceneNode(transparentObject));
            auto& materialView      = transparentObject.AddView<MaterialComponentView>(materials.CreateMaterial(defaultMaterial));

            materialView.SetProperty(GetCRCGUID(PBR_ALBEDO), float4{ 1.0f / 20 * X, 1.0f / 20 * X, 1.0f / 40 * X * Y, 0.1f });

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

    SetVisable(IKTarget, false);

    SetBoundingSphereFromMesh(testAnimation);

    SetWorldPosition(particleEmitter, float3{ 0.0f, 40, 0.0f });
    Pitch(particleEmitter, float(pi / 2.0f));
    */

    runOnceDrawEvents.push_back([&]()
        {
            base.render.AddTask(
                [&](auto& frameGraph, auto& frameResources)
                {
                    base.render.BuildSceneGI(frameGraph, scene, frameResources.passes, frameResources.reserveCB);
                });
        });
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

    if (pointLight1 && move)
    {
        static float t      = 0.0f;
        static float3 XZ    = GetWorldPosition(*pointLight1);

        if(mode)
            SetWorldPosition(*pointLight1, float3{ XZ.x, 10 * sin(t) + 20, XZ.z });
        else
            SetWorldPosition(*pointLight1, float3{ 15.0f * sin(t), 2, 15.0f * cos(t) });

        t += dT;
    }


    if (smolina)
        Yaw(*smolina, (float)pi);

    //SetWorldPosition(particleEmitter, float3{ 100.0f * sin(t), 20, 100.0f * cos(t) });
    //SetWorldPosition(IKTarget, float3{ 2.0f * cos(t), 4.0f * sin(t / 2.0f) + 8.0f, 4.0f * sin(t) } + float3{ 30.0f, 0.0f, 10.0f });


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

    runOnceDrawEvents.Process();

    if (base.renderWindow.GetWH().Product() != 0)
    {
        auto& renderSystem = base.framework.core.RenderSystem;

        auto [triMesh, loaded] = FindMesh(7894);

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), 7894);


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
            .debugDrawMode  = ClusterDebugDrawMode::Clusters,

            .transformDependency    = transforms,
            .cameraDependency       = cameras,
            /*
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
                */
        };

        auto drawnScene = base.render.DrawScene(
                                dispatcher,
                                frameGraph,
                                sceneDesc,
                                targets,
                                core.GetBlockMemory(),
                                core.GetTempMemoryMT());

        base.streamingEngine.TextureFeedbackPass(
                                dispatcher,
                                frameGraph,
                                activeCamera,
                                base.renderWindow.GetWH(),
                                drawnScene.passes,
                                drawnScene.skinnedDraws,
                                reserveCB,
                                reserveVB);
    }


    if(false)
    {
        LineSegments lines(core.GetTempMemory());
#if 1
        auto wiggler = FlexKit::FindGameObject(worldState.GetScene(), "wiggle").value_or(nullptr);

        if (wiggler)
        {
            //worldState.GetScene().Query(base.framework.core.GetTempMemory(), ROStringQuery{ "wiggler" });
            // Draw Skeleton overlay
            const auto Skeleton = GetSkeleton(*wiggler);
            const auto pose     = GetPoseState(*wiggler);
            const auto node     = GetSceneNode(*wiggler);

            if (!Skeleton)
                return nullptr;

            lines = DEBUG_DrawPoseState(*pose, node, core.GetTempMemory());

            Apply(*wiggler,
                [&](FABRIKView& view)
                {
                    auto& jointPositions = view->Debug;

                    if (jointPositions.size())
                        for (size_t J = 0; J < 3; ++J)
                        {
                            float3 A = jointPositions[J];
                            float3 B = jointPositions[J + 1];

                            LineSegment line;
                            line.A = A;
                            line.AColour = float3(1, 1, 1);
                            line.B = B;
                            line.BColour = float3(1, 1, 1);

                            lines.push_back(line);
                        }
                });
#endif

            if (0)
            {
                const size_t divisions      = 64;
                const auto boundingSphere   = GetBoundingSphere(*smolina);
                const auto POS              = boundingSphere.xyz();
                const auto radius           = boundingSphere.w;
                const float Step            = 2.0f * (float)FlexKit::pi / divisions;

                auto lightHandle = GetPointLight(*pointLight1);
                const auto& light = PointLightComponent::GetComponent()[lightHandle];
                const auto pointLightPosition = GetPositionW(light.Position);

                auto f = GetFrustum(1.0f, (float)pi / 2.0f, 1.0f, light.R, pointLightPosition, Quaternion{ 0.0f,  90.0f, 0.0f });
                auto r = Intersects(f, boundingSphere);

                for (size_t I = 0; I < divisions; I++)
                {
                    const float3 V1 = { radius * cos(Step * (I + 1)),	0.0f, (radius * sin(Step * (I + 1))) };
                    const float3 V2 = { radius * cos(Step * I),		    0.0f, (radius * sin(Step * I)) };

                    auto color = r ? float3{ 1, 0, 1 } : float3{ 1, 1, 1 };

                    LineSegment L1;
                    L1.A = V1 + POS;
                    L1.B = V2 + POS;
                    L1.AColour = color;
                    L1.BColour = color;

                    LineSegment L2;
                    L2.A = float3{ V1.x, V1.z, 0 } + POS;
                    L2.B = float3{ V2.x, V2.z, 0 } + POS;
                    L2.AColour = color;
                    L2.BColour = color;

                    LineSegment L3;
                    L3.A = float3{ 0, V1.x, V1.z } + POS;
                    L3.B = float3{ 0, V2.x, V2.z } + POS;
                    L3.AColour = color;
                    L3.BColour = color;

                    lines.push_back(L1);
                    lines.push_back(L2);
                    lines.push_back(L3);
                }


                static const Quaternion Orientations[6] = {
                    Quaternion{   0,  90, 0 }, // Right
                    Quaternion{   0, -90, 0 }, // Left
                    Quaternion{ -90,   0, 0 }, // Top
                    Quaternion{  90,   0, 0 }, // Bottom
                    Quaternion{   0, 180, 0 }, // Backward
                    Quaternion{   0,   0, 0 }, // Forward
                };

                for (size_t I = 0; I < 6; I++)
                    [&](float AspectRatio, float FOV, float Near, float Far, float3 Position, Quaternion Q)
                {
                    float3 FTL(0);
                    float3 FTR(0);
                    float3 FBL(0);
                    float3 FBR(0);

                    FTL.z = -Far;
                    FTL.y = tan(FOV / 2) * Far;
                    FTL.x = -FTL.y * AspectRatio;

                    FTR = { -FTL.x,  FTL.y, FTL.z };
                    FBL = { FTL.x, -FTL.y, FTL.z };
                    FBR = { -FTL.x, -FTL.y, FTL.z };

                    float3 NTL(0);
                    float3 NTR(0);
                    float3 NBL(0);
                    float3 NBR(0);

                    NTL.z = -Near;
                    NTL.y = tan(FOV / 2) * Near;
                    NTL.x = -NTL.y * AspectRatio;

                    NTR = { -NTL.x,  NTL.y, NTL.z };
                    NBL = { NTL.x, -NTL.y, NTL.z };
                    NBR = { NTR.x, -NTR.y, NTR.z };

                    FTL = Position + Q * FTL;
                    FTR = Position + Q * FTR;
                    FBL = Position + Q * FBL;
                    FBR = Position + Q * FBR;

                    NTL = Position + Q * NTL;
                    NTR = Position + Q * NTR;
                    NBL = Position + Q * NBL;
                    NBR = Position + Q * NBR;

                    LineSegment L1;
                    L1.AColour = float3{ 1, 1, 1 };
                    L1.BColour = float3{ 1, 1, 1 };

                    L1.A = FTL;
                    L1.B = FTR;
                    lines.push_back(L1);

                    L1.A = FBL;
                    L1.B = FBR;
                    lines.push_back(L1);


                    L1.A = NTL;
                    L1.B = NTR;
                    lines.push_back(L1);

                    L1.A = NBL;
                    L1.B = NBR;
                    lines.push_back(L1);

                    L1.A = NBL;
                    L1.B = NTL;
                    lines.push_back(L1);

                    L1.A = NBR;
                    L1.B = NTR;
                    lines.push_back(L1);

                    L1.A = FBL;
                    L1.B = FTL;
                    lines.push_back(L1);

                    L1.A = FBR;
                    L1.B = FTR;
                    lines.push_back(L1);

                    L1.A = FTL;
                    L1.B = NTL;
                    lines.push_back(L1);

                    L1.A = FBL;
                    L1.B = NBL;
                    lines.push_back(L1);

                    L1.A = FTR;
                    L1.B = NTR;
                    lines.push_back(L1);

                    L1.A = FBR;
                    L1.B = NBR;
                    lines.push_back(L1);
                }(1.0f, (float)pi / 2.0f, 0.1f, 20.0f, pointLightPosition, Orientations[I]);
            }


            const auto PV = GetCameraConstants(activeCamera).PV;

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
    }

    base.DrawDebugHUD(core, dispatcher, frameGraph, reserveVB, reserveCB, targets.RenderTarget, dT);
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
    ProfileFunction();

    if (base.enableHud)
        base.debugUI.HandleInput(evt);

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
                case KC_C:
                    mode = !mode;
                    return true;
                case KC_V:
                    move = !move;
                    return true;
                case KC_M:
                    base.renderWindow.ToggleMouseCapture();
                    return true;
                case KC_R:
                    /*
                    framework.GetRenderSystem().QueuePSOLoad(SHADINGPASS);
                    framework.GetRenderSystem().QueuePSOLoad(OITBLEND);
                    framework.GetRenderSystem().QueuePSOLoad(OITDRAW);
                    */

                    framework.GetRenderSystem().QueuePSOLoad(CREATECLUSTERS);
                    framework.GetRenderSystem().QueuePSOLoad(CREATECLUSTERLIGHTLISTS);
                    framework.GetRenderSystem().QueuePSOLoad(GBUFFERPASS);
                    framework.GetRenderSystem().QueuePSOLoad(SHADINGPASS);
                    framework.GetRenderSystem().QueuePSOLoad(SHADOWMAPPASS);
                    framework.GetRenderSystem().QueuePSOLoad(SHADOWMAPANIMATEDPASS);

                    framework.GetRenderSystem().QueuePSOLoad(AVERAGELUMINANCE_BLOCK);
                    framework.GetRenderSystem().QueuePSOLoad(AVERAGELUMANANCE_GLOBAL);
                    framework.GetRenderSystem().QueuePSOLoad(TONEMAP);

                    //framework.GetRenderSystem().QueuePSOLoad(VXGI_DRAWVOLUMEVISUALIZATION);

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
