#include "pch.h"
#include "ProceduralGenerationTest.h"
#include "Generator.h"

#include <SceneLoadingContext.h>
#include <imgui.h>
#include <angelscript.h>
#include <ranges>

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

	constraints			{ framework.core.GetBlockMemory() },
	scriptConstraints	{ framework.core.GetBlockMemory() },

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

	auto generatorModule	= LoadScriptFile("Generator", R"(assets\scripts\TestConstraints.as)", framework.core.GetTempMemory());
	auto initate			= generatorModule->GetFunctionByName("ProcGenInitate");
	auto release			= generatorModule->GetFunctionByName("ProcGenRelease");

	if (!initate)
	{
		FK_LOG_ERROR("Failed to load ProcGen Script");
	}
	else
	{
		auto scriptContext = GetContext();
		scriptContext->Prepare(initate);
		scriptContext->Execute();

		SparseMap map{ framework.core.GetTempMemory() };
		map.SetCell({ 0, 0, 0, }, CellStates::TileNSEW);
		map.Generate(constraints, int3{ 256, 256, 1 }, framework.core.GetTempMemory());

		ReleaseContext(scriptContext);
		scriptContext->Unprepare();

		scriptContext->Prepare(release);
		scriptContext->Execute();

		generatorModule->Discard();
	}


	layer = physx.CreateLayer();

	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());

	EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Procedural Generation");

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
	new(tileSet) TileSet{};
}

void TileSet_Deconstruct(TileSet* tileSet)
{
	tileSet->~static_vector();
}

void TileSet_CopyConstruct(TileSet* tileSet, TileSet& rhs)
{
	new(tileSet) TileSet{ rhs };
}


/************************************************************************************************/


struct ScriptConstraint : public iConstraint
{
	ScriptConstraint(asIScriptObject* obj_ptr) :
		object { obj_ptr }
	{
		type = object->GetObjectType();
	}

	bool Constraint(const	SparseMap& map, const CellCoord coord)	const noexcept final
	{
		auto ctx = GetContext();

		auto function = type->GetMethodByName("Constraint");
		ctx->Prepare(function);
		ctx->SetObject(object);
		ctx->SetArgAddress(0, (void*)&map);
		ctx->SetArgObject(1, (void*)&coord);
		ctx->Execute();

		auto byte = ctx->GetReturnByte();

		ctx->Unprepare();
		ReleaseContext(ctx);

		return byte;
	}

	bool Apply(SparseMap& map, const CellCoord coord)	const noexcept final
	{
		auto ctx = GetContext();

		auto function = type->GetMethodByName("Apply");
		ctx->Prepare(function);
		ctx->SetObject(object);
		ctx->SetArgAddress(0, (void*)&map);
		ctx->SetArgObject(1, (void*)&coord);
		ctx->Execute();

		auto byte = ctx->GetReturnByte();

		ctx->Unprepare();
		ReleaseContext(ctx);

		return byte;
	}

	TileSet GetInvalidTiles(const	SparseMap& map, const CellCoord coord)	const noexcept final
	{
		auto ctx = GetContext();

		auto function = type->GetMethodByName("GetInvalidTiles");
		auto res = ctx->Prepare(function);
		ctx->SetObject(object);
		ctx->SetArgAddress(0, (void*)&map);
		ctx->SetArgObject(1, (void*)&coord);
		res = ctx->Execute();

		FlexKit::static_vector<CellState_t, CellStates::Count> tileSet{ *((TileSet*)(ctx->GetAddressOfReturnValue())) };

		ctx->Unprepare();
		ReleaseContext(ctx);

		return tileSet;
	}

	asITypeInfo*		type	= nullptr;
	asIScriptObject*	object	= nullptr;
};

void GenerationTest::RegisterConstraint(asIScriptObject* object)
{
	constraints.push_back(std::make_unique<ScriptConstraint>(object));
}

void GenerationTest::UnregisterConstraint(asIScriptObject* object)
{
	auto res = std::ranges::find_if(
		scriptConstraints,
		[&](auto& v) -> bool
		{
			return v.object == object;
		});

	if (res != scriptConstraints.end())
	{
		constraints.erase(
				std::remove_if(
					constraints.begin(),
					constraints.end(),
					[&](auto& _ptr) { return _ptr.get() == res; }),
				constraints.end());
				
		scriptConstraints.remove_unstable(res);
	}
}


