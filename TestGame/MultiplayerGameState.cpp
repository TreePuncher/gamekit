#include "MultiplayerGameState.h"
#include "Packets.h"

#include <imgui.h>
#include <implot.h>

using namespace FlexKit;


/************************************************************************************************/


void UpdatePlayerState(GameObject& player, const PlayerInputState& currentInputState, double dT)
{
    Apply(player,
        [&](LocalPlayerView&        player,
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

            player.GetData().inputHistory.push_back({ {}, {}, {}, currentInputState, cameraState });
            camera.GetData().Update(currentInputState.mousedXY, tpc_keyState, dT);
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
        default:
            return false;
        }
    }

    return false;
}


/************************************************************************************************/


PlayerFrameState GetPlayerFrameState(GameObject& gameObject)
{
    PlayerFrameState out;

    Apply(
        gameObject,
        [&](LocalPlayerView& view)
        {
            const float3        pos = GetCameraControllerHeadPosition(gameObject);
            const Quaternion    q = GetCameraControllerOrientation(gameObject);

            out.pos = pos;
            out.orientation = q;
        });

    return out;
}


/************************************************************************************************/


RemotePlayerData* FindPlayer(MultiplayerPlayerID_t ID, RemotePlayerComponent& players)
{
    for (auto& player : players)
    {
        if (player.componentData.ID == ID)
            return &players[player.handle];
    }

    return nullptr;
}


/************************************************************************************************/


LocalGameState::LocalGameState(GameFramework& IN_framework, WorldStateMangagerInterface& IN_worldState, BaseState& IN_base) :
        FrameworkState  { IN_framework  },
        worldState      { IN_worldState },
        base            { IN_base }
{
    base.renderWindow.ToggleMouseCapture();
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

    return tasks.update;
}


/************************************************************************************************/


UpdateTask* LocalGameState::Draw(UpdateTask* updateTask, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    ProfileFunction();


    frameGraph.Resources.AddBackBuffer(base.renderWindow.GetBackBuffer());
    frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

    CameraHandle activeCamera = worldState.GetActiveCamera();
    SetCameraAspectRatio(activeCamera, base.renderWindow.GetAspectRatio());

    auto& scene             = worldState.GetScene();
    auto& transforms        = QueueTransformUpdateTask(dispatcher);
    auto& cameras           = CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
    auto& cameraConstants   = MakeHeapCopy(Camera::ConstantBuffer{}, core.GetTempMemory());

    if (updateTask)
        transforms.AddInput(*updateTask);

    cameras.AddInput(transforms);

    WorldRender_Targets targets = {
        base.renderWindow.GetBackBuffer(),
        base.depthBuffer
    };

    ClearVertexBuffer   (frameGraph, base.vertexBuffer);
    ClearBackBuffer     (frameGraph, targets.RenderTarget, {0.0f, 0.0f, 0.0f, 0.0f});
    ClearDepthBuffer    (frameGraph, base.depthBuffer, 1.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    if (base.renderWindow.GetWH().Product() != 0)
    {
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
        };

        auto& drawnScene = base.render.DrawScene(dispatcher, frameGraph, sceneDesc, targets, core.GetBlockMemory(), core.GetTempMemoryMT());

        base.streamingEngine.TextureFeedbackPass(
            dispatcher,
            frameGraph,
            activeCamera,
            base.renderWindow.GetWH(),
            drawnScene.PVS,
            reserveCB,
            reserveVB);
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
            }   break;

            case Event::InputAction::Exit:
                framework.quit = true;
                break;
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
                    break;
                case KC_ESC:
                    framework.quit = true;
                    break;
                };
        }   break;
    }
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
