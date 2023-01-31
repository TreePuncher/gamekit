#include "pch.h"
#include "ProceduralGenerationTest.h"
#include "Generator.h"

#include <SceneLoadingContext.h>
#include <imgui.h>
#include <angelscript.h>

using namespace FlexKit;


/************************************************************************************************/


GenerationTest::GenerationTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState		{ IN_framework },

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

	physx					{ framework.core.Threads, framework.core.GetBlockMemory() },
	rigidBodies				{ physx },
	staticBodies			{ physx },
	characterController		{ physx, framework.core.GetBlockMemory() },

	renderer				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	textureStreamingEngine	{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },

	gbuffer			{ { 1920, 1080 }, framework.GetRenderSystem() },
	depthBuffer		{ framework.GetRenderSystem(), { 1920, 1080 } },
	renderWindow	{},

	constantBuffer	{ framework.GetRenderSystem().CreateConstantBuffer(64 * MEGABYTE, false) },
	vertexBuffer	{ framework.GetRenderSystem().CreateVertexBuffer(64 * MEGABYTE, false) },
	runOnceQueue	{ framework.core.GetBlockMemory() },
	scene			{ framework.core.GetBlockMemory() },
	debugUI			{ framework.core.RenderSystem, framework.core.GetBlockMemory() },

	inputMap		{ framework.core.GetBlockMemory() }
{
	InitiateScriptRuntime();

	RegisterMathTypes(GetScriptEngine(), SystemAllocator);
	RegisterRuntimeAPI(GetScriptEngine());

	RegisterGenerationAPI();

	layer = physx.CreateLayer();

	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());

	EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Texture Streaming");

	// Load Test Scene
	SceneLoadingContext loadCtx{
		.scene = scene,
		.layer = layer,
		.nodes = Vector<FlexKit::NodeHandle>{ framework.core.GetBlockMemory() }
	};


	AddAssetFile(R"(assets\TextureStreaming.gameres)");
	//auto loadSuccess = LoadScene(framework.core, loadCtx, 1234);
	//AddAssetFile(R"(assets\WCFTileSet.gameres)");


	/*
	// Setup Camera
	auto& orbitComponent = camera.AddView<OrbitCameraBehavior>();
	activeCamera = orbitComponent.camera;

	auto cameraHandle = GetCameraControllerCamera(camera);
	SetCameraAspectRatio(cameraHandle, renderWindow.GetAspectRatio());
	SetCameraFOV(cameraHandle, (float)FlexKit::pi / 4.0f);

	activeCamera = cameraHandle;

	SetCameraAspectRatio(orbitComponent.camera, renderWindow.GetAspectRatio());
	SetCameraFOV(orbitComponent.camera, (float)pi / 4.0f);

	OrbitCameraTranslate(camera, { 0, 3, 0 });
	*/
}


/************************************************************************************************/


GenerationTest::~GenerationTest()
{
	ReleaseScriptRuntime();
}


/************************************************************************************************/


void TileSet_Construct(TileSet* tileSet)
{
	new(tileSet) TileSet();
}


void GenerationTest::RegisterGenerationAPI()
{
	auto engine_ptr = GetScriptEngine();

	int res = -1;

	res = engine_ptr->RegisterObjectType("TileSet", sizeof(TileSet), asOBJ_VALUE | asOBJ_APP_CLASS_CAK);	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectBehaviour("TileSet", asBEHAVE_CONSTRUCT, "void Construct()", asFUNCTION(TileSet_Construct), asCALL_CDECL_OBJFIRST);	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectMethod("TileSet", "void opAssign(const TileSet& rhs)", asMETHODPR(TileSet, operator=, TileSet&, TileSet&), asCALL_THISCALL);	FK_ASSERT(res >= 0);


	res = engine_ptr->RegisterObjectType("SparseMap", 0, asOBJ_REF | asOBJ_NOCOUNT);						FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterInterface("iConstraint");																			FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterInterfaceMethod("iConstraint", "bool	Constraint			(SparseMap& map, const uint3 coord");	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterInterfaceMethod("iConstraint", "bool	Apply				(SparseMap& map, const uint3 coord");	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterInterfaceMethod("iConstraint", "TileSet GetInvalidTiles	(SparseMap& map, const uint3 coord");	FK_ASSERT(res >= 0);
}


/************************************************************************************************/


FlexKit::UpdateTask* GenerationTest::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	UpdateInput();
	renderWindow.UpdateCapturedMouseInput(dT);

	OrbitCameraUpdate(camera, renderWindow.mouseState, dT);

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


FlexKit::UpdateTask* GenerationTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
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

	textureStreamingEngine.TextureFeedbackPass(dispatcher, frameGraph, activeCamera, core.RenderSystem.GetTextureWH(targets.RenderTarget), res.entityConstants, res.passes, res.skinnedDraws, reserveCB, reserveVB);

	debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

	FlexKit::PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void GenerationTest::PostDrawUpdate(FlexKit::EngineCore&, double dT)
{
	renderWindow.Present(0, 0);
}


/************************************************************************************************/


bool GenerationTest::EventHandler(FlexKit::Event evt)
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
				case KC_M:
					renderWindow.ToggleMouseCapture();

					return true;
				}
			}	break;
			}
		}	break;
		default:
			break;
	}

	OrbitCameraHandleEvent(camera, evt);

	if ((evt.InputSource == FlexKit::Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC) ||
		(evt.InputSource == FlexKit::Event::E_SystemEvent && evt.Action == FlexKit::Event::Exit))
	{
		framework.quit = true;
		return true;
	}
	else
		return debugUI.HandleInput(evt);
}


/************************************************************************************************/
