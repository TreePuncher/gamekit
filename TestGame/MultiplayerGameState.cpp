#include "MultiplayerGameState.h"
#include "Packets.h"

#include <imgui.h>
#include <implot.h>

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


UpdateTask* GameState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	return base.Update(core, dispatcher, dT);
}


/************************************************************************************************/


UpdateTask* GameState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    return nullptr;
}


/************************************************************************************************/


void GameState::PostDrawUpdate(EngineCore& core, double dT)
{
	base.PostDrawUpdate(core, dT);
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
    Apply(thirdPersonCamera,
        [&](CameraControllerView& camera)
        {
            camera.GetData().SetPosition({ 100, 15, 0 });
        });

	eventMap.MapKeyToEvent(KEYCODES::KC_W, TPC_MoveForward);
	eventMap.MapKeyToEvent(KEYCODES::KC_S, TPC_MoveBackward);
	eventMap.MapKeyToEvent(KEYCODES::KC_A, TPC_MoveLeft);
	eventMap.MapKeyToEvent(KEYCODES::KC_D, TPC_MoveRight);
	eventMap.MapKeyToEvent(KEYCODES::KC_Q, TPC_MoveDown);
	eventMap.MapKeyToEvent(KEYCODES::KC_E, TPC_MoveUp);

    base.framework.GetRenderSystem().DEBUG_AttachPIX();
}


/************************************************************************************************/


