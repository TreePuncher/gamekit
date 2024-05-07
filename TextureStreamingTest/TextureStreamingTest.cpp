#include "pch.h"
#include "CameraUtilities.h"
#include "SceneLoadingContext.h"
#include "TextureStreamingTest.h"
#include "ProfilingUtilities.h"

#include <imgui.h>
#include <fmt\format.h>


using namespace FlexKit;


/************************************************************************************************/


const uint2 resolution = uint2{ 1920, 1080 };

TextureStreamingTest::TextureStreamingTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState		{ IN_framework },

	animators				{ framework.core.GetBlockMemory() },
	brushes					{ framework.core.GetBlockMemory() },
	cameras					{ framework.core.GetBlockMemory() },
	sceneNodes				{},
	materials				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	visibilityComponent		{ framework.core.GetBlockMemory() },
	pointLights				{ framework.core.GetBlockMemory() },
	orbitCameras			{ framework.core.GetBlockMemory() },
	pointLightShadowMaps	{ framework.core.GetBlockMemory() },
	ikComponent				{ framework.core.GetBlockMemory() },
	skeletons				{ framework.core.GetBlockMemory() },
	triggers				{ framework.core.GetBlockMemory(), framework.core.GetBlockMemory() },

	physx					{ framework.core.Threads, framework.core.GetBlockMemory() },
	rigidBodies				{ physx },
	staticBodies			{ physx },

	renderer				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	textureStreamingEngine	{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },

	gbuffer			{ { resolution }, framework.GetRenderSystem() },
	depthBuffer		{ framework.GetRenderSystem(), { resolution } },

	constantBuffer	{ framework.GetRenderSystem().CreateConstantBuffer(128 * MEGABYTE, false) },
	vertexBuffer	{ framework.GetRenderSystem().CreateVertexBuffer(128 * MEGABYTE, false) },
	runOnceQueue	{ framework.core.GetBlockMemory() },
	scene			{ framework.core.GetBlockMemory() }
{	// Setup Window and input
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .fullscreen = true, .height = resolution[1], .width = resolution[0], }); res)
	{
		renderWindow = res;
	}
	else
	{
		FK_LOG_ERROR("Failed to create render window!");

		throw std::runtime_error{ std::format("Failed to create render window! arguments [fullscreen = {}, width = {}, height = {} ]", false, resolution[0], resolution[1])};
	}

	framework.core.activeWindow = renderWindow;
	framework.GetRenderSystem().DEBUG_AttachPIX();

	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify	= &FlexKit::EventsWrapper;
	sub._ptr	= &framework;

	renderWindow->Handler.Subscribe(sub);
	renderWindow->SetWindowTitle("Texture Streaming");

	// Load Test Scene
	layer = physx.CreateLayer(false);

	SceneLoadingContext loadCtx{
		.scene = scene,
		.layer = layer,
		.nodes = Vector<FlexKit::NodeHandle>{ framework.core.GetBlockMemory() }
	};

	//AddAssetFile(R"(assets\SanMiguel.gameres)");
	AddAssetFile(R"(assets\TextureStreaming.gameres)");
	//AddAssetFile(R"(assets\ShadowTest.gameres)");

	if (!LoadScene(framework.core, loadCtx, "Scene"))
		throw std::runtime_error("Failed to load scene!");

	if(false)
	scene.QueryFor(
		[&](GameObject& gameObject, LightView& light, SceneNodeView& node)
		{
			light.SetType(FlexKit::LightType::SpotLightNoShadows);
			light.SetOuterAngle((float)pi / 1.3f);
			light.SetIntensity(2 * light.GetIntensity());
			//light.SetIntensity(4096);
			light.SetRadius(50.0f);
			node.SetPosition({ 16.8f, -2.0f, -6.0f });
		},
		LightQuery{}, SceneNodeReq{});

	// Setup Camera
	auto& orbitComponent = orbitCamera.AddView<OrbitCameraBehavior>();
	activeCamera = orbitComponent.camera;
	orbitComponent.moveRate = 400;
	orbitComponent.acceleration = 100;

	if(!rotate)
		renderWindow->ToggleMouseCapture();

	SetCameraAspectRatio(orbitComponent.camera, renderWindow->GetAspectRatio());
	SetCameraFOV(orbitComponent.camera, (float)pi / 4.0f);

	//OrbitCameraPitch(orbitCamera, float(pi / 2.0f));
	//OrbitCameraTranslate(orbitCamera, { 16.8, -2, -6 });
}


