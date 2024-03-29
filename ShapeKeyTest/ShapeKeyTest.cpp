#include "pch.h"

#include <Application.h>
#include <Scene.h>
#include <physicsutilities.h>
#include <TextureStreamingUtilities.h>
#include <TriggerComponent.h>
#include <WorldRender.h>
#include <Win32Graphics.h>
#include <DebugUI.h>
#include <imgui.h>
#include <..\source\Signals.h>
#include <CameraUtilities.h>
#include <SceneLoadingContext.h>
#include <ranges>

using namespace FlexKit;


/************************************************************************************************/


class ShapeKeyTest : public FlexKit::FrameworkState
{
public:
	ShapeKeyTest(FlexKit::GameFramework& IN_framework) :
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
		debugUI			{ framework.core.RenderSystem, framework.core.GetBlockMemory() }
	{
		using namespace std::views;

		auto& rs = IN_framework.GetRenderSystem();
		rs.RegisterPSOLoader(DRAW_LINE_PSO, { &rs.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO });
		rs.RegisterPSOLoader(DRAW_LINE3D_PSO, { &rs.Library.RS6CBVs4SRVs, CreateDraw2StatePSO });

		InitiateScriptRuntime();

		RegisterMathTypes(GetScriptEngine(), framework.core.GetBlockMemory());
		RegisterRuntimeAPI(GetScriptEngine());

		//AddAssetFile(R"(assets\aerith.gameres)");
		AddAssetFile(R"(assets\ShadowTestScene.gameres)");

		if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
			renderWindow = std::move(res.value());

		EventNotifier<>::Subscriber sub;
		sub.Notify = &FlexKit::EventsWrapper;
		sub._ptr = &framework;

		renderWindow.Handler->Subscribe(sub);
		renderWindow.SetWindowTitle("Shape Key Test");

		auto& orbitCameraView = orbitCamera.AddView<OrbitCameraBehavior>();
		activeCamera = orbitCameraView;
		SetCameraAspectRatio(activeCamera, 1920.0f / 1080.0f);
		//SetCameraFOV(activeCamera, pi / 8);
		//SetCameraFar(activeCamera, 100.0f);
		SceneLoadingContext ctx
		{
			.scene	= scene,
			.nodes			{ framework.core.GetBlockMemory() },
			.pendingActions	{ framework.core.GetBlockMemory() }
		};

		LoadScene(framework.core, ctx, "Scene");

		/*
		// Load Model
		auto meshHandle = FlexKit::GetMesh("Player.mesh");
		if (meshHandle == InvalidHandle)
			throw std::runtime_error("Failed to setup scene");

		auto& brushView		= object.AddView<BrushView>(meshHandle);
		auto& materialView	= object.AddView<MaterialView>();
		auto& skeleton		= object.AddView<SkeletonView>(3568);
		auto& animator		= object.AddView<AnimatorView>();

		brushView.SetMaterial(materialView);
		materialView.Add2Pass(GBufferAnimatedPassID);
		materialView.Add2Pass(PassHandle{ GetCRCGUID(MorphTargetID) });
		//materialView.Add2Pass(GBufferPassID);

		scene.AddGameObject(object);
		SetBoundingSphereFromMesh(object);
		*/


		// Create 3 point lighting
		if(false)
		{
			{
				auto& pointLightNode = light1.AddView<SceneNodeView>();
				auto& pointLightView = light1.AddView<LightView>(float3{ 1, 1, 1 }, 1000, 20, pointLightNode.node, true);
				pointLightNode.SetPosition({ 10, 10, -5 });
				scene.AddGameObject(light1, pointLightNode);
			}
			{
				auto& pointLightNode = light2.AddView<SceneNodeView>();
				auto& pointLightView = light2.AddView<LightView>(float3{ 1, 1, 1 }, 1000, 20, pointLightNode.node, true);
				pointLightNode.SetPosition({ -10, 10, -5 });
				scene.AddGameObject(light2, pointLightNode);
			}
			{
				auto& pointLightNode = light3.AddView<SceneNodeView>();
				auto& pointLightView = light3.AddView<LightView>(float3{ 1, 1, 1 }, 1000, 20, pointLightNode.node, true);
				pointLightNode.SetPosition({ 0, 10, 10 });
				scene.AddGameObject(light3, pointLightNode);
			}
		}

		auto res = scene.Query(framework.core.GetBlockMemory(), LightQuery{}, GameObjectReq{});

		for (auto&& [idx, queryRes] : zip(iota(0),  res))
		{
			auto& [lightView, gameObject] = queryRes.value();
			lightView.SetRadius(50.0f);
			lightView.SetIntensity(50000.0f);
			lightView.SetOuterAngle((float)pi / 4.0f);
			lightView.SetOuterAngle((float)pi / 1.5f);
			lightView.SetType(LightType::SpotLight);
			//SetOrientation(lightView.GetNode(), Quaternion{ 0, 45.0f, 0.0f });
		}

		{
			//auto& [lightView, gameObject] = res[0].value();
			//lightView.SetType(LightType::PointLight);
			//lightView.SetIntensity(5000.0f);
		}

		if (res.size())
		{
			auto& [pointLightView, gameObject] = res.front().value();
			auto pos = GetPositionW(pointLightView.GetNode());
			orbitCameraView.SetCameraPosition(pos);
		}
	}


	~ShapeKeyTest() final
	{
		scene.ClearScene();
	}


