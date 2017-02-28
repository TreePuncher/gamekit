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

#include "ConsoleSubState.h"
#include "GameFramework.h"
#include "MenuState.h"

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
//		Multi-threaded Texture Uploads
//		Terrain Rendering
//			Texture Splatting
//		Occlusion Culling
//		Animation State Machine
//		(DONE/PARTLY) 3rd Person Camera Handler
//		(DONE) Object -> Bone Attachment
//		(DONE) Texture Loading
//		(DONE) Simple Window Utilities
//		(DONE) Simple Window Elements
//		(DONE) Deferred Rendering
//		(DONE) Forward Rendering
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
//	Network:
//		Client:
//		Server:
//
//	Tools:
//		(DONE) Meta-Data for Resource Compiling
//


/************************************************************************************************/

using namespace FlexKit;


void HandleKeyEvents(const Event& in, GameFramework* _ptr) {
	//_ptr->Quit = true;

	switch (in.Action)
	{
	case Event::InputAction::Pressed:
	{
		switch (in.mData1.mKC[0])
		{
		case KC_ESC:
			_ptr->Engine->End = true;
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
		case KC_C:
			QueuePSOLoad(_ptr->Engine->RenderSystem, EPIPELINESTATES::TILEDSHADING_SHADE);
			break;
		case KC_Q:
			PushSubState(_ptr, CreateConsoleSubState(_ptr->Engine, &_ptr->Console, _ptr));
			break;
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
			std::cout << "KC_LMB PRESSED\n";
		}
	}	break;
	case Event::InputAction::Release:
	{
		if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			_ptr->MouseState.LMB_Pressed = false;
			std::cout << "KC_LMB RELEASED\n";
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
	while(itr != base->SubStates.rend())
	{
		if (*itr && (*itr)->EventHandler) 
		{
			if (!(*itr)->EventHandler((SubState*)(*itr), evt))
				break;
		}
		itr++;
	}

	switch (evt.InputSource)
	{
	case Event::Keyboard:
		HandleKeyEvents(evt, base);
	case Event::Mouse:
		HandleMouseEvents(evt, reinterpret_cast<GameFramework*>(_ptr));
		break;
	}
}


/************************************************************************************************/


bool LoadScene(GameFramework* State, const char* SceneName)
{
	auto Engine = State->Engine;
	return LoadScene(Engine->RenderSystem, &Engine->Nodes, &Engine->Assets, &Engine->Geometry, SceneName, &State->GScene, Engine->TempAllocator);
}


/************************************************************************************************/


void DrawMouseCursor(EngineMemory* Engine, GameFramework* State, float2 CursorPos, float2 CursorSize)
{
	using FlexKit::Conversion::Vect2TOfloat2;

	FlexKit::Draw_RECT Cursor;
	Cursor.BLeft  = CursorPos - CursorSize		* float2(0, 1);
	Cursor.TRight = Cursor.BLeft + CursorSize;

	Cursor.Color  = float4(1, 1, 1, 1);

	PushRect(State->GUIRender, Cursor);
}


/************************************************************************************************/


void ReleaseGameFramework(EngineMemory* Engine, GameFramework* State)
{
	auto RItr = State->SubStates.rbegin();
	auto REnd = State->SubStates.rend();
	while (RItr != REnd)
	{
		auto VTable = *RItr;
		if (VTable->Release) {
			VTable->Release(reinterpret_cast<SubState*>(VTable));
		}
		RItr++;
	}

	Release(State->DefaultAssets.Font);
	Release(State->DefaultAssets.Terrain);

	ReleaseTerrain(State->Nodes, &State->Landscape);
	ReleaseCamera(State->Nodes, &State->DefaultCamera);

	ReleaseGraphicScene(&State->GScene);
	ReleaseDrawGUI(Engine->RenderSystem, &State->GUIRender);
}


/************************************************************************************************/


inline void PushSubState(GameFramework* _ptr, SubState* SS)
{
	_ptr->SubStates.push_back(GetStateVTable(SS));
}


/************************************************************************************************/


void PopSubState(GameFramework* State)
{
	auto Top = State->SubStates.back();
	Top->Release(reinterpret_cast<SubState*>(Top));

	State->SubStates.pop_back();
}


/************************************************************************************************/


void UpdateGameFramework(EngineMemory* Engine, GameFramework* State, double dT)
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
		auto VTable = *RItr;
		if (VTable->Update && !VTable->Update(reinterpret_cast<SubState*>(VTable), Engine, dT))
			break;

		RItr++;
	}
}


