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
#include "..\coreutilities\Logging.h"

// Todo List
//	Gameplay:
//		Entity Model
//	Sound:
//	
//	Generic:
//		(DONE) Scene Loading
//		Config loading system?
//
//	Graphics:
//		(PARTLY) Frame Graph Rendering
//				(Done) Auto Barrier Handling
//				TODO: (In Progress) Replacement Draw Utility Functions that use the frame graph rather then direct submission
//
//		(DONE) Basic Gui rendering methods (Draw Rect, etc)
//		(PARTLY) Multi-threaded Texture Uploads
//		TODO: (Partly)	Terrain Rendering
//				(DONE) Geometry generation
//				(DONE) CULLING
//				Texture Splatting
//				Normal Generation
//		(DONE) Occlusion Culling
//		(TODO:)(Partly Done)Animation State Machine
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
				_ptr->Core->RenderSystem.QueuePSOLoad(EPIPELINESTATES::DRAW_LINE_PSO);
				_ptr->Core->RenderSystem.QueuePSOLoad(EPIPELINESTATES::TERRAIN_CULL_PSO);
				_ptr->Core->RenderSystem.QueuePSOLoad(EPIPELINESTATES::TERRAIN_DRAW_PSO);
				_ptr->Core->RenderSystem.QueuePSOLoad(EPIPELINESTATES::TERRAIN_DRAW_PSO_DEBUG);
				_ptr->Core->RenderSystem.QueuePSOLoad(EPIPELINESTATES::TERRAIN_DRAW_WIRE_PSO);
				break;
			case KC_E:
			{
			}	break;
			case KC_T:
				_ptr->Core->RenderSystem.QueuePSOLoad(EPIPELINESTATES::TILEDSHADING_SHADE);
				break;
			case KC_M:
				_ptr->MouseState.Enabled = !_ptr->MouseState.Enabled;
				break;
			case KC_TILDA:
			{
				FK_LOG_INFO("Console Key Pressed!");

				if (!_ptr->ConsoleActive) {
					PushSubState(_ptr, &_ptr->Core->GetBlockMemory().allocate<ConsoleSubState>(_ptr));
					_ptr->ConsoleActive = true;
				}
			}	break;
			case KC_F1:
			{
				auto Temp1 = _ptr->DrawDebug;
				auto Temp2 = _ptr->DrawDebugStats;
				_ptr->DrawDebug			= !_ptr->DrawDebug		| (_ptr->DrawDebugStats	& !(Temp1 & Temp2));
				_ptr->DrawDebugStats	= !_ptr->DrawDebugStats | (_ptr->DrawDebug		& !(Temp1 & Temp2));
			}	break;
			case KC_F2:
			{
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


	void GameFramework::Update(double dT)
	{
		TimeRunning += dT;


		UpdateMouseInput(&MouseState, &Core->Window);

		if (!SubStates.size()) {
			Quit = true;
			return;
		}

		UpdateDispatcher Dispatcher{ Core->GetTempMemory() };

		auto RItr = SubStates.rbegin();
		auto REnd = SubStates.rend();

		while (RItr != REnd)
		{

			auto State = *RItr;
			if (!State->Update(Core, Dispatcher, dT))
				break;

			RItr++;
		}

		Dispatcher.Execute();
		Core->End = Quit;
	}


	void GameFramework::UpdateFixed(double dt)
	{
		UpdateMouseInput(&MouseState, &Core->Window);
	}


	void GameFramework::UpdatePreDraw(iAllocator* TempMemory, double dT)
	{
		if (!SubStates.size()) {
			Quit = true;
			return;
		}

		{
			UpdateDispatcher Dispatcher{ Core->GetTempMemory() };

			if (DrawDebug) {
				auto RItr = SubStates.rbegin();
				auto REnd = SubStates.rend();
				while (RItr != REnd)
				{

					auto State = *RItr;
					if (!State->DebugDraw(Core, Dispatcher, dT))
						break;

					RItr++;
				}
			}

			Dispatcher.Execute();
		}

		{
			UpdateDispatcher Dispatcher{ Core->GetTempMemory() };

			auto RItr = SubStates.rbegin();
			auto REnd = SubStates.rend();
			while (RItr != REnd)
			{
				auto State = *RItr;
				if (!State->PreDrawUpdate(Core, Dispatcher, dT))
					break;

				RItr++;
			}

			Dispatcher.Execute();
		}

		if (Stats.Fps_T > 1.0)
		{
			Stats.FPS         = Stats.FPS_Counter;
			Stats.FPS_Counter = 0;
			Stats.Fps_T       = 0.0;
		}

		Stats.FPS_Counter++;
		Stats.Fps_T += dT;
	}


	void GameFramework::Draw(iAllocator* TempMemory)
	{
		FrameGraph		FrameGraph(Core->RenderSystem, TempMemory);

		// Add in Base Resources
		FrameGraph.Resources.AddRenderTarget(Core->Window.GetBackBuffer());
		FrameGraph.UpdateFrameGraph(Core->RenderSystem, ActiveWindow, Core->GetTempMemory());

		UpdateDispatcher Dispatcher{ Core->GetTempMemory() };

		{
			auto Itr = SubStates.begin();
			auto End = SubStates.end();

			while (Itr != End)
			{
				auto Framework = *Itr;		
				if (!Framework->Draw(Core, Dispatcher, 0, FrameGraph))
					break;

				Itr++;
			}
		}

		{
			auto Itr = SubStates.rbegin();
			auto End = SubStates.rend();

			while (Itr != End)
			{
				auto Framework = *Itr;
				if (!Framework->PostDrawUpdate(Core, Dispatcher, 0, FrameGraph))
					break;

				Itr++;
			}
		}

		Dispatcher.Execute();

		ProfileBegin(PROFILE_SUBMISSION);

		if(	ActiveWindow )
		{
			FrameGraph.SubmitFrameGraph(Core->RenderSystem, ActiveWindow);

			Free_DelayedReleaseResources(Core->RenderSystem);
		}

		ProfileEnd(PROFILE_SUBMISSION);
	}


	void GameFramework::PostDraw(iAllocator* TempMemory, double dt)
	{
		Core->RenderSystem.PresentWindow(&Core->Window);

		for (;PopCount && SubStates.size(); --PopCount)
			PopSubState(this);
	}


	void GameFramework::Cleanup()
	{
		Core->RenderSystem.ShutDownUploadQueues();

		ReleaseConsole(&Console);
		//Release(DefaultAssets.Font);

		// wait for last Frame to finish Rendering
		auto CL = Core->RenderSystem._GetCurrentCommandList();

		for (size_t I = 0; I < 4; ++I) 
		{
			Core->RenderSystem.WaitforGPU();
			Core->RenderSystem._IncrementRSIndex();
		}

		ReleaseGameFramework(Core, this);

		// Counters are at Max 3
		Free_DelayedReleaseResources(Core->RenderSystem);
		Free_DelayedReleaseResources(Core->RenderSystem);
		Free_DelayedReleaseResources(Core->RenderSystem);

		FreeAllResourceFiles	();
		FreeAllResources		();

		Core->GetBlockMemory().release_allocation(*Core);

		DEBUGBLOCK(PrintBlockStatus(&Core->GetBlockMemory()));
	}


	bool GameFramework::DispatchEvent(const Event& evt)
	{
		auto itr = SubStates.rbegin();
		while (itr != SubStates.rend())
		{
			if (!(*itr)->EventHandler(evt))
				return false;
			itr++;
		}
		return true;
	}


	void GameFramework::DrawDebugHUD(double dT, VertexBufferHandle TextBuffer, FrameGraph& Graph)
	{
		uint32_t VRamUsage	= GetVidMemUsage(Core->RenderSystem) / MEGABYTE;
		char* TempBuffer	= (char*)Core->GetTempMemory().malloc(512);
		auto DrawTiming		= float(GetDuration(PROFILE_SUBMISSION)) / 1000.0f;

		sprintf_s(TempBuffer, 512, 
			"Current VRam Usage: %u MB\n"
			"FPS: %u\n"
			"Draw Time: %fms\n"
			"Objects Drawn: %u\n"
			"Build Date: " __DATE__ "\n",
			VRamUsage, 
			(uint32_t)Stats.FPS, DrawTiming, 
			(uint32_t)Stats.ObjectsDrawnLastFrame);


		PrintTextFormatting Format = PrintTextFormatting::DefaultParams();

		DrawSprite_Text(
				TempBuffer, 
				Graph, 
				*DefaultAssets.Font, 
				TextBuffer, 
				GetCurrentBackBuffer(&Core->Window), 
				Core->GetTempMemory(), 
				Format);
	}

	void GameFramework::PostPhysicsUpdate()
	{

	}


	/************************************************************************************************/


	void GameFramework::PrePhysicsUpdate()
	{

	}


	/************************************************************************************************/


	void GameFramework::PopState()
	{
		++PopCount;
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

		if (base->DispatchEvent(evt))
		{
			switch (evt.InputSource)
			{
			case Event::Keyboard:
				HandleKeyEvents(evt, base);
			case Event::Mouse:
				HandleMouseEvents(evt, reinterpret_cast<GameFramework*>(_ptr));
				break;
			}
		}

	}


	/************************************************************************************************/


	bool LoadScene(EngineCore* Engine, GraphicScene* Scene, const char* SceneName)
	{
		return LoadScene(Engine->RenderSystem, SceneName, Scene, Engine->GetTempMemory());
	}


	bool LoadScene(EngineCore* Engine, GraphicScene* Scene, GUID_t SceneID)
	{
		return LoadScene(Engine->RenderSystem, SceneID, Scene, Engine->GetTempMemory());
	}


	/************************************************************************************************/


	void DrawMouseCursor(EngineCore* Engine, GameFramework* State, float2 CursorPos, float2 CursorSize)
	{
		using FlexKit::Conversion::Vect2TOfloat2;


		FK_ASSERT(0);

		//FlexKit::Draw_RECT Cursor;
		//Cursor.BLeft  = CursorPos - CursorSize		* float2(0, 1);
		//Cursor.TRight = Cursor.BLeft + CursorSize;
		//Cursor.Color  = float4(1, 1, 1, 1);

		//PushRect(State->Immediate, Cursor);
	}


	/************************************************************************************************/


	void ReleaseGameFramework(EngineCore* Core, GameFramework* State)
	{
		auto RItr = State->SubStates.rbegin();
		auto REnd = State->SubStates.rend();
		while (RItr != REnd)
		{
			(*RItr)->~FrameworkState();
			RItr++;
		}


		ReleaseGeometryTable();
		ReleaseResourceTable();

		//Release(State->DefaultAssets.Font);
		Release(State->DefaultAssets.Terrain);
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
		}
		return false;
	}


	/************************************************************************************************/


	void InitiateFramework(EngineCore* Core, GameFramework& Framework)
	{
		SetDebugMemory(Core->GetDebugMemory());
		InitiateResourceTable(Core->GetBlockMemory());
		InitiateGeometryTable(Core->GetBlockMemory());
		InitiateCameraTable(Core->GetBlockMemory());

		AddResourceFile("assets\\assets.gameres");
		AddResourceFile("assets\\ResourceFile.gameres");
		AddResourceFile("assets\\ShaderBallTestScene.gameres");

		Framework.ClearColor				= { 0.0f, 0.2f, 0.4f, 1.0f };
		Framework.Quit						= false;
		Framework.PhysicsUpdateTimer		= 0.0f;
		Framework.TerrainSplits				= 12;
		Framework.Core						= Core;

		Framework.DrawDebug					= false;

#ifdef _DEBUG
		Framework.DrawDebugStats			= true;
#else
		Framework.DrawDebugStats			= false;
#endif

		//Framework.ActivePhysicsScene		= nullptr;
		Framework.ActiveScene				= nullptr;
		Framework.ActiveWindow				= &Core->Window;

		Framework.DrawPhysicsDebug			= false;

		Framework.Stats.FPS						= 0;
		Framework.Stats.FPS_Counter				= 0;
		Framework.Stats.Fps_T					= 0.0;
		Framework.Stats.ObjectsDrawnLastFrame	= 0;
		Framework.RootNode						= GetZeroedNode();

		{
			uint2	WindowRect	   = Core->Window.WH;
			float	Aspect		   = (float)WindowRect[0] / (float)WindowRect[1];

			Framework.MouseState.NormalizedPos	= { 0.5f, 0.5f };
			Framework.MouseState.Position		= { float(WindowRect[0]/2), float(WindowRect[1] / 2) };
		}

		/*
		{
			TextureBuffer TempBuffer;
			LoadBMP("assets\\textures\\TestMap.bmp", Core->GetTempMemory(), &TempBuffer);
			Texture2D	HeightMap = LoadTexture(&TempBuffer, Core->RenderSystem, Core->GetTempMemory());

			Framework.DefaultAssets.Terrain = HeightMap;
		}
		*/

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify = &EventsWrapper;
		sub._ptr   = &Framework;
		Core->Window.Handler.Subscribe(sub);

		Framework.DefaultAssets.Font = LoadFontAsset	("assets\\fonts\\", "fontTest.fnt", Core->RenderSystem, Core->GetTempMemory(), Core->GetBlockMemory());
		
		InitateConsole(&Framework.Console, Framework.DefaultAssets.Font, Core);
		BindUIntVar(&Framework.Console, "TerrainSplits",	&Framework.TerrainSplits);
		BindUIntVar(&Framework.Console, "FPS",				&Framework.Stats.FPS);
		BindBoolVar(&Framework.Console, "HUD",				&Framework.DrawDebugStats);
		BindBoolVar(&Framework.Console, "DrawDebug",		&Framework.DrawDebug);
		//BindBoolVar(&Framework.Console, "DrawPhysicsDebug",	&Framework.DrawPhysicsDebug);
		BindBoolVar(&Framework.Console, "FrameLock",		&Core->FrameLock);

		AddConsoleFunction(&Framework.Console, { "SetRenderMode", &SetDebugRenderMode, &Framework, 1, { ConsoleVariableType::CONSOLE_UINT }});

		Core->RenderSystem.UploadResources();// Uploads fresh Resources to GPU
	}


}	/************************************************************************************************/