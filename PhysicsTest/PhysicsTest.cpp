#include "pch.h"
#include "level.hpp"
#include "PhysicsTest.h"
#include <KeyValueIds.h>
#include <SceneLoadingContext.h>
#include <imgui.h>
#include <PhysicsDebugVis.h>
#include <TriggerSlotIDs.hpp>
#include <ScriptingRuntime.h>


using namespace FlexKit;


/************************************************************************************************/


PortalFactory::PortalFactory(PhysicsTest* IN_test) : test{ IN_test } {}


void PortalFactory::OnCreateView(
	FlexKit::GameObject&	gameObject,
	FlexKit::ValueMap		userValues,
	const std::byte*		buffer,
	const size_t			bufferSize,
	FlexKit::iAllocator*	allocator)
{
	PortalComponentBlob portal;
	memcpy(&portal, buffer, sizeof(portal));

	auto& triggers			= gameObject.AddView<TriggerView>();
	auto& portalView		= gameObject.AddView<PortalView>();
	portalView->levelID			= portal.sceneID;
	portalView->spawnPointID	= portal.spawnObjectID;

	triggers->CreateTrigger(ActivateTrigger);
	triggers->CreateSlot(PortalSlot,
		[&](void*, uint64_t)
		{
			auto& portalView	= *gameObject.GetView<PortalView>();
			auto levelID		= portalView->levelID;
			auto spawnPointID	= portalView->spawnPointID;

			if (levelID == INVALIDHANDLE)
				return;

			if (!LoadLevel(levelID, test->framework.core))
				throw std::runtime_error("Failed to load Level!");

			auto currentLevel	= GetActiveLevel();
			auto level			= GetLevel(levelID);

			if (!level)
				return;

			level->scene.QueryFor(
				[&](auto& gameObject, auto&& res)
				{
					const float3 newPosition = GetWorldPosition(gameObject);

					SetControllerPosition(test->cameraRig, newPosition);

					if (levelID != GetActiveLevelID())
					{
						// Remove Character Controller
						// Readd new Character controller in new layer
						SetActiveLevel(levelID);

						test->physx.GetLayer_ref(currentLevel->layer).paused = true;
						test->physx.GetLayer_ref(level->layer).paused = false;

						currentLevel->scene.RemoveEntity(test->cameraRig);
						level->scene.AddGameObject(test->cameraRig);

						Apply(gameObject,
							[&](CharacterControllerView& ccv)
							{
								ccv.ChangeLayer(level->layer);
							});
					}
				},
				FlexKit::StringHashQuery{ spawnPointID });


			int x = 0;
		});

	triggers->Connect(ActivateTrigger, PortalSlot);
}


void SpawnFactory::OnCreateView(
	FlexKit::GameObject&	gameObject,
	FlexKit::ValueMap		user_ptr,
	const std::byte*		buffer,
	const size_t			bufferSize,
	FlexKit::iAllocator*	allocator)
{
	SpawnComponentBlob spawn;
}


/************************************************************************************************/


