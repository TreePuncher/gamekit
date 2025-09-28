#include "pch.h"
#include "level.hpp"
#include "AnimationTest.hpp"
#include "AnimationComponents.hpp"
#include <KeyValueIDs.hpp>
#include <SceneLoadingContext.hpp>
#include <imgui.h>
#include <PhysicsDebugVis.hpp>
#include <TriggerSlotIDs.hpp>
#include <ScriptingRuntime.hpp>
#include <fmt/format.h>
#include <RenderSystemInterface.hpp>

#include "vkWin32Surface.hpp"


using namespace FlexKit;

#define USEVULKAN 1

/************************************************************************************************/

class PlayerAnimationController : public FlexKit::IAnimatorController
{
public:
	PlayerAnimationController(GameObject& IN_gameObject, iAllocator& IN_allocator) :
		gameObject	{ &IN_gameObject	},
		allocator	{ &IN_allocator		}
	{
		auto animator = gameObject->GetView<AnimatorView>();

		movementVectorIdx = animator->AddInput("MovementVector", AnimatorInputType::Float3);

		auto test = animator->GetInputValue(movementVectorIdx).value()->xyz;
		int x = 0;
	}

	void Update(AnimationStateContext& ctx, double dT, iAllocator& temp) override
	{

	}

	void Release() override
	{
		allocator->release(this);
	}

	uint32_t	movementVectorIdx	= 0;
	GameObject* gameObject			= nullptr;
	iAllocator* allocator			= nullptr;
};




/************************************************************************************************/