UpdateTask* LocalPlayerState::Update(EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
    ProfileFunction();

    base.Update(core, dispatcher, dT);
    base.debugUI.Update(base.renderWindow, core, dispatcher, dT);

    ImGui::NewFrame();

    if (showDebugMenu)
    {
        ImGui::Begin("Debug Options");

        if (ImGui::Button("Animated Camera"))
            animateCamera = !animateCamera;

        if (ImGui::Button("Toggle Stats"))
            showDebugStats = !showDebugStats;

        if (ImGui::Button("Toggle Profiler"))
            drawProfiler = !drawProfiler;

        if (ImGui::BeginMenu("Plot Menu"))
        {
            if (ImGui::MenuItem("Frame Time"))
                drawFrameChart = !drawFrameChart;
            if (ImGui::MenuItem("Shading Time"))
                drawShadingPlot = !drawShadingPlot;
            if (ImGui::MenuItem("Feedback Time"))
                drawFeedbackPlot = !drawFeedbackPlot;
            if (ImGui::MenuItem("Present Time"))
                drawPresentWaitTime = !drawPresentWaitTime;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug Vis Menu"))
        {
            if (ImGui::BeginMenu("Scene Vis"))
            {
                if (ImGui::MenuItem("BVH"))
                {
                    renderMode = DebugVisMode::BVHVIS;
                    bvhVisMode = BVHVisMode::BVH;
                }
                if (ImGui::MenuItem("AABB"))
                {
                    renderMode = DebugVisMode::BVHVIS;
                    bvhVisMode = BVHVisMode::BoundingVolumes;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Deferred Shading VIS"))
            {
                if (ImGui::MenuItem("BVH"))
                {
                    renderMode = DebugVisMode::ClusterVIS;
                    debugDrawMode = ClusterDebugDrawMode::BVH;
                }
                if (ImGui::MenuItem("Clusters"))
                {
                    renderMode = DebugVisMode::ClusterVIS;
                    debugDrawMode = ClusterDebugDrawMode::Clusters;
                }
                if (ImGui::MenuItem("Lights"))
                {
                    renderMode = DebugVisMode::ClusterVIS;
                    debugDrawMode = ClusterDebugDrawMode::Lights;
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Clear"))
            {
                renderMode = DebugVisMode::Disabled;
            }
            ImGui::EndMenu();
        }

        ImGui::End();
    }

    if (drawFrameChart || drawShadingPlot || drawFeedbackPlot || drawPresentWaitTime)
    {
        if(ImGui::Begin("Plots"))
        {
            ImPlot::BeginPlot("Time Plots");

            if(drawFrameChart)
            {
                double Y_values[120];
                double X_values[120];
                int valueCount = 0;

                for (auto time : framework.stats.frameTimes)
                {
                    Y_values[valueCount] = time.duration;
                    X_values[valueCount] = valueCount;

                    valueCount++;
                }

                ImPlot::PlotLine("Update Times", X_values, Y_values, valueCount);
            }

            if (drawShadingPlot)
            {
                double Y_values[120];
                double X_values[120];
                int valueCount = 0;

                for (auto time : framework.stats.shadingTimes)
                {
                    Y_values[valueCount] = time.duration;
                    X_values[valueCount] = valueCount;

                    valueCount++;
                }

                ImPlot::PlotLine("Deferred Shading", X_values, Y_values, valueCount);
            }

            if (drawFeedbackPlot)
            {
                double Y_values[120];
                double X_values[120];
                int valueCount = 0;

                for (auto time : framework.stats.feedbackTimes)
                {
                    Y_values[valueCount] = time.duration;
                    X_values[valueCount] = valueCount;

                    valueCount++;
                }

                ImPlot::PlotShaded("texture feedback", X_values, Y_values, valueCount);
            }

            if(drawPresentWaitTime)
            {
                double Y_values[120];
                double X_values[120];
                int valueCount = 0;

                for (auto time : framework.stats.presentTimes)
                {
                    Y_values[valueCount] = time.duration;
                    X_values[valueCount] = valueCount;

                    valueCount++;
                }

                ImPlot::PlotLine("texture feedback", X_values, Y_values, valueCount);
            }

            ImPlot::EndPlot();
            ImGui::End();
        }
    }

    if(showDebugStats)
        base.DEBUG_PrintDebugStats(core);

    if(drawProfiler)
        profiler.DrawProfiler(core.GetTempMemory());

    ImGui::Render();

    if(animateCamera)
        Apply(thirdPersonCamera,
            [&](CameraControllerView& camera)
            {
                camera.GetData().Yaw(dT * pi);

                camera.GetData().SetPosition(
                    lerp(
                        float3{-100, 15, 0 },
                        float3{ 100, 15, 0 },
                        std::sin(T) / 2.0f + 0.5f));

                T += dT;
            });

    return nullptr;
}


/************************************************************************************************/


UpdateTask* LocalPlayerState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    ProfileFunction();

	frameGraph.Resources.AddBackBuffer(base.renderWindow.GetBackBuffer());
	frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

	CameraHandle activeCamera = GetCameraControllerCamera(thirdPersonCamera);
	SetCameraAspectRatio(activeCamera, base.renderWindow.GetAspectRatio());

	float2 mouse_dPos       = base.renderWindow.mouseState.Normalized_dPos;

	auto& scene				= game.scene;

	auto& transforms		= QueueTransformUpdateTask(dispatcher);
	auto& cameras			= CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
	auto& cameraConstants	= MakeHeapCopy(Camera::ConstantBuffer{}, core.GetTempMemory());
	auto& cameraControllers = UpdateThirdPersonCameraControllers(dispatcher, mouse_dPos, dT);

	transforms.AddInput(cameraControllers);

	cameras.AddInput(cameraControllers);
	cameras.AddInput(transforms);

	WorldRender_Targets targets = {
		base.renderWindow.GetBackBuffer(),
		base.depthBuffer
	};

	ClearVertexBuffer   (frameGraph, base.vertexBuffer);
	ClearBackBuffer     (frameGraph, targets.RenderTarget, 0.0f);
	ClearDepthBuffer    (frameGraph, base.depthBuffer, 1.0f);

	auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
	auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

	if(base.renderWindow.GetWH().Product() != 0)
	{
        DrawSceneDescription scene =
        {
            .camera = activeCamera,
            .scene  = game.scene,
            .dt     = dT,
            .t      = base.t,

            .gbuffer    = base.gbuffer,
            .reserveVB  = reserveVB,
            .reserveCB  = reserveCB,

            .debugDisplay           = renderMode,
            .BVHVisMode             = bvhVisMode,
            .debugDrawMode          = debugDrawMode,

            .transformDependency    = transforms,
            .cameraDependency       = cameras,
        };

        auto& drawnScene = base.render.DrawScene(dispatcher, frameGraph, scene, targets, core.GetBlockMemory(), core.GetTempMemoryMT());

        base.streamingEngine.TextureFeedbackPass(
            dispatcher,
            frameGraph,
            activeCamera,
            base.renderWindow.GetWH(),
            drawnScene.PVS,
            reserveCB,
            reserveVB);

	    // Draw Skeleton overlay
	    if (auto [gameObject, res] = FindGameObject(game.scene, "Cylinder"); res)
	    {
		    auto Skeleton = GetSkeleton(*gameObject);
		    auto pose     = GetPoseState(*gameObject);
		    auto node     = GetSceneNode(*gameObject);

		    //RotateJoint(*gameObject, JointHandle(0), Quaternion{ 0, T, 0 });

            /*
		    for (size_t I = 0; I < 5; I++)
		    {
			    JointHandle joint{ I };

			    auto jointPose = GetJointPose(*gameObject, joint);
			    jointPose.r = Quaternion{ 0, 0, (float)(T) * 90 };
			    SetJointPose(*gameObject, joint, jointPose);
		    }
            */

		    T += dT;

            if (!Skeleton)
                return nullptr;

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

            /*
		        DrawShapes(
			    DRAW_LINE_PSO,
			    frameGraph,
			    reserveVB,
			    reserveCB,
			    targets.RenderTarget,
			    core.GetTempMemory(),
			    LineShape{ lines });
            */
	    }
    }

	//framework.stats.objectsDrawnLastFrame = PVS.GetData().solid.size();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, base.renderWindow.GetBackBuffer());

    const auto shadingStats         = base.render.GetTimingValues();
    const auto texturePassTime      = base.streamingEngine.debug_GetPassTime();
    const auto textureUpdateTime    = base.streamingEngine.debug_GetUpdateTime();

    framework.stats.shadingTimes.push_back(
        GameFramework::TimePoint{
                .T          = framework.runningTime,
                .duration   = shadingStats.shadingPass
        });


    framework.stats.feedbackTimes.push_back(
        GameFramework::TimePoint{
                .T          = framework.runningTime,
                .duration   = texturePassTime
        });

	PresentBackBuffer(frameGraph, base.renderWindow);

    return nullptr;
}


/************************************************************************************************/


void LocalPlayerState::PostDrawUpdate(EngineCore& core, double dT)
{
    ProfileFunction();

	base.PostDrawUpdate(core, dT);
}


/************************************************************************************************/


bool LocalPlayerState::EventHandler(Event evt)
{
    ProfileFunction();

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

			case Event::InputAction::Exit:
				framework.quit = true;
				break;
			default:
				break;
			}
		}   break;
		case Event::Keyboard:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_U: // Reload Shaders
			{
				if (evt.Action == Event::Release)
				{
					std::cout << "Reloading Shaders\n";
                    framework.core.RenderSystem.QueuePSOLoad(LIGHTBVH_DEBUGVIS_PSO);
                    framework.core.RenderSystem.QueuePSOLoad(CLUSTER_DEBUGVIS_PSO);
                    framework.core.RenderSystem.QueuePSOLoad(CREATECLUSTERS);
                    framework.core.RenderSystem.QueuePSOLoad(CREATECLUSTERLIGHTLISTS);
                    framework.core.RenderSystem.QueuePSOLoad(SHADINGPASS);
                }
			}   return true;
			case KC_K:
			{
				if (evt.Action == Event::Release)
				{
                    switch (renderMode)
                    {
                    case DebugVisMode::BVHVIS:
                        renderMode = DebugVisMode::ClusterVIS;  break;
                    case DebugVisMode::ClusterVIS:
                        renderMode = DebugVisMode::Disabled;  break;
                    case DebugVisMode::Disabled:
                        renderMode = DebugVisMode::BVHVIS;  break;
                    }
				}
			}   return true;
            case KC_L:
            {
                if (evt.Action == Event::Release)

                    if (renderMode == DebugVisMode::BVHVIS)
                    {
                        switch (bvhVisMode)
                        {
                        case BVHVisMode::BVH:
                            bvhVisMode = BVHVisMode::BoundingVolumes;
                            break;
                        case BVHVisMode::BoundingVolumes:
                            bvhVisMode = BVHVisMode::Both;
                            break;
                        case BVHVisMode::Both:
                            bvhVisMode = BVHVisMode::BVH;
                            break;
                        }
                    }
                    else if (renderMode == DebugVisMode::ClusterVIS)
                    {
                        switch (debugDrawMode)
                        {
                        case ClusterDebugDrawMode::BVH:
                            debugDrawMode = ClusterDebugDrawMode::Lights;
                            break;
                        case ClusterDebugDrawMode::Lights:
                            debugDrawMode = ClusterDebugDrawMode::Clusters;
                            break;
                        case ClusterDebugDrawMode::Clusters:
                            debugDrawMode = ClusterDebugDrawMode::BVH;
                            break;
                        }
                    }
            }   return true;
			case KC_M:
				if (evt.Action == Event::Release)
					base.renderWindow.EnableCaptureMouse(!base.renderWindow.mouseCapture);
				break;
            case KC_P: // Reload Shaders
            {
                if (evt.Action == Event::Release)
                {
                    if (captureInProgress)
                    {
                        FK_LOG_INFO("Ending Pix Capture");
                        captureInProgress = !base.framework.GetRenderSystem().DEBUG_EndPixCapture();
                    }
                    else
                    {
                        FK_LOG_INFO("Beginning Pix Capture");
                        captureInProgress = base.framework.GetRenderSystem().DEBUG_BeginPixCapture();
                    }
                }
            }   break;
            case KC_F1:
                if (evt.Action == Event::Release)
                    showDebugMenu = !showDebugMenu;
			default:
				return handled;
			}
		}   break;
	}

    if (!handled)
        return base.debugUI.HandleInput(evt);
    else
	    return true;
}


/**********************************************************************

Copyright (c) 2020 Robert May

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