PhysicsTest::PhysicsTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState{ IN_framework },

	animators				{ framework.core.GetBlockMemory() },
	brushes					{ framework.core.GetBlockMemory(), framework.GetRenderSystem() },
	cameras					{ framework.core.GetBlockMemory() },
	sceneNodes				{ },
	materials				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	visibilityComponent		{ framework.core.GetBlockMemory() },
	pointLights				{ framework.core.GetBlockMemory() },
	orbitCameras			{ framework.core.GetBlockMemory() },
	pointLightShadowMaps	{ framework.core.GetBlockMemory() },
	ikComponent				{ framework.core.GetBlockMemory() },
	skeletons				{ framework.core.GetBlockMemory() },
	stringIDs				{ framework.core.GetBlockMemory() },
	triggers				{ framework.core.GetBlockMemory(), framework.core.GetBlockMemory() },

	physx					{ framework.core.Threads, framework.core.GetBlockMemory() },
	rigidBodies				{ physx },
	staticBodies			{ physx },
	characterController		{ physx, framework.core.GetBlockMemory() },

	renderer				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	textureStreamingEngine	{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },

	gbuffer			{ { 1920, 1080 }, framework.GetRenderSystem() },
	depthBuffer		{ framework.GetRenderSystem(), { 1920, 1080 } },
	renderWindow	{ },

	constantBuffer	{ framework.GetRenderSystem().CreateConstantBuffer(64 * MEGABYTE, false) },
	vertexBuffer	{ framework.GetRenderSystem().CreateVertexBuffer(64 * MEGABYTE, false) },
	runOnceQueue	{ framework.core.GetBlockMemory() },
	debugUI			{ framework.core.RenderSystem, framework.core.GetBlockMemory() },

	inputMap		{ framework.core.GetBlockMemory() },

	portalComponent	{ framework.core.GetBlockMemory(), this },
	spawnComponent	{ framework.core.GetBlockMemory() }
{
	auto& rs = IN_framework.GetRenderSystem();
	rs.RegisterPSOLoader(DRAW_LINE_PSO, { &rs.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO });
	rs.RegisterPSOLoader(DRAW_LINE3D_PSO, { &rs.Library.RS6CBVs4SRVs, CreateDraw2StatePSO });

	RegisterPhysicsDebugVis(framework.GetRenderSystem());
	AddAssetFile(R"(assets\spawnRoom.gameres)");
	AddAssetFile(R"(assets\MainCharacterPrefab.gameres)");

	InitiateScriptRuntime();

	RegisterMathTypes(GetScriptEngine(), framework.core.GetBlockMemory());
	RegisterRuntimeAPI(GetScriptEngine());

	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());

	EventNotifier<>::Subscriber sub;
	sub.Notify		= &FlexKit::EventsWrapper;
	sub._ptr		= &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Physics Test");

	if (!LoadLevel(32688, framework.core))
		throw std::runtime_error("Failed to load Level!");

	auto level = GetActiveLevel();

	// Setup Camera
	auto& tpc = CreateThirdPersonCameraController(cameraRig, level->layer, framework.core.GetBlockMemory(), 1.0f, 4.0f);

	tpc->SetPosition({ 0, 100, 0 });

	auto cameraHandle = GetCameraControllerCamera(cameraRig);
	SetCameraAspectRatio(cameraHandle, renderWindow.GetAspectRatio());
	SetCameraFOV(cameraHandle, (float)FlexKit::pi / 4.0f);

	auto loadPrefabRes = FlexKit::LoadPrefab(character, 21536, framework.core.GetBlockMemory());
	level->scene.AddGameObject(character);

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
		auto& [gameObject, sceneNode, ID] = res.front().value();

		sceneNode.SetPositionL({ 0, 10, 0 });
		sceneNode.SetParentNode(tpc->objectNode);
	}

	inputMap.MapKeyToEvent(KC_W, TPC_MoveForward);
	inputMap.MapKeyToEvent(KC_A, TPC_MoveLeft);
	inputMap.MapKeyToEvent(KC_S, TPC_MoveBackward);
	inputMap.MapKeyToEvent(KC_D, TPC_MoveRight);
}


/************************************************************************************************/


PhysicsTest::~PhysicsTest()
{
	cameraRig.Release();
	ReleaseAllLevels();
	ReleaseScriptRuntime();
}


/************************************************************************************************/


FlexKit::UpdateTask* PhysicsTest::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	UpdateInput();
	renderWindow.UpdateCapturedMouseInput(dT);

	UpdateThirdPersonCameraControllers(renderWindow.mouseState.Normalized_dPos, dT);

	cameras.MarkDirty(activeCamera);

	debugUI.Update(renderWindow, core, dispatcher, dT);

	ImGui::NewFrame();
	ImGui::SetNextWindowPos({ (float)renderWindow.WH[0] - 400.0f, 0});
	ImGui::SetNextWindowSize({ 400, 400 });

	ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

	const auto memoryStats = framework.core.GetBlockMemory().GetStats();

	auto memoryInUse = (memoryStats.smallBlocksAllocated * 64 +
		memoryStats.mediumBlocksAllocated * 2048 +
		memoryStats.largeBlocksAllocated * KILOBYTE * 128) / MEGABYTE;

	auto str = fmt::format(
		"Debug Stats\n"
		"SmallBlocks: {} / {}\n"
		"MediumBlocks: {} / {}\n"
		"LargeBlocks: {} / {}\n"
		"Memory in use: {}mb\n"
		"M to toggle mouse\n"
		"T to toggle texture streaming\n"
		"R to toggle rotating camera\n",
		memoryStats.smallBlocksAllocated, memoryStats.totalSmallBlocks,
		memoryStats.mediumBlocksAllocated, memoryStats.totalMediumBlocks,
		memoryStats.largeBlocksAllocated, memoryStats.totalLargeBlocks,
		memoryInUse
	);

	ImGui::Text(str.c_str());

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();

	return nullptr;
}