AnimationTest::AnimationTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState{ IN_framework },

	animators				{ framework.core.GetBlockMemory() },
	brushes					{ framework.core.GetBlockMemory() },
	cameras					{ framework.core.GetBlockMemory() },
	materials				{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },
	visibilityComponent		{ framework.core.GetBlockMemory() },
	pointLights				{ framework.core.GetBlockMemory() },
	orbitCameras			{ framework.core.GetBlockMemory() },
	shadowMaps				{ framework.core.GetBlockMemory() },
	ikComponent				{ framework.core.GetBlockMemory() },
	skeletons				{ framework.core.GetBlockMemory() },
	stringIDs				{ framework.core.GetBlockMemory() },
	triggers				{ framework.core.GetBlockMemory(), framework.core.GetBlockMemory() },

	physx					{ framework.core.Threads, framework.core.GetBlockMemory() },
	rigidBodies				{ physx },
	staticBodies			{ physx },
	characterController		{ physx, framework.core.GetBlockMemory() },

    renderer				{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },

	gbuffer			{ { 1920, 1080 }, framework.GetRenderSystem() },
	depthBuffer		{ framework.GetRenderSystem(), { 1920, 1080 } },
	renderWindow	{ },

	constantBuffer	{ framework.GetRenderSystem().CreateConstantBuffer(64 * MEGABYTE, false) },
	vertexBuffer	{ framework.GetRenderSystem().CreateVertexBuffer(64 * MEGABYTE, false) },
	runOnceQueue	{ framework.core.GetBlockMemory() },

	inputMap		{ framework.core.GetBlockMemory() },

	portalComponent	{ framework.core.GetBlockMemory(), framework.core, playerObject },
	spawnComponent	{ framework.core.GetBlockMemory() },
	playerComponent	{ framework.core.GetBlockMemory(), framework.core.GetBlockMemory() },

	debugRays		{ framework.core.GetTempMemoryMT() }
{
	auto& rs = IN_framework.GetRenderSystem();

#if USEVULKAN 
	if (auto res = CreateWin32VKSurface(rs, {1920, 1080}, DeviceFormat::R8G8B8A8_UNORM); res != nullptr)
	{
		renderWindow = res;
	}
	else
		throw std::runtime_error{ "Failed to create vulkan surface" };
#else
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = res;
	else
		throw std::runtime_error{ "Failed to create render window" };
#endif

	EventNotifier<>::Subscriber sub;
	sub.Notify		= &FlexKit::EventsWrapper;
	sub._ptr		= &framework;


	//Subscribe(renderWindow, sub);
	//SetWindowTitle("Physics Test", renderWindow);

	auto& allocator = framework.core.GetBlockMemory();
	rs.RegisterPSOLoader(DRAW_LINE_PSO,		CreateDrawLineStatePSO);
	rs.RegisterPSOLoader(DRAW_LINE3D_PSO,	CreateDraw2StatePSO);

	RegisterPhysicsDebugVis(framework.GetRenderSystem());
	AddAssetFile(R"(assets\TestWorld.gameres)");
	AddAssetFile(R"(assets\MainCharacterPrefab.gameres)");

	InitiateScriptRuntime();

	RegisterMathTypes(GetScriptEngine(), framework.core.GetBlockMemory());
	RegisterRuntimeAPI(GetScriptEngine());

	if (!LoadLevel(21654, framework.core))
		throw std::runtime_error("Failed to load Level!");

	auto level = GetActiveLevel();

	// Setup Camera
	auto& player			= playerObject.AddView<PlayerView>().GetData();
	auto& tpc				= CreateThirdPersonCameraController(playerObject, level->layer, framework.core.GetBlockMemory(), 1.0f, 4.0f);
	auto& playerTriggers	= *playerObject.GetView<TriggerView>();

	tpc->drag		= 8.0f;
	tpc->gravity	= float3{ 0, -20.0f, 0 };
	tpc->moveRate	= player.moveRate;
	tpc->SetPosition({ 7.0f, 10.0f, -55.0f });

	playerTriggers->CreateSlot(GetCRCGUID(ResetMovement),
		[&](auto ...)
		{
			tpc->moveRate	= player.moveRate;
			tpc->gravity	= float3{ 0, -20.0f, 0 };
		});

	playerTriggers->CreateTrigger(OnFloorContact);
	playerTriggers->Connect(OnFloorContact, GetCRCGUID(ResetMovement));

	auto cameraHandle = GetCameraControllerCamera(playerObject);
	SetCameraAspectRatio(cameraHandle, renderWindow->GetAspectRatio());
	SetCameraFOV(cameraHandle, (float)FlexKit::pi / 4.0f);

	auto loadPrefabRes = FlexKit::LoadPrefab(character, 21536, framework.core.GetBlockMemory());
	level->scene.AddGameObject(character);

	if (!loadPrefabRes)
		FK_LOG_WARNING("Failed to load player prefab!");
	else if(auto animator = character.GetView<AnimatorView>(); animator)
	{
		auto& controller = allocator.allocate<PlayerAnimationController>(character, allocator);
		animator->SetController(controller);
	}

	auto& material = character.AddView<MaterialView>();
	auto material1 = material.CreateSubMaterial();
	auto material2 = material.CreateSubMaterial();
	auto material3 = material.CreateSubMaterial();
	SetMaterialHandle(character, material.handle);

	auto& materials = MaterialComponent::GetComponent();
	material.Add2Pass(GBufferPassID);
	materials.Add2Pass(material1, GBufferPassID);
	materials.Add2Pass(material2, GBufferPassID);
	materials.Add2Pass(material3, GBufferPassID);

	activeCamera = cameraHandle;

	SetParentNode(character, tpc->objectNode);

	auto res = level->scene.Query(framework.core.GetTempMemory(), GameObjectReq{}, SceneNodeReq{}, ROStringQuery{ "guramesh" });

	if (res.size())
	{
		auto& [gameObject, sceneNode, ID] = res.front();

		sceneNode.SetPositionL({ 0, 10, 0 });
		sceneNode.SetParentNode(tpc->objectNode);
	}

	inputMap.MapKeyToEvent(KC_W, TPC_MoveForward);
	inputMap.MapKeyToEvent(KC_A, TPC_MoveLeft);
	inputMap.MapKeyToEvent(KC_S, TPC_MoveBackward);
	inputMap.MapKeyToEvent(KC_D, TPC_MoveRight);
}


/************************************************************************************************/


AnimationTest::~AnimationTest()
{
	playerObject.Release();
	ReleaseAllLevels();
	ReleaseScriptRuntime();
}


/************************************************************************************************/


FlexKit::UpdateTask* AnimationTest::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
#if USEVULKAN
	vkWin32UpdateInput();
#else
	Win32UpdateInput();
	auto mouseState = UpdateCapturedMouseInput(dT, renderWindow);
