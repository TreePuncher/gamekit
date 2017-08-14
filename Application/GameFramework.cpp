/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "stdafx.h"
#include "ConsoleSubState.h"
#include "GameFramework.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\ImageUtilities.h"

// TODO's
//	Gameplay:
//		Entity Model
//	Sound:
//	
//	Generic:
//		(DONE) Scene Loading
//		Config loading system?
//
//	Graphics:
//		(DONE) Basic Gui rendering methods (Draw Rect, etc)
//		(PARTLY) Multi-threaded Texture Uploads
//		Terrain Rendering
//			(DONE) Geometry generation
//			(PARTLY) CULLING
//			Texture Splatting
//			Normal Generation
//		(DONE) Occlusion Culling
//		(Partly Done)Animation State Machine
//		(DONE/PARTLY) 3rd Person Camera Handler
//		(DONE) Object -> Bone Attachment
//		(DONE) Texture Loading
//		(DONE) Simple Window Utilities
//		(DONE) Simple Window Elements
//		(DONE) Deferred Rendering
//		(DONE) Forward Rendering
//		(DONE) Debug Drawing Functions for Drawing Circles, Capsules, Boxes
//
//		Particles
//		Static Mesh Batcher
//
//		Bugs:
//			TextRendering Bug, Certain Characters do not get Spaces Correctly
//
//	AI:
//		Path Finding
//		State Handling
//
//	Physics:
//		(DONE) Statics
//		(DONE) TriMesh Statics
//		
//	Network:
//		Client:
//		(DONE) Connect to Server
//		(DONE) Basic Game Room
//		(DONE) Client Side Prediction
//		Server:
//		(DONE) Correct errors on Clients deviating from Server
//
//	Tools:
//		(DONE) Meta-Data for Resource Compiling
//		(PARTLY) TTF Loading
//


namespace FlexKit 
{	/************************************************************************************************/


	void HandleKeyEvents(const Event& in, GameFramework* _ptr)
	{
		switch (in.Action)
		{
		case Event::InputAction::Pressed:
		{
			switch (in.mData1.mKC[0])
			{
			case KC_ESC:
				_ptr->Quit = true;
				break;
			case KC_R:
				QueuePSOLoad(_ptr->Engine->RenderSystem, EPIPELINESTATES::TERRAIN_CULL_PSO);
				QueuePSOLoad(_ptr->Engine->RenderSystem, EPIPELINESTATES::TERRAIN_DRAW_PSO);
				QueuePSOLoad(_ptr->Engine->RenderSystem, EPIPELINESTATES::TERRAIN_DRAW_PSO_DEBUG);
				QueuePSOLoad(_ptr->Engine->RenderSystem, EPIPELINESTATES::TERRAIN_DRAW_WIRE_PSO);
				break;
			case KC_E:
			{
				_ptr->DrawTerrainDebug = !_ptr->DrawTerrainDebug;
			}	break;
			case KC_T:
				QueuePSOLoad(_ptr->Engine->RenderSystem, EPIPELINESTATES::TILEDSHADING_SHADE);
				break;
			case KC_M:
				_ptr->MouseState.Enabled = !_ptr->MouseState.Enabled;
				break;
			case KC_V:
			{
				_ptr->DP_DrawMode = EDEFERREDPASSMODE(_ptr->DP_DrawMode + 1);
				if (_ptr->DP_DrawMode == EDEFERREDPASSMODE::EDPM_COUNT)
					_ptr->DP_DrawMode = EDEFERREDPASSMODE::EDPM_DEFAULT;
			}	break;
			case KC_TILDA:
			{
				std::cout << "Pushing Console State\n";
				if (!_ptr->ConsoleActive) {
					PushSubState(_ptr, CreateConsoleSubState(_ptr));
					_ptr->ConsoleActive = true;
				}
			}	break;
			case KC_F1:
			{
				_ptr->DrawDebug			= !_ptr->DrawDebug;
				_ptr->DrawDebugStats	= !_ptr->DrawDebugStats;
			}	break;
			case KC_F2:
			{
				_ptr->ScreenSpaceReflections = !_ptr->ScreenSpaceReflections;
			}	break;
			default:
				break;
			}
		}	break;
		default:
			break;
		}
	}


	/************************************************************************************************/