/************************************************************************************************/


FlexKit::UpdateTask* PhysicsTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	frameGraph.AddOutput(renderWindow.GetBackBuffer());
	core.RenderSystem.ResetConstantBuffer(constantBuffer);

	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	FlexKit::WorldRender_Targets targets{
		.RenderTarget = renderWindow.GetBackBuffer(),
		.DepthTarget = depthBuffer,
	};
	ReserveConstantBufferFunction	reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());
	ReserveVertexBufferFunction		reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());

	auto& physicsUpdate = physx.Update(dispatcher, dT);
	
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

		.reserveVB = reserveVB, 
		.reserveCB = reserveCB, 

		.transformDependency	= transformUpdate,
		.cameraDependency		= cameraUpdate
	};

	auto res = renderer.DrawScene(
		dispatcher,
		frameGraph,
		drawSceneDesc,
		targets,
		core.GetBlockMemory(),
		core.GetTempMemoryMT()
	);

	textureStreamingEngine.TextureFeedbackPass(dispatcher, frameGraph, activeCamera, core.RenderSystem.GetTextureWH(targets.RenderTarget), res.entityConstants, res.passes, res.skinnedDraws, reserveCB, reserveVB);
	RenderPhysicsOverlay(frameGraph, targets.RenderTarget, depthBuffer.Get(), currentLevel->layer, activeCamera, reserveVB, reserveCB);

	debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

	LineSegments segments{ core.GetTempMemory() };
	auto constants = GetCameraConstants(activeCamera);

	auto PV = constants.PV;

	const auto A_DC = PV * float4{ A, 1 };
	const auto B_DC = PV * float4{ B, 1 };

	if (!(A_DC.w <= 0 || B_DC.w <= 0))
	{
		const auto A_NDC = A_DC.xyz() / A_DC.w;
		const auto B_NDC = B_DC.xyz() / B_DC.w;
		segments.emplace_back(A_NDC, float3{ 1, 0, 1 }, B_NDC, FlexKit::float3{ 1, 0, 1 });
	}

	DrawShapes(DRAW_LINE_PSO, frameGraph, reserveVB, reserveCB, targets.RenderTarget, core.GetTempMemoryMT(),
		LineShape{ segments });

	FlexKit::PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void PhysicsTest::PostDrawUpdate(FlexKit::EngineCore&, double dT)
{
	renderWindow.Present(1, 0);
}


/************************************************************************************************/


void PhysicsTest::Action()
{
	const auto r	= FlexKit::ViewRay(activeCamera, { 0.0f, 0.5f });

	auto layer		= GetActiveLevel()->layer;

	auto& layer_ref	= physx.GetLayer_ref(layer);

	std::cout << layer.INDEX << "\n";

	layer_ref.RayCast({ r.D, r.O }, 100,
		[&](PhysicsLayer::RayCastHit hit)
		{
			if(auto stringID = GetStringID(*hit.gameObject); stringID)
				std::cout << stringID << "\n";

			A = r.O;
			B = r.R(hit.distance);

			Trigger(*hit.gameObject, ActivateTrigger, nullptr);
			return false;
		});
}


/************************************************************************************************/


void PhysicsTest::Jump()
{
}


/************************************************************************************************/


void PhysicsTest::Fall()
{
}


/************************************************************************************************/


bool PhysicsTest::EventHandler(FlexKit::Event evt)
{
	switch (evt.InputSource)
	{
		case Event::Keyboard:
		{
			auto evt_mapped = evt;
			if (inputMap.Map(evt, evt_mapped))
				return HandleTPCEvents(cameraRig, evt_mapped);

			switch (evt.Action)
			{
			case Event::Pressed:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_SPACE:
					Jump();
					return true;
				}
			}	break;
			case Event::Release:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_M:
					renderWindow.ToggleMouseCapture();
					return true;
				case KC_E:
					Action();
					return true;
				case KC_SPACE:
					Fall();
					return true;
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
				return debugUI.HandleInput(evt);
		}	break;
		case Event::Mouse:
		{
		}	break;
		}
	}

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
