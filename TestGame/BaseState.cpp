#include "BaseState.h"
#include "PhysicsDebugVis.h"


/************************************************************************************************/


BaseState::BaseState(
	GameFramework&	IN_Framework,
	FKApplication&	IN_App,
	uint2			WH) :
		App					{ IN_App },
		FrameworkState		{ IN_Framework },
		depthBuffer			{ IN_Framework.core.RenderSystem, renderWindow.GetWH() },

		vertexBuffer		{ IN_Framework.core.RenderSystem.CreateVertexBuffer(MEGABYTE * 16, false) },
		constantBuffer		{ IN_Framework.core.RenderSystem.CreateConstantBuffer(MEGABYTE * 16, false) },
		asEngine			{ asCreateScriptEngine() },
		streamingEngine		{ IN_Framework.core.RenderSystem, IN_Framework.core.GetBlockMemory() },
		//sounds				{ IN_Framework.core.Threads,      IN_Framework.core.GetBlockMemory() },

		renderWindow{ std::get<0>(CreateWin32RenderWindow(IN_Framework.GetRenderSystem(), DefaultWindowDesc({ WH }) )) },

		render	{	IN_Framework.core.RenderSystem,
					streamingEngine,
					IN_Framework.core.GetBlockMemory(),
				},

		rtEngine{ IN_Framework.core.RenderSystem.RTAvailable() ?
			(iRayTracer&)IN_Framework.core.GetBlockMemory().allocate<RTX_RayTracer>(IN_Framework.core.RenderSystem, IN_Framework.core.GetBlockMemory()) :
			(iRayTracer&)IN_Framework.core.GetBlockMemory().allocate<NullRayTracer>() },

		// Components
		animations				{ framework.core.GetBlockMemory() },
		IK						{ framework.core.GetBlockMemory() },
		IKTargets				{ framework.core.GetBlockMemory() },
		cameras					{ framework.core.GetBlockMemory() },
		ids						{ framework.core.GetBlockMemory() },
		brushes 				{ framework.core.GetBlockMemory(), IN_Framework.GetRenderSystem() },
		materials				{ IN_Framework.core.RenderSystem, streamingEngine, framework.core.GetBlockMemory() },
		visables				{ framework.core.GetBlockMemory() },
		pointLights				{ framework.core.GetBlockMemory() },
		skeletonComponent		{ framework.core.GetBlockMemory() },
		gbuffer					{ renderWindow.GetWH(), framework.core.RenderSystem },
		shadowCasters			{ IN_Framework.core.GetBlockMemory() },
		physics					{ IN_Framework.core.Threads, IN_Framework.core.GetBlockMemory() },
		rigidBodies				{ physics },
		staticBodies			{ physics },
		characterControllers	{ physics, framework.core.GetBlockMemory() },
		orbitCameras			{ framework.core.GetBlockMemory() },
		debugUI					{ framework.core.RenderSystem, framework.core.GetBlockMemory() },
		gamepads				{ renderWindow.WindowHandle() }
{
	auto& RS = *IN_Framework.GetRenderSystem();
	RS.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO,	{ &RS.Library.RS6CBVs4SRVs, LoadSpriteTextPSO			});
	RS.RegisterPSOLoader(DRAW_PSO,				{ &RS.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO		});
	RS.RegisterPSOLoader(DRAW_TEXTURED_PSO,		{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStatePSO	});
	RS.RegisterPSOLoader(DRAW_LINE_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO		});

	RS.RegisterPSOLoader(DRAW_LINE3D_PSO,		{ &RS.Library.RS6CBVs4SRVs, CreateDraw2StatePSO			});
	RS.RegisterPSOLoader(DRAW_TRI3D_PSO,		{ &RS.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO		});

	RS.QueuePSOLoad(DRAW_PSO);
	RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);
	RS.QueuePSOLoad(DRAW_TRI3D_PSO);

	RegisterPhysicsDebugVis(RS);

	EventNotifier<>::Subscriber sub;
	sub.Notify = &EventsWrapper;
	sub._ptr = &framework;
	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("[S.P.A.R.]");

	InitiateScriptRuntime();
	RegisterMathTypes(GetScriptEngine(), framework.core.GetBlockMemory());
	RegisterRuntimeAPI(GetScriptEngine());

	gbufferPass				= materials.CreateMaterial();
	gbufferAnimatedPass		= materials.CreateMaterial();

	materials.Add2Pass(gbufferPass,         GBufferPassID);
	materials.Add2Pass(gbufferAnimatedPass, GBufferAnimatedPassID);

	gamepads.SetAxisMultiplier(2, -1.0f);
	gamepads.SetAxisMultiplier(4, -1.0f);
}


/************************************************************************************************/


BaseState::~BaseState()
{
	framework.GetRenderSystem().ReleaseVB(vertexBuffer);
	framework.GetRenderSystem().ReleaseCB(constantBuffer);
	framework.core.GetBlockMemory().release_allocation(rtEngine);

	depthBuffer.Release();
	renderWindow.Release();

	ReleaseScriptRuntime();
}