	FlexKit::UpdateTask* Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT) final
	{
		UpdateInput();
		renderWindow.UpdateCapturedMouseInput(dT);

		for (auto& queryRes : scene.Query(framework.core.GetBlockMemory(), LightQuery{}))
		{
			const auto& [lightView] = queryRes.value();

			//Pitch(lightView.GetNode(), dT * pi / 4);
			//Yaw(lightView.GetNode(), dT * pi / 4);

			lightView.SetSize(std::cos(t) / 4.0f + 0.25);
		}



		t += dT;

		debugUI.Update(renderWindow, core, dispatcher, dT);

		ImGui::NewFrame();
		ImGui::SetNextWindowPos({ (float)renderWindow.WH[0] - 600.0f, 0 });
		ImGui::SetNextWindowSize({ 600, 400 });

		//ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
		//ImGui::Text("Hello world!");

		//ImGui::End();
		ImGui::EndFrame();
		ImGui::Render();

		return nullptr;
	}


	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) final
	{
		frameGraph.AddOutput(renderWindow.GetBackBuffer());

		ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

		FlexKit::WorldRender_Targets targets{
			.RenderTarget	= renderWindow.GetBackBuffer(),
			.DepthTarget	= depthBuffer,
		};

		ReserveConstantBufferFunction	reserveCB = CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());
		ReserveVertexBufferFunction		reserveVB = CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());

		static double T = 0.0;
		T += dT;

		auto& transformUpdate	= FlexKit::QueueTransformUpdateTask(dispatcher);
		auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);
		auto& orbitCameraUpdate = QueueOrbitCameraUpdateTask(dispatcher, *orbitCamera.GetView<OrbitCameraBehavior>(), renderWindow.mouseState, dT);

		transformUpdate.AddInput(orbitCameraUpdate);
		cameraUpdate.AddInput(transformUpdate);

		FlexKit::DrawSceneDescription drawSceneDesc{
			.camera = activeCamera,
			.scene	= scene,
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
		
		textureStreamingEngine.TextureFeedbackPass(dispatcher, frameGraph, activeCamera, core.RenderSystem.GetTextureWH(targets.RenderTarget), res.entityConstants, res.passes, res.animationResources, reserveCB, reserveVB, core.GetTempMemoryMT());

		debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

		FlexKit::PresentBackBuffer(frameGraph, renderWindow);

		return nullptr;
	}


	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) final
	{
		renderWindow.Present(1, 0);

		core.RenderSystem.ResetConstantBuffer(constantBuffer);
	}


	bool EventHandler(FlexKit::Event evt) final
	{
		if(!OrbitCameraHandleEvent(orbitCamera, evt))
		{
			switch (evt.InputSource)
			{
				case Event::Keyboard:
				{
					switch (evt.Action)
					{
						case Event::Pressed:
						{
							switch (evt.mData1.mKC[0])
							{
							case KC_R:
								framework.core.RenderSystem.QueuePSOLoad(SHADINGPASS);
								framework.core.RenderSystem.QueuePSOLoad(BUILDROWSUMS);
								framework.core.RenderSystem.QueuePSOLoad(BUILDCOLUMNSUMS);
								framework.core.RenderSystem.QueuePSOLoad(SHADOWMAPPASS);
								framework.core.RenderSystem.QueuePSOLoad(SHADOWMAPANIMATEDPASS);
								return true;
							case KC_M:
								renderWindow.ToggleMouseCapture();
								return true;
							case KC_ESC:
								framework.quit = true;
								return true;
							}
						}	break;
						case Event::Release:
						{
						}	break;
					}
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

			if(!renderWindow.mouseCapture)
				return debugUI.HandleInput(evt);
			else
				return false;
		}
		else return true;
	}


	FlexKit::AnimatorComponent				animators;
	FlexKit::CameraComponent				cameras;
	FlexKit::CameraControllerComponent		orbitCameras;
	FlexKit::SceneNodeComponent				sceneNodes;
	FlexKit::MaterialComponent				materials;
	FlexKit::SceneVisibilityComponent		visibilityComponent;
	FlexKit::BrushComponent					brushes;
	FlexKit::LightComponent					pointLights;
	FlexKit::ShadowMapComponent						pointLightShadowMaps;
	FlexKit::FABRIKComponent				ikComponent;
	FlexKit::SkeletonComponent				skeletons;
	FlexKit::StringIDComponent				stringIDs;
	FlexKit::TriggerComponent				triggers;

	FlexKit::PhysXComponent					physx;
	FlexKit::RigidBodyComponent				rigidBodies;
	FlexKit::StaticBodyComponent			staticBodies;
	FlexKit::CharacterControllerComponent	characterController;

	FlexKit::GBuffer						gbuffer;
	FlexKit::DepthBuffer					depthBuffer;
	FlexKit::Win32RenderWindow				renderWindow;
	FlexKit::ConstantBufferHandle			constantBuffer;
	FlexKit::VertexBufferHandle				vertexBuffer;

	FlexKit::WorldRender					renderer;
	FlexKit::TextureStreamingEngine			textureStreamingEngine;

	FlexKit::Scene							scene;

	FlexKit::CameraHandle					activeCamera = FlexKit::InvalidHandle;
	FlexKit::GameObject						orbitCamera;

	FlexKit::GameObject						object;
	FlexKit::GameObject						light1;
	FlexKit::GameObject						light2;
	FlexKit::GameObject						light3;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	FlexKit::ImGUIIntegrator		debugUI;

	double t = 0.0f;
};


/************************************************************************************************/


int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1);

		app->PushState<ShapeKeyTest>();
		app->GetCore().FPSLimit = 144;
		app->GetCore().FrameLock = false;
		app->GetCore().vSync = false;
		app->Run();
	}
	catch (std::runtime_error runtimeError)
	{
		FK_LOG_ERROR("Exception Caught!\n%s", runtimeError.what());
	}
	catch (...)
	{
		FK_LOG_ERROR("Exception Caught!");
		return -1;
	}
	return 0;
}


/************************************************************************************************/