/************************************************************************************************/


TextureStreamingTest::~TextureStreamingTest()
{
	scene.ClearScene();

	renderWindow->Release();
	framework.GetRenderSystem().ReleaseVB(vertexBuffer);
	framework.GetRenderSystem().ReleaseCB(constantBuffer);
}


/************************************************************************************************/


FlexKit::UpdateTask* TextureStreamingTest::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	FK_LOG_9("Frame Begin");
	ProfileFunctionTextName(Update);

	UpdateInput();

	renderWindow->UpdateCapturedMouseInput(dT);

	OrbitCameraUpdate(orbitCamera, renderWindow->mouseState, dT);

	if(false)
	scene.QueryFor(
		[&](GameObject& gameObject, auto&& light, auto&& node)
		{
			node.Yaw(dT * pi * 0.125f);
		},
		LightQuery{}, SceneNodeReq{});

	if(rotate)
		OrbitCameraYaw(orbitCamera, pi * dT / 3.0f);

	cameras.MarkDirty(activeCamera);

	if (framework.ImGuiAvailable())
	{
		ImGui::SetNextWindowPos({ (float)renderWindow->WH[0] - 400.0f, 0 });
		ImGui::SetNextWindowSize({ 400, 1080 });

		ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

		const auto memoryStats = framework.core.GetBlockMemory().GetStats();

		auto memoryInUse = (memoryStats.smallBlocksAllocated * 64 +
			memoryStats.mediumBlocksAllocated * 2048 +
			memoryStats.largeBlocksAllocated * KILOBYTE * 128) / MEGABYTE;

		auto temp = core.RenderSystem.directFence->GetCompletedValue();
		size_t space0 = 0;
		size_t space1 = 0;
		size_t space2 = 0;
		for (auto& a : renderer.UAVPool.freeRanges)
			if (a.frameID < temp)
				space0 += a.blockCount;

		for (auto& a : renderer.RTPool.freeRanges)
			if (a.frameID < temp)
				space1 += a.blockCount;

		for (auto& a : renderer.UAVTexturePool.freeRanges)
			if (a.frameID < temp)
				space2 += a.blockCount;

		space0 = (space0 * 64 * KILOBYTE) / MEGABYTE;
		space1 = (space1 * 64 * KILOBYTE) / MEGABYTE;
		space2 = (space2 * 64 * KILOBYTE) / MEGABYTE;

		auto vidMemStats = core.RenderSystem._GetVidMemStats();
		size_t textureBlocksInUse = textureStreamingEngine.TilesAllocated();
		size_t textureBlocksFree = textureStreamingEngine.TilesFree();
		size_t textureBlocksStale = textureStreamingEngine.TilesStale();

		auto pos = GetPositionW(GetCameraNode(activeCamera));

		auto str = fmt::format(
			"Debug Stats\n"
			"FPS: {}\n"
			"Occlusion Culling: {}\n"
			"SmallBlocks: {} / {}\n"
			"MediumBlocks: {} / {}\n"
			"LargeBlocks: {} / {}\n"
			"Memory in use: {}MB\n"
			"M to toggle mouse\n"
			"T to toggle texture streaming\n"
			"R to toggle rotating camera\n"
			"V to toggle vsync\n"
			"UAV buffer   Pool space left: {}MB\n"
			"RenderTarget Pool space left: {}MB\n"
			"UAV texture  Pool space left: {}MB\n"
			"Video Memory {}/{}\n"
			"Camera Position:"
			"	X: {}\n"
			"	Y: {}\n"
			"	Z, {}\n"
			"Texture Blocks:\n"
			"	inuse:  {}\n"
			"	free:   {}\n"
			"	stale:  {}\n"
			"	total:  {}\n",
			framework.stats.fps,
			renderer.occlusionCulling ? "Enabled" : "Disabled", 
			memoryStats.smallBlocksAllocated, memoryStats.totalSmallBlocks,
			memoryStats.mediumBlocksAllocated, memoryStats.totalMediumBlocks,
			memoryStats.largeBlocksAllocated, memoryStats.totalLargeBlocks,
			memoryInUse,
			space0, space1, space2,
			vidMemStats.used / MEGABYTE, vidMemStats.available / MEGABYTE,
			pos.x, pos.y, pos.z,
			textureBlocksInUse, textureBlocksFree, textureBlocksStale, textureBlocksInUse + textureBlocksFree + textureBlocksStale);


		ImGui::Text(str.c_str());

		ImGui::End();
	}

	return nullptr;
}