#endif
	auto& playerUpdate				= QueuePlayerUpdate(playerObject, dispatcher, dT);
	auto& thirdPersonCameraUpdate = QueueThirdPersonCameraControllers(dispatcher, {} /* mouseState.Normalized_dPos */, dT);

	thirdPersonCameraUpdate.AddInput(playerUpdate);

	cameras.MarkDirty(activeCamera);

	if(framework.UpdateDebugUI(*renderWindow, core, dispatcher, dT))
	{
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({ (float)renderWindow->GetWH()[0] - 600.0f, 0});
		ImGui::SetNextWindowSize({ 600, 400 });

		ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

		const auto memoryStats = framework.core.GetBlockMemory().GetStats();

		auto memoryInUse = (memoryStats.smallBlocksAllocated * 64 +
			memoryStats.mediumBlocksAllocated * 2048 +
			memoryStats.largeBlocksAllocated * KILOBYTE * 128) / MEGABYTE;

		auto [position, velocity, gravity, hangState, jumpSpeed, airMovementRatio, moveRate, climbDown] =
			Apply(playerObject,
				[&](CameraControllerView& cameraController,
					PlayerView&				playerView){
						return std::make_tuple(
										cameraController->GetPosition(), cameraController->velocity, cameraController->gravity,
										playerView->hangPossible, playerView->jumpSpeed, playerView->airMovementRatio, playerView->moveRate, playerView->climbDownPossible); },
						[]{ return std::make_tuple(float3::Zero(), float3::Zero(), float3::Zero(), false, 0.0f, 0.0f, 0.0f, false);  });

		auto str = fmt::format(
			"Debug Stats\n"
			"SmallBlocks: {} / {}\n"
			"MediumBlocks: {} / {}\n"
			"LargeBlocks: {} / {}\n"
			"Memory in use: {}mb\n"
			"Player Position: {}, {}, {}\n"
			"Player Velocity: {}, {}, {}\n"
			"Player Gravity: {}, {}, {}\n"
			"{}"
			"{}"
			"Press M to toggle mouse look",
			memoryStats.smallBlocksAllocated, memoryStats.totalSmallBlocks,
			memoryStats.mediumBlocksAllocated, memoryStats.totalMediumBlocks,
			memoryStats.largeBlocksAllocated, memoryStats.totalLargeBlocks,
			memoryInUse,
			position.x, position.y, position.z,
			velocity.x, velocity.y, velocity.z,
			gravity.x, gravity.y, gravity.z,
			hangState ? "Hang Found!\n" : "No Hang\n",
			climbDown ? "Climb Down Ledge Found!\n" : "");

		ImGui::Text(str.c_str());
		if (ImGui::SliderFloat("Jump height", &jumpSpeed, 10.0f, 50.0f))
			Apply(playerObject, [&](PlayerView& playerView) { playerView->jumpSpeed = jumpSpeed; });

		if (ImGui::SliderFloat("In Air speed ratio", &airMovementRatio, 0.0f, 1.0f) ||
			ImGui::SliderFloat("MoveRate", &moveRate, 10, 100))
		{
			Apply(playerObject,
				[&](CameraControllerView& cameraController)
				{
					cameraController->moveRate = moveRate;
					cameraController->airMovementRatio = airMovementRatio;
				});
		}

		ImGui::End();
		ImGui::EndFrame();
		ImGui::Render();
	}

	return &thirdPersonCameraUpdate;
}


/************************************************************************************************/


