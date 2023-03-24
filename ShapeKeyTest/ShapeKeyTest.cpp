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
		debugUI			{ framework.core.RenderSystem, framework.core.GetBlockMemory() },

		inputMap		{ framework.core.GetBlockMemory() }
	{
			auto& rs = IN_framework.GetRenderSystem();
		rs.RegisterPSOLoader(DRAW_LINE_PSO, { &rs.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO });
		rs.RegisterPSOLoader(DRAW_LINE3D_PSO, { &rs.Library.RS6CBVs4SRVs, CreateDraw2StatePSO });

		InitiateScriptRuntime();

		RegisterMathTypes(GetScriptEngine(), framework.core.GetBlockMemory());
		RegisterRuntimeAPI(GetScriptEngine());


		if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
			renderWindow = std::move(res.value());

		EventNotifier<>::Subscriber sub;
		sub.Notify = &FlexKit::EventsWrapper;
		sub._ptr = &framework;

		renderWindow.Handler->Subscribe(sub);
		renderWindow.SetWindowTitle("Shape Key Test");
	}

	~ShapeKeyTest() final
	{

	}

	FlexKit::UpdateTask* Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT) final
	{
		UpdateInput();
		renderWindow.UpdateCapturedMouseInput(dT);

		debugUI.Update(renderWindow, core, dispatcher, dT);

		ImGui::NewFrame();
		ImGui::SetNextWindowPos({ (float)renderWindow.WH[0] - 600.0f, 0 });
		ImGui::SetNextWindowSize({ 600, 400 });

		ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
		ImGui::Text("Hello world!");

		ImGui::End();
		ImGui::EndFrame();
		ImGui::Render();

		return nullptr;
	}

	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) final
	{
		frameGraph.AddOutput(renderWindow.GetBackBuffer());
		core.RenderSystem.ResetConstantBuffer(constantBuffer);

		ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

		FlexKit::WorldRender_Targets targets{
			.RenderTarget	= renderWindow.GetBackBuffer(),
			.DepthTarget	= depthBuffer,
		};

		ReserveConstantBufferFunction	reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());
		ReserveVertexBufferFunction		reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());

		debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

		FlexKit::PresentBackBuffer(frameGraph, renderWindow);

		return nullptr;
	}

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final
	{
		renderWindow.Present(1, 0);
	}

	bool EventHandler(FlexKit::Event evt) final
	{
		switch (evt.InputSource)
		{
			case Event::Keyboard:
			{
				switch (evt.Action)
				{
				case Event::Pressed:
				{
				}	break;
				case Event::Release:
				{
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

		if(!renderWindow.mouseCapture)
			return debugUI.HandleInput(evt);
		else
			return false;
		}

	FlexKit::AnimatorComponent				animators;
	FlexKit::CameraComponent				cameras;
	FlexKit::CameraControllerComponent		orbitCameras;
	FlexKit::SceneNodeComponent				sceneNodes;
	FlexKit::MaterialComponent				materials;
	FlexKit::SceneVisibilityComponent		visibilityComponent;
	FlexKit::BrushComponent					brushes;
	FlexKit::PointLightComponent			pointLights;
	FlexKit::PointLightShadowMap			pointLightShadowMaps;
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

	FlexKit::CameraHandle					activeCamera = FlexKit::InvalidHandle;

	FlexKit::InputMap						inputMap;
	FlexKit::GameObject						playerObject;
	FlexKit::GameObject						character;

	FlexKit::GameObject						floorCollider;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;

	FlexKit::ImGUIIntegrator		debugUI;
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
		app->GetCore().FrameLock = true;
		app->GetCore().vSync = true;
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