/************************************************************************************************/


TileSet SparseMapGetSuperState(SparseMap* map, uint3& xyz)
{
	return map->GetSuperState(xyz);
}

void GenerationTest::RegisterGenerationAPI()
{
	auto engine_ptr = GetScriptEngine();

	int res = -1;

	res = engine_ptr->RegisterEnum("CellStates");
	res = engine_ptr->RegisterEnumValue("CellStates", "TileE", CellStates::TileE);			FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileS", CellStates::TileS);			FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileN", CellStates::TileN);			FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileW", CellStates::TileW);			FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterEnumValue("CellStates", "TileEW", CellStates::TileEW);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileSW", CellStates::TileSW);		FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterEnumValue("CellStates", "TileNE", CellStates::TileNE);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileNW", CellStates::TileNW);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileSE", CellStates::TileSE);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileSW", CellStates::TileSW);		FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterEnumValue("CellStates", "TileNSE", CellStates::TileNSE);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileNSW", CellStates::TileNSW);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileSWE", CellStates::TileSWE);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileNWE", CellStates::TileNWE);		FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterEnumValue("CellStates", "TileNSEW", CellStates::TileNSEW);	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileCount", CellStates::Count);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterEnumValue("CellStates", "TileSuper", CellStates::Super);		FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterEnumValue("CellStates", "TileError", CellStates::Error);		FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterObjectType("TileSet", sizeof(TileSet), asOBJ_VALUE | asOBJ_APP_CLASS_CAK);																		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectBehaviour("TileSet", asBEHAVE_CONSTRUCT, "void Construct()",					asFUNCTION(TileSet_Construct),		asCALL_CDECL_OBJFIRST);	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectBehaviour("TileSet", asBEHAVE_CONSTRUCT, "void Construct(const TileSet& in)",	asFUNCTION(TileSet_CopyConstruct),	asCALL_CDECL_OBJFIRST);	FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectBehaviour("TileSet", asBEHAVE_DESTRUCT,  "void Deconstruct()",					asFUNCTION(TileSet_Deconstruct),	asCALL_CDECL_OBJFIRST);	FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterObjectMethod("TileSet", "void opAssign(const TileSet& in)",	asMETHODPR(TileSet, operator=, (const TileSet&), TileSet&), asCALL_THISCALL);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectMethod("TileSet", "uint64 push_back(uint8& in)",		asMETHOD(TileSet, push_back), asCALL_THISCALL);										FK_ASSERT(res >= 0);


	res = engine_ptr->RegisterObjectType("SparseMap", 0, asOBJ_REF | asOBJ_NOCOUNT);																		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterObjectMethod("SparseMap", "TileSet GetSuperState(uint3& in)",		asFUNCTION(SparseMapGetSuperState), asCALL_CDECL_OBJFIRST);	FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterInterface("iConstraint");																		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterInterfaceMethod("iConstraint", "bool	Constraint			(SparseMap@, const uint3)");		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterInterfaceMethod("iConstraint", "bool	Apply				(SparseMap@, const uint3)");		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterInterfaceMethod("iConstraint", "TileSet GetInvalidTiles	(SparseMap@, const uint3)");		FK_ASSERT(res >= 0);

	res = engine_ptr->RegisterGlobalFunction("void RegisterConstraint(iConstraint@)",	asMETHOD(GenerationTest, RegisterConstraint), asCALL_THISCALL_ASGLOBAL, this);		FK_ASSERT(res >= 0);
	res = engine_ptr->RegisterGlobalFunction("void UnregisterConstraint(iConstraint@)", asMETHOD(GenerationTest, UnregisterConstraint), asCALL_THISCALL_ASGLOBAL, this);	FK_ASSERT(res >= 0);
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

	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	FlexKit::WorldRender_Targets targets{
		.RenderTarget = renderWindow.GetBackBuffer(),
		.DepthTarget = depthBuffer,
	};
	ReserveConstantBufferFunction	reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());
	ReserveVertexBufferFunction		reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());

	/*
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
	*/

	debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

	FlexKit::PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void GenerationTest::PostDrawUpdate(FlexKit::EngineCore& core, double dT)
{
	renderWindow.Present(0, 0);

	core.RenderSystem.ResetConstantBuffer(constantBuffer);
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