FlexKit::UpdateTask* AnimationTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	frameGraph.AddOutput(renderWindow->GetBackBuffer());
	frameGraph.AddConstantBuffer(constantBuffer);
	frameGraph.AddVertexBuffer(vertexBuffer);

	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	FlexKit::WorldRender_Targets targets{
		.RenderTarget	= renderWindow->GetBackBuffer(),
		.DepthTarget	= depthBuffer,
	};

	auto& physicsUpdate = physx.Update(dispatcher, dT);
	physicsUpdate.AddInput(*update);
	
	static double T = 0.0;
	T += dT;

	auto& transformUpdate	= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);

	transformUpdate.AddInput(physicsUpdate);
	cameraUpdate.AddInput(transformUpdate);

	auto currentLevel = GetActiveLevel();

	FlexKit::DrawSceneDescription drawSceneDesc{
		.camera = activeCamera,
		.scene	= currentLevel->scene,
		.dt		= dT,
		.t		= T,

		.gbuffer = gbuffer,

		.transformDependency	= transformUpdate,
		.cameraDependency		= cameraUpdate
	};

	//auto res = renderer.DrawScene(
	//	dispatcher,
	//	frameGraph,
	//	drawSceneDesc,
	//	targets,
	//	core.GetBlockMemory(),
	//	core.GetTempMemoryMT()
	//);
	//
	//textureStreamingEngine.TextureFeedbackPass(dispatcher, frameGraph, activeCamera, core.RenderSystem->GetTextureWH(targets.RenderTarget), res.entityConstants, res.passes, res.animationResources, dT, core.GetTempMemoryMT());
	RenderPhysicsOverlay(frameGraph, targets.RenderTarget, depthBuffer.Get(), currentLevel->layer, activeCamera);

	framework.DrawDebugUI(dT, dispatcher, frameGraph, renderWindow->GetBackBuffer());

	LineSegments segments{ core.GetTempMemory() };
	auto constants = GetCameraConstants(activeCamera);

	const auto P	= constants.Proj;
	const auto V	= constants.View;
	const auto PV	= constants.PV;


	//if (A_DC.w > 0 && B_DC.w > 0)
	for(auto& line : debugRays)
	{
		const auto A_DC = PV * float4{ line.A, 1 };
		const auto B_DC = PV * float4{ line.B, 1 };

		const auto A_NDC = A_DC.xyz() / A_DC.w;
		const auto B_NDC = B_DC.xyz() / B_DC.w;
		segments.emplace_back(A_NDC, float3{ 1, 0, 1 }, B_NDC, FlexKit::float3{ 1, 0, 1 });
	}

	debugRays.clear();

	/*
	else if ((A_DC.w > 0 && B_DC.w < 0) || (A_DC.w < 0 && B_DC.w > 0) )
	{
		const auto V1_vs = V * float4{ A, 1 };
		const auto V2_vs = V * float4{ B, 1};

		const auto z	= float3(0, 0, -1).dot(V1_vs.xyz() + float3(0, 0, 0.01f));
		auto dir		= (V1_vs - V2_vs).xyz().normal();
		auto asdf		= V1_vs + dir * z;

		const auto A_DC		= P * float4(asdf.xyz(), 1);
		const auto A_NDC	= A_DC.xyz() / A_DC.w;
		const auto B_NDC	= B_DC.xyz() / B_DC.w;
		segments.emplace_back(A_NDC, float3{ 1, 0, 1 }, B_NDC, FlexKit::float3{ 1, 0, 1 });
	}
	*/

	DrawShapes(DRAW_LINE_PSO, frameGraph, targets.RenderTarget, core.GetTempMemoryMT(),
		LineShape{ segments });

	FlexKit::PresentBackBuffer(frameGraph, *renderWindow);

	return nullptr;
}


/************************************************************************************************/


void AnimationTest::PostDrawUpdate(FlexKit::EngineCore& core, double dT)
{
	renderWindow->Present(1, 0);

	core.RenderSystem->ResetConstantBuffer(constantBuffer);
}


/************************************************************************************************/


bool AnimationTest::EventHandler(FlexKit::Event evt)
{
	switch (evt.InputSource)
	{
		case Event::Keyboard:
		{	
			if (auto evt_mapped = evt; inputMap.Map(evt, evt_mapped))
				return PlayerHandleEvents(playerObject, evt_mapped);

			switch (evt.Action)
			{
			case Event::Pressed:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_SPACE:
					Trigger(playerObject, OnJumpTriggerID);
					return true;
				case KC_LEFTCTRL:
					Trigger(playerObject, OnCrouchTriggerID);
					return true;
				case KC_T:
					framework.core.RenderSystem->QueuePSOLoad(SHADINGPASS);
					framework.core.RenderSystem->QueuePSOLoad(SHADOWMAPPASS);
					framework.core.RenderSystem->QueuePSOLoad(SHADOWMAPANIMATEDPASS);
					return true;
				}
			}	break;
			case Event::Release:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_M:
					FK_ASSERT(0);
					//ToggleMouseCapture(renderWindow);
					return true;
				case KC_E:
					Trigger(playerObject, ActivateTrigger);
					return true;
				case KC_SPACE:
					Trigger(playerObject, OnJumpReleaseTriggerID);
					return true;
				case KC_ESC:
					framework.quit = true;
					return true;
				case KC_LEFTCTRL:
					Trigger(playerObject, OnCrouchReleaseTriggerID);
					break;
				}
			}	break;
		default:
			if ((evt.InputSource == FlexKit::Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC) ||
				(evt.InputSource == FlexKit::Event::E_SystemEvent && evt.Action == FlexKit::Event::Exit))
			{
				framework.quit = true;
				return true;
			}
			else
				return framework.HandleDebugInput(evt);
		}	break;
		case Event::Mouse:
		{
			return false;
		}	break;
		}
	}

	FK_ASSERT(0);

    //if(!IsMouseCaptured(renderWindow))
	//	return framework.HandleDebugInput(evt);
	//else
		return false;
}


/**********************************************************************

Copyright (c) 2019-2023 Robert May

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