/************************************************************************************************/


FlexKit::UpdateTask* TextureStreamingTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	ProfileFunctionTextName(Draw);

	frameGraph.AddOutput(renderWindow->GetBackBuffer());

	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	FlexKit::WorldRender_Targets targets{
		.RenderTarget = renderWindow->GetBackBuffer(),
		.DepthTarget = depthBuffer,
	};

	ReserveConstantBufferFunction	reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());
	ReserveVertexBufferFunction		reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());


	static double T = 0.0;
	T += dT;

	auto& transformUpdate	= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);

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


	framework.DrawDebugUI(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow->GetBackBuffer());


	PresentBackBuffer(frameGraph, *renderWindow);

	frameGraph.SubmitDirect(dispatcher, core.RenderSystem, core.GetTempMemoryMT());

	if (streamingUpdates)
		textureStreamingEngine.TextureFeedbackPass(dispatcher, frameGraph, activeCamera, core.RenderSystem.GetTextureWH(targets.RenderTarget), res.entityConstants, res.passes, res.animationResources, reserveCB, reserveVB, dT, core.GetTempMemoryMT());

	return nullptr;
}


/************************************************************************************************/


void TextureStreamingTest::PostDrawUpdate(FlexKit::EngineCore& core, double dT)
{
	FK_LOG_9("Frame End");

	core.RenderSystem.ResetConstantBuffer(constantBuffer);
	renderWindow->Present(core.vSync ? 1 : 0, 0);

	depthBuffer.Increment();
}


/************************************************************************************************/


bool TextureStreamingTest::EventHandler(FlexKit::Event evt)
{
	switch (evt.InputSource)
	{
		case Event::Keyboard:
		{
			switch (evt.Action)
			{
			case Event::Release:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_O:
					renderer.occlusionCulling = !renderer.occlusionCulling;
					return true;
				case KC_N:
					framework.core.RenderSystem.QueuePSOLoad(SHADINGPASS);
					return true;
				case KC_M:
					rotate = false;
					renderWindow->ToggleMouseCapture();
					return true;
				case KC_P:
					framework.GetRenderSystem().DEBUG_BeginPixCapture();
					return true;
				case KC_L:
					framework.GetRenderSystem().DEBUG_EndPixCapture();
					return true;
				case KC_T:
					streamingUpdates = !streamingUpdates;
					return true;
				case KC_Y:
					if (true)
						scene.QueryFor(
							[&](GameObject& gameObject, LightView& light, SceneNodeView& node)
							{
								light.SetType(FlexKit::LightType::SpotLight);
								light.SetOuterAngle((float)pi / 1.3f);
							},
							LightQuery{}, SceneNodeReq{});
					return true;
				case KC_V:
					framework.core.vSync = !framework.core.vSync;
					return true;
				case KC_R:
					rotate = !rotate;

					if(rotate)
						renderWindow->EnableCaptureMouse(false);
					return true;
				case KC_I:
				{
					renderer.passHistories.ResetAll();
				}	return true;
				}
			}	break;
			}
		}	break;
		default:
			break;
	}

	OrbitCameraHandleEvent(orbitCamera, evt);

	if ((evt.InputSource == FlexKit::Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC) ||
		(evt.InputSource == FlexKit::Event::E_SystemEvent && evt.Action == FlexKit::Event::Exit))
	{
		framework.quit = true;
		return true;
	}

	return false;
}


/**********************************************************************

Copyright (c) 2014-2023 Robert May

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