	void HandleMouseEvents(const Event& in, GameFramework* _ptr) {
	switch (in.Action)
	{
	case Event::InputAction::Pressed:
	{
		if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			_ptr->MouseState.LMB_Pressed = true;
		}
	}	break;
	case Event::InputAction::Release:
	{
		if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			_ptr->MouseState.LMB_Pressed = false;
		}
	}	break;
	default:
		break;
	}
}


	/************************************************************************************************/


	void EventsWrapper(const Event& evt, void* _ptr)
	{
		auto* base = reinterpret_cast<GameFramework*>(_ptr);

		auto itr = base->SubStates.rbegin();

		switch (evt.InputSource)
		{
		case Event::Keyboard:
			HandleKeyEvents(evt, base);
		case Event::Mouse:
			HandleMouseEvents(evt, reinterpret_cast<GameFramework*>(_ptr));
			break;
		}

		while(itr != base->SubStates.rend())
		{
			if (!(*itr)->EventHandler(evt))
				break;
			itr++;
		}
	}


	/************************************************************************************************/


	bool LoadScene(EngineCore* Engine, GraphicScene* Scene, const char* SceneName)
	{
		return LoadScene(Engine->RenderSystem, Engine->Nodes, &Engine->Assets, &Engine->Geometry, SceneName, Scene, Engine->GetTempMemory());
	}


	/************************************************************************************************/


	void DrawMouseCursor(EngineCore* Engine, GameFramework* State, float2 CursorPos, float2 CursorSize)
	{
		using FlexKit::Conversion::Vect2TOfloat2;

		FlexKit::Draw_RECT Cursor;
		Cursor.BLeft  = CursorPos - CursorSize		* float2(0, 1);
		Cursor.TRight = Cursor.BLeft + CursorSize;
		Cursor.Color  = float4(1, 1, 1, 1);

		PushRect(State->Immediate, Cursor);
	}


	/************************************************************************************************/


	void ReleaseGameFramework(EngineCore* Engine, GameFramework* State)
	{
		auto RItr = State->SubStates.rbegin();
		auto REnd = State->SubStates.rend();
		while (RItr != REnd)
		{
			(*RItr)->~FrameworkState();
			RItr++;
		}

		Release(State->DefaultAssets.Font);
		Release(State->DefaultAssets.Terrain);

		ReleaseTerrain	(State->Engine->Nodes, &State->Landscape);
		//ReleaseCamera	(State->Engine->Nodes, &State->DefaultCamera);
		//ReleaseCamera	(State->Engine->Nodes, &State->DebugCamera);

		ReleaseDrawImmediate	(Engine->RenderSystem, &State->Immediate);
	}


	/************************************************************************************************/


	inline void PushSubState(GameFramework* _ptr, FrameworkState* SS)
	{
		_ptr->SubStates.push_back(SS);
	}


	/************************************************************************************************/


	void PopSubState(GameFramework* State)
	{
		if (!State->SubStates.size())
			return;

		State->SubStates.back()->~FrameworkState();
		State->SubStates.pop_back();
	}


	/************************************************************************************************/


	void UpdateGameFramework(EngineCore* Engine, GameFramework* State, double dT)
	{
		UpdateMouseInput(&State->MouseState, &Engine->Window);

		if (!State->SubStates.size()) {
			State->Quit = true;
			return;
		}

		auto RItr = State->SubStates.rbegin();
		auto REnd = State->SubStates.rend();

		while (RItr != REnd)
		{
			auto State = *RItr;
			if (!State->Update(Engine, dT))
				break;

			RItr++;
		}
	}


	/************************************************************************************************/


	void PreDrawGameFramework(EngineCore* Engine, GameFramework* State, double dT)
	{
		if (!State->SubStates.size()) {
			State->Quit = true;
			return;
		}

		if (State->DrawDebug) {
			auto RItr = State->SubStates.rbegin();
			auto REnd = State->SubStates.rend();
			while (RItr != REnd)
			{
				auto State = *RItr;
				if (!State->DebugDraw(Engine, dT))
					break;

				RItr++;
			}
			/*
			for (size_t I = 0; I < State->GScene.PLights.size(); ++I)
			{
				auto P		= State->GScene.PLights[I];
				auto PState = State->GScene.PLights.Flags->at(I);

				if ((LightBufferFlags)PState != LightBufferFlags::Unused) {
					auto POS = GetPositionW(Engine->Nodes, P.Position);
					PushCircle3D(&State->Immediate, Engine->TempAllocator, POS, P.R);
				}
			}
			*/
		}

		auto RItr = State->SubStates.rbegin();
		auto REnd = State->SubStates.rend();
		while (RItr != REnd)
		{
			auto State = *RItr;
			if (!State->PreDrawUpdate(Engine, dT))
				break;

			RItr++;
		}
	}


	/************************************************************************************************/


	bool SetDebugRenderMode(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void* USR)
	{
		GameFramework* Framework = (GameFramework*)USR;
		if (ArguementCount == 1 && Arguments->Type == ConsoleVariableType::STACK_STRING) {
			size_t Mode = 0;

			const char* VariableIdentifier = (const char*)Arguments->Data_ptr;
			for (auto Var : C->Variables)
			{
				if (!strncmp(Var.VariableIdentifier.str, VariableIdentifier, min(strlen(Var.VariableIdentifier.str), Arguments->Data_size)))
				{
					if(Var.Type == ConsoleVariableType::CONSOLE_UINT)
					Mode = *(size_t*)Var.Data_ptr;
				}
			}


			if (Mode < EDEFERREDPASSMODE::EDPM_COUNT)
			{
				Framework->DP_DrawMode = (EDEFERREDPASSMODE)Mode;
				return true;
			}
		}
		return false;
	}


	/************************************************************************************************/


	void SetActiveCamera(GameFramework* Framework, GameObjectInterface* Camera)
	{
		auto C = FindComponent(Camera, CameraComponentID);
		if (C)
			Framework->ActiveCamera = C->ComponentHandle;
	}


	/************************************************************************************************/


	GameFramework* InitiateFramework(EngineCore* Engine)
	{
		GameFramework& Framework = Engine->GetBlockMemory().allocate_aligned<GameFramework>();
		SetDebugMemory(Engine->GetDebugMemory());

		AddResourceFile("assets\\assets.gameres",				&Engine->Assets);
		AddResourceFile("assets\\ResourceFile.gameres",			&Engine->Assets);
		AddResourceFile("assets\\ShaderBallTestScene.gameres",	&Engine->Assets);

		Framework.ClearColor				= { 0.0f, 0.2f, 0.4f, 1.0f };
		Framework.Quit						= false;
		Framework.PhysicsUpdateTimer		= 0.0f;
		Framework.TerrainSplits				= 12;
		Framework.Engine					= Engine;
		Framework.DP_DrawMode				= EDEFERREDPASSMODE::EDPM_DEFAULT;

#ifdef _DEBUG
		Framework.DrawDebug					= true;
		Framework.DrawDebugStats			= true;
		Framework.DrawTerrainDebug			= true;
#else
		Framework.DrawTerrainDebug			= false;
		Framework.DrawDebug					= false;
		Framework.DrawDebugStats			= false;
#endif

		Framework.ActiveCamera				= InvalidComponentHandle;
		Framework.ActivePhysicsScene		= nullptr;
		Framework.ActiveScene				= nullptr;
		Framework.ActiveWindow				= &Engine->Window;

		Framework.DrawPhysicsDebug			= false;
		Framework.DrawTerrain				= true;
		Framework.OcclusionCulling			= false;
		Framework.ScreenSpaceReflections	= false;

		Framework.Stats.FPS						= 0;
		Framework.Stats.FPS_Counter				= 0;
		Framework.Stats.Fps_T					= 0.0;
		Framework.Stats.ObjectsDrawnLastFrame	= 0;

		ForwardPass_DESC	FP_Desc{&Engine->DepthBuffer, &Engine->Window};
		TiledRendering_Desc DP_Desc{&Engine->DepthBuffer, &Engine->Window, nullptr };

		InitiateImmediateRender	  (Engine->RenderSystem, &Framework.Immediate, Engine->GetBlockMemory(), Engine->GetTempMemory());
		

		{
			uint2	WindowRect	   = Engine->Window.WH;
			float	Aspect		   = (float)WindowRect[0] / (float)WindowRect[1];
			//InitiateCamera(Engine->RenderSystem, Engine->Nodes, &Framework.DefaultCamera, Aspect, 0.1f, 160000.0f, true);
			//InitiateCamera(Engine->RenderSystem, Engine->Nodes, &Framework.DebugCamera, Aspect, 0.1f, 160000.0f, true);

			//Framework.ActiveCamera				= &Framework.DefaultCamera;
			Framework.MouseState.NormalizedPos	= { 0.5f, 0.5f };
			Framework.MouseState.Position		= { float(WindowRect[0]/2), float(WindowRect[1] / 2) };
		}
		{
			TextureBuffer TempBuffer;
			LoadBMP("assets\\textures\\TestMap.bmp", Engine->GetTempMemory(), &TempBuffer);
			Texture2D	HeightMap = LoadTexture(&TempBuffer, Engine->RenderSystem, Engine->GetTempMemory());

			Framework.DefaultAssets.Terrain = HeightMap;

			Landscape_Desc Land_Desc;{
				Land_Desc.HeightMap = Framework.DefaultAssets.Terrain;
			};


			InitiateLandscape(Engine->RenderSystem, GetZeroedNode(Framework.Engine->Nodes), &Land_Desc, Engine->GetBlockMemory(), &Framework.Landscape);
		}

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify = &EventsWrapper;
		sub._ptr   = &Framework;
		Engine->Window.Handler.Subscribe(sub);

		Framework.DefaultAssets.Font = LoadFontAsset	("assets\\fonts\\", "fontTest.fnt", Engine->RenderSystem, Engine->GetTempMemory(), Engine->GetBlockMemory());
		
		InitateConsole(&Framework.Console, Framework.DefaultAssets.Font, Engine);
		BindUIntVar(&Framework.Console, "TerrainSplits",			&Framework.TerrainSplits);
		BindUIntVar(&Framework.Console, "FPS",						&Framework.Stats.FPS);
		BindBoolVar(&Framework.Console, "HUD",						&Framework.DrawDebugStats);
		BindBoolVar(&Framework.Console, "DrawDebug",				&Framework.DrawDebug);
		BindBoolVar(&Framework.Console, "DrawTerrain",				&Framework.DrawTerrain);
		BindBoolVar(&Framework.Console, "DrawPhysicsDebug",			&Framework.DrawPhysicsDebug);
		BindBoolVar(&Framework.Console, "OcclusionCulling",			&Framework.OcclusionCulling);
		BindBoolVar(&Framework.Console, "ScreenSpaceReflections",	&Framework.ScreenSpaceReflections);
		BindBoolVar(&Framework.Console, "FrameLock",				&Engine->FrameLock);

		AddUIntVar(&Framework.Console, "RM_Default",	EDEFERREDPASSMODE::EDPM_DEFAULT);
		AddUIntVar(&Framework.Console, "RM_Normals",	EDEFERREDPASSMODE::EDPM_SSNORMALS);
		AddUIntVar(&Framework.Console, "RM_PositionWS", EDEFERREDPASSMODE::EDPM_POSITION);
		AddUIntVar(&Framework.Console, "RM_Depth",		EDEFERREDPASSMODE::EDPM_LINEARDEPTH);

		AddConsoleFunction(&Framework.Console, { "SetRenderMode", &SetDebugRenderMode, &Framework, 1, { ConsoleVariableType::CONSOLE_UINT }});

		UploadResources(&Engine->RenderSystem);// Uploads fresh Resources to GPU

		return &Framework;
	}


	void Update(EngineCore* Engine, GameFramework* Framework, double dT)
	{
		Framework->TimeRunning += dT;

		UpdateGameFramework(Engine, Framework, dT);

		//UpdateScene		(&Framework->ActivePhysicsScene->Scene, 1.0f/60.0f, nullptr, nullptr, nullptr );
		//UpdateColliders	(&Framework->ActivePhysicsScene->Scene, Engine->Nodes);

		Engine->End = Framework->Quit;
	}


	void UpdateFixed(EngineCore* Engine, double dt, GameFramework* State)
	{
		UpdateMouseInput(&State->MouseState, &Engine->Window);
	}


	void UpdateAnimations(EngineCore* Engine, iAllocator* TempMemory, double dt, GameFramework* _ptr)
	{
		if(_ptr->ActiveScene)	
			UpdateAnimationsGraphicScene(_ptr->ActiveScene, dt);
	}


	void UpdatePreDraw(EngineCore* Core, iAllocator* TempMemory, double dt, GameFramework* Framework)
	{
		PreDrawGameFramework(Core, Framework, dt);

		if(Framework->ActiveScene)
		{
			UpdateCoreComponents(Core, dt);
			UpdateGraphicScene	(Framework->ActiveScene); // Default Scene
		}

		if (Framework->Stats.Fps_T > 1.0)
		{
			Framework->Stats.FPS         = Framework->Stats.FPS_Counter;
			Framework->Stats.FPS_Counter = 0;
			Framework->Stats.Fps_T       = 0.0;
		}

		Framework->Stats.FPS_Counter++;
		Framework->Stats.Fps_T += dt;

		if (Framework->DrawDebugStats)
		{
			uint32_t VRamUsage = GetVidMemUsage(Core->RenderSystem) / MEGABYTE;
			char* TempBuffer   = (char*)Core->GetTempMemory().malloc(512);
			auto DrawTiming    = float(GetDuration(PROFILE_SUBMISSION)) / 1000.0f;

			sprintf_s(TempBuffer, 512, "Current VRam Usage: %u MB\nFPS: %u\nDraw Time: %fms\nObjects Drawn: %u", VRamUsage, (uint32_t)Framework->Stats.FPS, DrawTiming, (uint32_t)Framework->Stats.ObjectsDrawnLastFrame);
			PrintText(&Framework->Immediate, TempBuffer, Framework->DefaultAssets.Font, { 0.0f, 0.0f }, { 0.5f, 0.5f }, float4(WHITE, 1), GetPixelSize(Core));
		}
	}


	void Draw(EngineCore* Engine, iAllocator* TempMemory, GameFramework* Framework)
	{
		ProfileBegin(PROFILE_SUBMISSION);

		auto RS = &Engine->RenderSystem;

		SubmitUploadQueues(RS);
		BeginSubmission(RS, Framework->ActiveWindow);

		auto CL = GetCurrentCommandList(RS);

		SetDepthBuffersWrite (RS, CL, { GetCurrent(&Engine->DepthBuffer) });
		ClearBackBuffer		 (RS, CL, Framework->ActiveWindow, { 0, 0, 0, 0 });
		ClearDepthBuffers	 (RS, CL, { GetCurrent(&Engine->DepthBuffer) }, DefaultClearDepthValues_0);
		ClearDepthBuffers	 (RS, CL, { GetCurrent(&Engine->Culler.OcclusionBuffer) }, DefaultClearDepthValues_0);

		Texture2D BackBuffer = GetBackBufferTexture(Framework->ActiveWindow);
		SetViewport	(CL, BackBuffer);
		SetScissor	(CL, BackBuffer.WH);

		UploadImmediate(RS, &Framework->Immediate, TempMemory, Framework->ActiveWindow);

		Camera* ActiveCamera = (Framework->ActiveCamera != InvalidComponentHandle) ? Framework->GetActiveCamera_ptr() : nullptr;

		if(	ActiveCamera &&
			Framework->ActivePhysicsScene &&
			Framework->ActiveScene &&
			Framework->ActiveWindow )
		{

			auto PVS			= FlexKit::PVS(TempMemory);
			auto Transparent	= FlexKit::PVS(TempMemory);
			auto OutputTarget	= GetRenderTarget(Framework->ActiveWindow);

			GetGraphicScenePVS(Framework->ActiveScene, ActiveCamera, &PVS, &Transparent);

			SortPVS				(Engine->Nodes, &PVS, ActiveCamera);
			SortPVSTransparent	(Engine->Nodes, &Transparent, ActiveCamera);

			Framework->Stats.ObjectsDrawnLastFrame = PVS.size() + Transparent.size();

			Free_DelayedReleaseResources(Engine->RenderSystem);

			// TODO: multi Thread these
			// Do Uploads
			{
				DeferredPass_Parameters	DPP;
				DPP.PointLightCount = Framework->ActiveScene->PLights.size();
				DPP.SpotLightCount  = Framework->ActiveScene->SPLights.size();
				DPP.Mode			= Framework->DP_DrawMode;
				DPP.WH				= GetWindowWH(Engine);

				UploadPoses	(RS, &PVS, &Engine->Geometry, TempMemory);
				UploadDeferredPassConstants	(RS, &DPP, {0.2f, 0.2f, 0.2f, 0}, &Engine->TiledRender);

				UploadCamera			(RS, Engine->Nodes, ActiveCamera, Framework->ActiveScene->PLights.size(), Framework->ActiveScene->SPLights.size(), 0.0f, Framework->ActiveWindow->WH);
				UploadGraphicScene		(Framework->ActiveScene, &PVS, &Transparent);

#if 1
				UploadLandscape		(RS, &Framework->Landscape, Engine->Nodes, ActiveCamera, true, true, Framework->TerrainSplits + 1);
#else
				
				UploadLandscape2		(
					RS, &Framework->Landscape, 
					Engine->Nodes, Framework->ActiveCamera, 
					GetFrustum(&Framework->DebugCamera, GetPositionW(Framework->Engine->Nodes, Framework->DebugCamera.Node), GetOrientation(Framework->Engine->Nodes, Framework->DebugCamera.Node)),
					true, true, Framework->TerrainSplits + 1);
#endif
			}

			// Submission
			{

				IncrementPassIndex		(&Engine->TiledRender);
				ClearTileRenderBuffers	(RS, &Engine->TiledRender);

				if(Framework->OcclusionCulling)
					OcclusionPass(RS, &PVS, &Engine->Culler, CL, &Engine->Geometry, ActiveCamera);

				//TiledRender_LightPrePass(RS, &Engine->TiledRender, State->ActiveCamera, &State->GScene.PLights, &State->GScene.SPLights, { OutputTarget.WH[0] / 8, OutputTarget.WH[1] / 16 });
				TiledRender_Fill	(RS, &PVS, &Engine->TiledRender, OutputTarget, ActiveCamera, nullptr, &Engine->Geometry,  nullptr, &Engine->Culler); // Do Early-Z?

				if(Framework->DrawTerrain)
					DrawLandscape		(RS, &Framework->Landscape, &Engine->TiledRender, Framework->TerrainSplits, ActiveCamera, Framework->DrawTerrainDebug);

				TiledRender_Shade		(RS, &PVS, &Engine->TiledRender, OutputTarget, ActiveCamera, &Framework->ActiveScene->PLights, &Framework->ActiveScene->SPLights);

				if(Framework->ScreenSpaceReflections)
				{
					TraceReflections		(Engine->RenderSystem, CL, &Engine->TiledRender, ActiveCamera, &Framework->ActiveScene->PLights, &Framework->ActiveScene->SPLights, GetWindowWH(Engine), &Engine->Reflections);
					PresentBufferToTarget	(RS, CL, &Engine->TiledRender, &OutputTarget, GetCurrentBuffer(&Engine->Reflections));
				}
				else
					PresentBufferToTarget(RS, CL, &Engine->TiledRender, &OutputTarget, &Engine->RenderSystem.NullSRV);

				ForwardPass				(&Transparent, &Engine->ForwardRender, RS, ActiveCamera, Framework->ClearColor, &Framework->ActiveScene->PLights, &Engine->Geometry);// Transparent Objects
			}
		}

		SetDepthBuffersRead(RS, CL, { GetCurrent(&Engine->DepthBuffer) });
		DrawImmediate(RS, CL, &Framework->Immediate, GetBackBufferTexture(Framework->ActiveWindow), ActiveCamera);
		CloseAndSubmit({ CL }, RS, Framework->ActiveWindow);

		ProfileEnd(PROFILE_SUBMISSION);
	}


	void PostDraw(EngineCore* Engine, iAllocator* TempMemory, double dt, GameFramework* State)
	{
		Engine->Culler.Increment();
		IncrementCurrent(&Engine->DepthBuffer);
		IncrementIndex(&Engine->Reflections);

		PresentWindow(&Engine->Window, Engine->RenderSystem);
	}


	void Cleanup(EngineCore* Engine, GameFramework* Framework)
	{
		ShutDownUploadQueues(Engine->RenderSystem);

		ReleaseConsole(&Framework->Console);
		Release(Framework->DefaultAssets.Font);

		// wait for last Frame to finish Rendering
		auto CL = GetCurrentCommandList(Engine->RenderSystem);

		for (size_t I = 0; I < 4; ++I) {
			WaitforGPU(&Engine->RenderSystem);
			IncrementRSIndex(Engine->RenderSystem);
		}

		ReleaseGameFramework(Engine, Framework);

		// Counters are at Max 3
		Free_DelayedReleaseResources(Engine->RenderSystem);
		Free_DelayedReleaseResources(Engine->RenderSystem);
		Free_DelayedReleaseResources(Engine->RenderSystem);

		Engine->GetBlockMemory().free(Framework);
		
		FreeAllResourceFiles	(&Engine->Assets);
		FreeAllResources		(&Engine->Assets);

		Engine->GetBlockMemory().release_allocation(*Engine);

		DEBUGBLOCK(PrintBlockStatus(&Engine->GetBlockMemory()));
	}


	void PostPhysicsUpdate(GameFramework*)
	{

	}


	void PrePhysicsUpdate(GameFramework*)
	{

	}



}