/************************************************************************************************/


UpdateTask* BaseState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	UpdateInput();
	renderWindow.UpdateCapturedMouseInput(dT);
	gamepads.Update();

	if(HUDmode != EHudMode::Disabled)
		debugUI.Update(renderWindow, core, dispatcher, dT);

	t += dT;

	BeginPixCapture();

	return nullptr;
}


/************************************************************************************************/


void BaseState::PostDrawUpdate(EngineCore& core, double dT)
{
	ProfileFunction();

	depthBuffer.Increment();
	renderWindow.Present(core.vSync ? 1 : 0, 0);
}


/************************************************************************************************/


void BaseState::Resize(const uint2 WH)
{
	if (WH[0] * WH[1] == 0)
		return;

	auto& renderSystem  = framework.GetRenderSystem();
	auto adjustedWH     = uint2{ Max(8u, WH[0]), Max(8u, WH[1]) };

	renderWindow.Resize(adjustedWH);
	depthBuffer.Resize(adjustedWH);
	gbuffer.Resize(adjustedWH);
}


/************************************************************************************************/


void BaseState::DrawDebugHUD(EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, ReserveVertexBufferFunction& reserveVB, ReserveConstantBufferFunction& reserveCB, ResourceHandle renderTarget, double dT)
{
	ProfileFunction();

	if (HUDmode != EHudMode::Disabled)
	{
		ImGui::NewFrame();
		switch (HUDmode)
		{
		case EHudMode::FPS_Counter:
			DEBUG_PrintDebugStats(core);
			break;
		case EHudMode::Profiler:
			FlexKit::profiler.DrawProfiler(uint2{ 0, 0u }, renderWindow.GetWH(), core.GetTempMemory());
			break;
		}

		ImGui::EndFrame();
		ImGui::Render();
		debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);
	}
}


/************************************************************************************************/


void BaseState::DEBUG_PrintDebugStats(EngineCore& core)
{
	ProfileFunction();

	const size_t bufferSize		= 1024;
	uint32_t VRamUsage			= (uint32_t)(core.RenderSystem._GetVidMemUsage() / MEGABYTE);
	char* TempBuffer			= (char*)core.GetTempMemory().malloc(bufferSize);
	auto updateTime				= framework.stats.dispatchTime;
	const char* RTFeatureStr	= core.RenderSystem.GetRTFeatureLevel() == RenderSystem::AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE ? "Not Available" : "Available";

	const auto shadingStats			= render.GetTimingValues();
	const auto texturePassTime		= streamingEngine.debug_GetPassTime();
	const auto textureUpdateTime	= streamingEngine.debug_GetUpdateTime();

	sprintf_s(TempBuffer, bufferSize,
		"Current VRam Usage: %u MB\n"
		"FPS: %u\n"
		"dT: %fms\n"
		"Average U+D Time: %fms\n"
		"Update + Draw Dispatch Time: %fms\n"
		//"Objects Drawn: %u\n"
		"Hardware RT: %s\n"
		"Timing:\n"
		"    GBufferPass: %fms\n"
		"    Shading: %fms\n"
		"    Cluster creation: %fms\n"
		"    Light BVH creation: %fms\n"
		"    Texture Feedback Pass: %fms\n"
		"    Texture Update: %fms\n"
		"\nBuild Date: " __DATE__ "\n",


		VRamUsage, 
		(uint32_t)framework.stats.fps,
		(float)framework.stats.dT,
		(float)framework.stats.du_average,
		updateTime,
		//(uint32_t)framework.stats.objectsDrawnLastFrame,
		RTFeatureStr,
		shadingStats.gBufferPass,
		shadingStats.shadingPass,
		shadingStats.ClusterCreation,
		shadingStats.BVHConstruction,
		texturePassTime,
		textureUpdateTime);

	ImGui::Begin("Debug Stats");
	ImGui::SetWindowPos({ 0, 0 });
	ImGui::SetWindowSize({ 400, 350 });
	ImGui::Text(TempBuffer);
	ImGui::End();
}


/************************************************************************************************/


void BaseState::PixCapture()
{
	PIX_SingleFrameCapture = true;
}


/************************************************************************************************/


void BaseState::BeginPixCapture()
{
	if (PIX_SingleFrameCapture && !PIX_CaptureInProgress)
	{
		framework.GetRenderSystem().DEBUG_BeginPixCapture();
		std::this_thread::sleep_for(60ms);


		PIX_CaptureInProgress = true;
		PIX_frameCaptures =  2;
	}
	else if (PIX_SingleFrameCapture && PIX_CaptureInProgress)
	{
		if (PIX_frameCaptures == 0)
		{
			std::this_thread::sleep_for(200ms);

			framework.GetRenderSystem().DEBUG_EndPixCapture();

			PIX_SingleFrameCapture = false;
			PIX_CaptureInProgress = false;
		}
		else
			PIX_frameCaptures--;
	}
}


/************************************************************************************************/


void BaseState::EndPixCapture()
{
	if (PIX_SingleFrameCapture && PIX_CaptureInProgress)
	{
			
	}

}
