/**********************************************************************

Copyright (c) 2019 Robert May

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

#include "DebugPanel.h"
#include "GameFramework.h"
#include "graphics.h"
#include "TextureUtilities.h"
#include "Logging.h"


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
//		TODO: Occlusion Culling
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
// Fix Resource Table leak


namespace FlexKit
{	/************************************************************************************************/


	bool SetDebugRenderMode	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void* USR);
	void EventsWrapper		(const Event& evt, void* _ptr);


	/************************************************************************************************/


	void HandleKeyEvents(const Event& in, GameFramework& framework)
	{
		switch (in.Action)
		{
		case Event::InputAction::Pressed:
		{
			switch (in.mData1.mKC[0])
			{
			case KC_ESC:
				framework.quit = true;
				break;
			case KC_E:
			{
			}	break;
			case KC_T:
				framework.core.RenderSystem.QueuePSOLoad(TILEDSHADING_SHADE);
				break;
			case KC_M:
                if (framework.MouseState.Enabled)
                    DisableMouseInput(&framework.MouseState);
				else
                    EnableMouseInput(&framework.MouseState, framework.ActiveWindow);

				break;
			case KC_TILDA:
			{
				FK_VLOG(Verbosity_9, "Console Key Pressed!");

				if (!framework.consoleActive) {
					framework.PushState<DebugPanel>(*framework.subStates.back());
					framework.consoleActive = true;
				}
			}	break;
			case KC_F1:
			{
				auto temp1 = framework.drawDebug;
				auto temp2 = framework.drawDebugStats;
				framework.drawDebug		    = !framework.drawDebug	    | (framework.drawDebugStats & !(temp1 & temp2));
				framework.drawDebugStats	= !framework.drawDebugStats | (framework.drawDebug & !(temp1 & temp2));
			}	break;
			case KC_F2:
			{
				framework.drawDebugStats = !framework.drawDebugStats;
			}	break;
			case KC_F3:
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


	void PushMessageToConsole(void* User, const char* Str, size_t StrLen)
	{
		GameFramework& framework = *reinterpret_cast<GameFramework*>(User);

		char* NewStr = (char*)framework.core.GetBlockMemory().malloc(StrLen + 1);
		memset((void*)NewStr, '\0', StrLen + 1);
		strncpy_s(NewStr, StrLen + 1, Str, StrLen);

		framework.console.PrintLine(NewStr, framework.core.GetBlockMemory());
	}


	/************************************************************************************************/


	GameFramework::GameFramework(EngineCore& IN_core) :
		console				{ DefaultAssets.Font, IN_core.RenderSystem, IN_core.GetBlockMemory() },
		core				{ IN_core	},
		fixStepAccumulator	{ 0.0		}

	{
		Initiate();
	}


	/************************************************************************************************/


	void GameFramework::Initiate()
	{
		SetDebugMemory			(core.GetDebugMemory());
		InitiateAssetTable	    (core.GetBlockMemory());
		InitiateGeometryTable	(core.GetBlockMemory());

		clearColor					= { 0.0f, 0.2f, 0.4f, 1.0f };
		quit						= false;
		physicsUpdateTimer			= 0.0f;

		drawDebug					= false;

#ifdef _DEBUG
		drawDebugStats			= true;
#else
		drawDebugStats			= true;
#endif

		ActiveScene					= nullptr;
		ActiveWindow				= &core.Window;

		drawPhysicsDebug			= false;

		stats.fps						= 0;
		stats.fpsCounter				= 0;
		stats.fpsT						= 0.0;
		stats.objectsDrawnLastFrame		= 0;
		rootNode						= GetZeroedNode();

		uint2	WindowRect	   = core.Window.WH;
		float	Aspect		   = (float)WindowRect[0] / (float)WindowRect[1];

		MouseState.NormalizedPos	= { 0.5f, 0.5f };
		MouseState.Position			= { float(WindowRect[0]/2), float(WindowRect[1] / 2) };


		EventNotifier<>::Subscriber sub;
		sub.Notify = &EventsWrapper;
		sub._ptr   = this;
		core.Window.Handler.Subscribe(sub);

		console.BindUIntVar("FPS",			&stats.fps);
		console.BindBoolVar("HUD",			&drawDebugStats);
		console.BindBoolVar("DrawDebug",	&drawDebug);
		console.BindBoolVar("FrameLock",    &core.FrameLock);

		console.AddFunction({ "SetRenderMode", &SetDebugRenderMode, this, 1, { ConsoleVariableType::CONSOLE_UINT }});

		AddLogCallback(&logMessagePipe, Verbosity_INFO);
	}


	/************************************************************************************************/


	void GameFramework::Update(UpdateDispatcher& dispatcher, double dT)
	{
		runningTime += dT;

		UpdateInput();
		UpdateMouseInput(&MouseState, &core.Window);

		if (!subStates.size()) {
			quit = true;
			return;
		}

		subStates.back()->Update(core, dispatcher, dT);

		core.End = quit;
	}


	/************************************************************************************************/


	void GameFramework::UpdateFixed(UpdateDispatcher& dispatcher, double dt)
	{
		//core.Physics.Simulate(dt);
	}


	/************************************************************************************************/


	void GameFramework::UpdatePreDraw(UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dT)
	{
		if (!subStates.size()) {
			quit = true;
			return;
		}

		if (drawDebug)
		{
			subStates.back()->Update(core, dispatcher, dT);
			dispatcher.Execute();
		}

		subStates.back()->Update(core, dispatcher, dT);
		dispatcher.Execute();

		if (stats.fpsT > 1.0)
		{
			stats.fps         = stats.fpsCounter;
			stats.fpsCounter = 0;
			stats.fpsT       = 0.0;
		}

		stats.fpsCounter++;
		stats.fpsT += dT;
	}


	/************************************************************************************************/


	void GameFramework::Draw(UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dT)
	{
		FK_LOG_9("Frame Draw Begin");

		FrameGraph&	frameGraph = TempMemory->allocate_aligned<FrameGraph>(core.RenderSystem, TempMemory);

		frameGraph.Resources.AddBackBuffer(core.Window.backBuffer);
		frameGraph.UpdateFrameGraph(core.RenderSystem, ActiveWindow, core.GetTempMemory());

		subStates.back()->Draw(core, dispatcher, dT, frameGraph);
		subStates.back()->PostDrawUpdate(core, dispatcher, dT, frameGraph);


		if(	ActiveWindow )
		{
			frameGraph.SubmitFrameGraph(dispatcher, core.RenderSystem, ActiveWindow, *TempMemory);
			Free_DelayedReleaseResources(core.RenderSystem);
		}


		FK_LOG_9("Frame Draw Begin");
	}


	/************************************************************************************************/


	void GameFramework::PostDraw(UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dt)
	{
		core.RenderSystem.PresentWindow(&core.Window);
	}


	/************************************************************************************************/


	void GameFramework::DrawFrame(double dT)
	{
		FK_LOG_9("Frame Begin");

		UpdateDispatcher dispatcher{ &core.Threads, core.GetTempMemory() };

		Update			(dispatcher, dT);
		UpdatePreDraw	(dispatcher, core.GetTempMemory(), dT);

		ProfileBegin(PROFILE_SUBMISSION);

		Draw			(dispatcher, core.GetTempMemory(), dT);

        dispatcher.Execute();

        ProfileEnd(PROFILE_SUBMISSION);

		PostDraw		(dispatcher, core.GetTempMemory(), dT);

		core.GetTempMemory().clear();

		fixStepAccumulator += dT;

		FK_LOG_9("Frame End");

		// Memory -----------------------------------------------------------------------------------
		//Engine->GetBlockMemory().LargeBlockAlloc.Collapse(); // Coalesce blocks
	}


	/************************************************************************************************/


	void GameFramework::Release()
	{
		core.Threads.SendShutdown();
		core.Threads.WaitForWorkersToComplete(core.Memory->TempAllocator);

		GetRenderSystem().WaitforGPU();

		while (subStates.size())
			PopState();

		console.Release();
		FlexKit::Release(DefaultAssets.Font, core.RenderSystem);


		FreeAllAssetFiles	();
		FreeAllAssets		();
	
		ReleaseGameFramework(core, *this);
	}


	/************************************************************************************************/


	bool GameFramework::DispatchEvent(const Event& evt)
	{
		if (subStates.size() != 0)
			return subStates.back()->EventHandler(evt);
		else
			return false;
	}


	/************************************************************************************************/


	void GameFramework::DrawDebugHUD(double dT, VertexBufferHandle textBuffer, FrameGraph& frameGraph)
	{
		uint32_t VRamUsage	= (uint32_t)(core.RenderSystem._GetVidMemUsage() / MEGABYTE);
		char* TempBuffer	= (char*)core.GetTempMemory().malloc(512);
		auto DrawTiming		= float(GetDuration(PROFILE_SUBMISSION)) / 1000.0f;

		sprintf_s(TempBuffer, 512, 
			"Current VRam Usage: %u MB\n"
			"FPS: %u\n"
			"Update/Draw Dispatch Time: %fms\n"
			"Objects Drawn: %u\n"
			"Build Date: " __DATE__ "\n",
			VRamUsage, 
			(uint32_t)stats.fps,
			DrawTiming, 
			(uint32_t)stats.objectsDrawnLastFrame);


        const uint2 WH          = ActiveWindow->WH;
        const float aspectRatio = GetWindowAspectRatio(core);

		PrintTextFormatting Format = PrintTextFormatting::DefaultParams();
        Format.Scale = float2{ 0.5f, 0.5f } * float2{ (float)WH[0], (float)WH[1] * aspectRatio } / float2{ 1080, 1920 };
        Format.Color = { 1, 1, 1, 1 };

		DrawSprite_Text(
				TempBuffer, 
				frameGraph, 
				*DefaultAssets.Font, 
				textBuffer, 
				core.Window.backBuffer, 
				core.GetTempMemory(), 
				Format);
	}


	/************************************************************************************************/


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
		subStates.back()->~FrameworkState();
		core.GetBlockMemory().free(subStates.back());
		subStates.pop_back();
	}


	/************************************************************************************************/


	void HandleMouseEvents(const Event& in, GameFramework& framework)
    {
	    switch (in.Action)
	    {
	    case Event::InputAction::Pressed:
	    {
		    if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			    framework.MouseState.LMB_Pressed = true;
		    }
	    }	break;
	    case Event::InputAction::Release:
	    {
		    if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			    framework.MouseState.LMB_Pressed = false;
		    }
	    }	break;
	    default:
		    break;
	    }
    }


	/************************************************************************************************/


	void EventsWrapper(const Event& evt, void* _ptr)
	{
		auto& framework = *reinterpret_cast<GameFramework*>(_ptr);

		if (!framework.DispatchEvent(evt))
		{
			switch (evt.InputSource)
			{
			case Event::Keyboard:
				HandleKeyEvents(evt, framework);
			case Event::Mouse:
				HandleMouseEvents(evt, framework);
				break;
			}
		}

	}


	/************************************************************************************************/


	bool LoadScene(EngineCore& core, GraphicScene& scene, const char* sceneName)
	{
		return LoadScene(core.RenderSystem, sceneName, scene, core.GetBlockMemory(), core.GetTempMemory());
	}


	/************************************************************************************************/


	bool LoadScene(EngineCore& core, GraphicScene& scene, GUID_t sceneID)
	{
		return LoadScene(core.RenderSystem, sceneID, scene, core.GetBlockMemory(), core.GetTempMemory());
	}


	/************************************************************************************************/


	void DrawMouseCursor(
		float2					CursorPos, 
		float2					CursorSize, 
		VertexBufferHandle		vertexBuffer, 
		ConstantBufferHandle	constantBuffer,
		ResourceHandle			renderTarget,
		iAllocator*				tempMemory,
		FrameGraph*				frameGraph)
	{
        FK_ASSERT(0);
        /*
		DrawShapes(
			DRAW_PSO,
			*frameGraph,
			vertexBuffer,
			constantBuffer,
			renderTarget,
			tempMemory,
			RectangleShape(
				CursorPos,
				CursorSize,
				{Grey(1.0f), 1.0f}));
         */
	}


	/************************************************************************************************/


	void ReleaseGameFramework(EngineCore& core, GameFramework& framework)
	{
		ClearLogCallbacks();

		auto    RItr        = framework.subStates.rbegin();
		auto    REnd        = framework.subStates.rend();
		auto&   allocator   = core.GetBlockMemory();
		while (RItr != REnd)
		{
			(*RItr)->~FrameworkState();
			allocator.free(*RItr);

			RItr++;
		}


		ReleaseGeometryTable();
		ReleaseAssetTable();

		//TODO
		//Release(State->DefaultAssets.Font);
		Release(framework.DefaultAssets.Terrain);

		core.Threads.SendShutdown();
		core.Threads.WaitForWorkersToComplete(core.Memory->TempAllocator);
	}


	/************************************************************************************************/


	inline void PushSubState(GameFramework& _ptr, FrameworkState& SS)
	{
		_ptr.subStates.push_back(&SS);
	}


	/************************************************************************************************/


	void PopSubState(GameFramework& framework)
	{
		if (!framework.subStates.size())
			return;

		FrameworkState* State = framework.subStates.back();
		State->~FrameworkState();

		framework.core.GetBlockMemory().free(State);
		framework.subStates.pop_back();
	}


	/************************************************************************************************/


	bool SetDebugRenderMode(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void* USR)
	{
		GameFramework* framework = (GameFramework*)USR;
		if (ArguementCount == 1 && Arguments->Type == ConsoleVariableType::STACK_STRING) {
			size_t Mode = 0;

			const char* VariableIdentifier = (const char*)Arguments->Data_ptr;
			for (auto Var : C->variables)
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


}	/************************************************************************************************/