/************************************************************************************************/


void PreDrawGameFramework(EngineMemory* Engine, GameFramework* State, double dT)
{
	if (!State->SubStates.size()) {
		State->Quit = true;
		return;
	}

	auto RItr = State->SubStates.rbegin();
	auto REnd = State->SubStates.rend();
	while (RItr != REnd)
	{
		auto VTable = *RItr;
		if (VTable->PreDrawUpdate && !VTable->PreDrawUpdate(reinterpret_cast<SubState*>(VTable), Engine, dT))
			break;

		RItr++;
	}
}


/************************************************************************************************/


SubStateVTable* GetStateVTable(SubState* _ptr)
{
	return &_ptr->VTable;
}


/************************************************************************************************/


extern "C"
{
	GAMESTATEAPI GameFramework* InitiateBaseGameState(EngineMemory* Engine)
	{
		GameFramework& State = Engine->BlockAllocator.allocate_aligned<GameFramework>();
		
		AddResourceFile("assets\\ResourceFile.gameres", &Engine->Assets);
		AddResourceFile("assets\\ShaderBallTestScene.gameres", &Engine->Assets);

		State.ClearColor				= { 0.0f, 0.2f, 0.4f, 1.0f };
		State.Nodes						= &Engine->Nodes;
		State.Quit						= false;
		State.PhysicsUpdateTimer		= 0.0f;
		State.TerrainSplits				= 12;
		State.ActiveWindow				= &Engine->Window;
		State.Engine					= Engine;
		State.DP_DrawMode				= EDEFERREDPASSMODE::EDPM_DEFAULT;
		State.ActiveScene				= &State.GScene;

		ForwardPass_DESC FP_Desc{&Engine->DepthBuffer, &Engine->Window};
		TiledRendering_Desc DP_Desc{&Engine->DepthBuffer, &Engine->Window, nullptr };

		InitiateScene			  (&Engine->Physics, &State.PScene, Engine->BlockAllocator);
		InitiateGraphicScene	  (&State.GScene, Engine->RenderSystem, &Engine->Assets, &Engine->Nodes, &Engine->Geometry, Engine->BlockAllocator, Engine->TempAllocator);
		InitiateDrawGUI			  (Engine->RenderSystem, &State.GUIRender, Engine->TempAllocator);

		//InitateConsole			  (&State.Console, Engine);
		
		InitiateLineSet(Engine->RenderSystem, Engine->BlockAllocator, &State.DebugLines);

		{
			uint2	WindowRect	= Engine->Window.WH;
			float	Aspect		= (float)WindowRect[0] / (float)WindowRect[1];
			InitiateCamera(Engine->RenderSystem, Engine->Nodes, &State.DefaultCamera, Aspect, 0.01f, 10000.0f, true);
			State.ActiveCamera = &State.DefaultCamera;

			State.MouseState.NormalizedPos = { 0.5f, 0.5f };
			State.MouseState.Position = { float(WindowRect[0]/2), float(WindowRect[1] / 2) };
		}

		{
			Landscape_Desc Land_Desc = { 
				State.DefaultAssets.Terrain
			};

			State.DefaultAssets.Terrain = LoadTextureFromFile("assets\\textures\\HeightMap_1.DDS", Engine->RenderSystem, Engine->BlockAllocator);

			InitiateLandscape(Engine->RenderSystem, GetZeroedNode(State.Nodes), &Land_Desc, Engine->BlockAllocator, &State.Landscape);
		}

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify = &EventsWrapper;
		sub._ptr   = &State;
		Engine->Window.Handler.Subscribe(sub);

		State.DefaultAssets.Font = LoadFontAsset	("assets\\fonts\\", "fontTest.fnt", Engine->RenderSystem, Engine->TempAllocator, Engine->BlockAllocator);

		auto MenuSubState = CreateMenuState(&State, Engine);
		PushSubState(&State, MenuSubState);

		UploadResources(&Engine->RenderSystem);// Uploads fresh Resources to GPU

		return &State;
	}


#include<thread>

	GAMESTATEAPI void Update(EngineMemory* Engine, GameFramework* State, double dt)
	{
		UpdateGameFramework(Engine, State, dt);

		UpdateScene		(&State->PScene, 1.0f/60.0f, nullptr, nullptr, nullptr );
		UpdateColliders	(&State->PScene, &Engine->Nodes);

		Engine->End = State->Quit;
	}


	GAMESTATEAPI void UpdateFixed(EngineMemory* Engine, double dt, GameFramework* State)
	{
		UpdateMouseInput(&State->MouseState, &Engine->Window);
	}


	GAMESTATEAPI void UpdateAnimations(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameFramework* _ptr)
	{
		UpdateAnimationsGraphicScene(&_ptr->GScene, dt);
	}


	GAMESTATEAPI void UpdatePreDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameFramework* State)
	{
		PreDrawGameFramework(Engine, State, dt);

		UpdateTransforms	(State->Nodes);
		UpdateCamera		(Engine->RenderSystem, State->Nodes, State->ActiveCamera, dt);
		UpdateGraphicScene	(&State->GScene); // Default Scene
	}


	GAMESTATEAPI void Draw(EngineMemory* Engine, iAllocator* TempMemory, GameFramework* State)
	{
		auto RS = &Engine->RenderSystem;

		BeginSubmission(RS, State->ActiveWindow);

		auto PVS			= TempMemory->allocate_aligned<FlexKit::PVS>();
		auto Transparent	= TempMemory->allocate_aligned<FlexKit::PVS>();
		auto CL				= GetCurrentCommandList(RS);
		auto OutputTarget	= GetRenderTarget(State->ActiveWindow);

		GetGraphicScenePVS(State->ActiveScene, State->ActiveCamera, &PVS, &Transparent);

		SortPVS(State->Nodes, &PVS, State->ActiveCamera);
		SortPVSTransparent(State->Nodes, &Transparent, State->ActiveCamera);

		Free_DelayedReleaseResources(Engine->RenderSystem);

		// TODO: multi Thread these
		// Do Uploads
		{

			{
				Draw_LineSet_3D Lines;
				Lines.C = State->ActiveCamera;
				Lines.Lines = &State->DebugLines;
				UploadLineSegments(Engine->RenderSystem, &State->DebugLines);
				PushLineSet(&State->GUIRender, Lines);
			}

			DeferredPass_Parameters	DPP;
			DPP.PointLightCount = State->GScene.PLights.size();
			DPP.SpotLightCount  = State->GScene.SPLights.size();
			DPP.Mode			= State->DP_DrawMode;
			DPP.WH				= GetWindowWH(Engine);

			SubmitUploadQueues(RS);

			UploadGUI	(RS, &State->GUIRender, TempMemory, State->ActiveWindow);	
			UploadPoses	(RS, &PVS, &Engine->Geometry, TempMemory);

			UploadDeferredPassConstants	(RS, &DPP, {0.2f, 0.2f, 0.2f, 0}, &Engine->TiledRender);

			UploadCamera			(RS, State->Nodes, State->ActiveCamera, State->GScene.PLights.size(), State->GScene.SPLights.size(), 0.0f, State->ActiveWindow->WH);
			UploadGraphicScene		(&State->GScene, &PVS, &Transparent);
			UploadLandscape			(RS, &State->Landscape, State->Nodes, State->ActiveCamera, false, true, State->TerrainSplits + 1);

		}

		// Submission
		{
			//SetDepthBuffersWrite(RS, CL, { GetCurrent(State->DepthBuffer) });

			ClearBackBuffer		 (RS, CL, State->ActiveWindow, { 0, 0, 0, 0 });
			ClearDepthBuffers	 (RS, CL, { GetCurrent(&Engine->DepthBuffer) }, DefaultClearDepthValues_0);

			Texture2D BackBuffer = GetBackBufferTexture(State->ActiveWindow);
			SetViewport	(CL, BackBuffer);
			SetScissor	(CL, BackBuffer.WH);

			IncrementPassIndex		(&Engine->TiledRender);
			ClearTileRenderBuffers	(RS, &Engine->TiledRender);

			//TiledRender_LightPrePass(RS, &Engine->TiledRender, State->ActiveCamera, &State->GScene.PLights, &State->GScene.SPLights, { OutputTarget.WH[0] / 8, OutputTarget.WH[1] / 16 });
			TiledRender_Fill			(RS, &PVS, &Engine->TiledRender, OutputTarget,  State->ActiveCamera, nullptr, &Engine->Geometry,  nullptr); // Do Early-Z?

			//DrawLandscape		(RS, &State->Landscape, &Engine->DeferredRender, State->TerrainSplits, State->ActiveCamera, false);

			TiledRender_Shade	(&PVS, &Engine->TiledRender, OutputTarget, RS, State->ActiveCamera, &State->GScene.PLights, &State->GScene.SPLights);
			ForwardPass			(&Transparent, &Engine->ForwardRender, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, &Engine->Geometry);// Transparent Objects

			DrawGUI(RS, CL, &State->GUIRender, GetBackBufferTexture(State->ActiveWindow));       
			CloseAndSubmit({ CL }, RS, State->ActiveWindow);
			State->DebugLines.LineSegments.clear();
		}
	}


	GAMESTATEAPI void PostDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameFramework* State)
	{
		IncrementCurrent(&Engine->DepthBuffer);

		PresentWindow(&Engine->Window, Engine->RenderSystem);
	}


	GAMESTATEAPI void Cleanup(EngineMemory* Engine, GameFramework* State)
	{

		Release(State->DefaultAssets.Font);

		State->GScene.ClearScene();

		// wait for last Frame to finish Rendering
		auto CL = GetCurrentCommandList(Engine->RenderSystem);

		for (size_t I = 0; I < 4; ++I) {
			WaitforGPU(&Engine->RenderSystem);
			IncrementRSIndex(Engine->RenderSystem);
		}

		//ShutDownUploadQueues(Engine->RenderSystem);

		ReleaseScene(&State->PScene, &Engine->Physics);
		ReleaseGameFramework(Engine, State);

		// Counters are at Max 3
		Free_DelayedReleaseResources(Engine->RenderSystem);
		//Free_DelayedReleaseResources(Engine->RenderSystem);
		//Free_DelayedReleaseResources(Engine->RenderSystem);

		Engine->BlockAllocator.free(State);
		
		FreeAllResourceFiles	(&Engine->Assets);
		FreeAllResources		(&Engine->Assets);

		ReleaseEngine			(Engine);
	}


	GAMESTATEAPI void PostPhysicsUpdate(GameFramework*)
	{

	}


	GAMESTATEAPI void PrePhysicsUpdate(GameFramework*)
	{

	}

	struct CodeExports
	{
		typedef GameFramework*	(*InitiateGameStateFN)	(EngineMemory* Engine);
		typedef void		(*UpdateFixedIMPL)		(EngineMemory* Engine,	double dt, GameFramework* _ptr);
		typedef void		(*UpdateIMPL)			(EngineMemory* Engine,	GameFramework* _ptr, double dt);
		typedef void		(*UpdateAnimationsFN)	(EngineMemory* RS,		iAllocator* TempMemory, double dt, GameFramework* _ptr);
		typedef void		(*UpdatePreDrawFN)		(EngineMemory* Engine,	iAllocator* TempMemory, double dt, GameFramework* _ptr);
		typedef void		(*DrawFN)				(EngineMemory* RS,		iAllocator* TempMemory,			   GameFramework* _ptr);
		typedef void		(*PostDrawFN)			(EngineMemory* Engine,	iAllocator* TempMemory, double dt, GameFramework* _ptr);
		typedef void		(*CleanUpFN)			(EngineMemory* Engine,	GameFramework* _ptr);
		typedef void		(*PostPhysicsUpdate)	(GameFramework*);
		typedef void		(*PrePhysicsUpdate)		(GameFramework*);

		InitiateGameStateFN		Init;
		InitiateEngineFN		InitEngine;
		UpdateIMPL				Update;
		UpdateFixedIMPL			UpdateFixed;
		UpdateAnimationsFN		UpdateAnimations;
		UpdatePreDrawFN			UpdatePreDraw;
		DrawFN					Draw;
		PostDrawFN				PostDraw;
		CleanUpFN				Cleanup;
	};

	GAMESTATEAPI void GetStateTable(CodeTable* out)
	{
		CodeExports* Table = reinterpret_cast<CodeExports*>(out);

		Table->Init				= &InitiateBaseGameState;
		Table->InitEngine		= &InitEngine;
		Table->Update			= &Update;
		Table->UpdateFixed		= &UpdateFixed;
		Table->UpdateAnimations	= &UpdateAnimations;
		Table->UpdatePreDraw	= &UpdatePreDraw;
		Table->Draw				= &Draw;
		Table->PostDraw			= &PostDraw;
		Table->Cleanup			= &Cleanup;
	}
}