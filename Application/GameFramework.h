#ifndef GameFramework_H
#define GameFramework_H

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

#ifdef COMPILE_GAMESTATE
#define GAMESTATEAPI __declspec(dllexport)
#else
#define GAMESTATEAPI __declspec(dllimport)
#endif

#include "..\Application\GameMemory.h"
#include "..\Application\GameUtilities.h"

#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\Resources.h"

#include "..\graphicsutilities\Graphics.h"
#include "..\graphicsutilities\TerrainRendering.h"
#include "..\graphicsutilities\SSReflections.h"
#include "..\graphicsutilities\ImmediateRendering.h"

#include "Console.h"

#include <iostream>

namespace FlexKit
{
	struct MouseHandler
	{
	};

	class FrameworkState;

	typedef bool (*FN_UPDATE_SUBSTATE)  (FrameworkState* StateMemory, EngineMemory*, double DT); // Return True to allow Cascading updates down State Stack
	typedef void (*FN_RELEASE_SUBSTATE) (FrameworkState* StateMemory);

	typedef bool(*FN_EVENT_HANDLER) (FrameworkState* StateMemory, Event);


	/************************************************************************************************/


	struct GameFramework
	{
		struct {
			FontAsset*			Font;
			Texture2D			Terrain;
		}DefaultAssets;

		MouseInputState		MouseState;
		Landscape			Landscape;

		Camera					DebugCamera;
		Camera					DefaultCamera;
		GraphicScene*			ActiveScene;
		PhysicsComponentSystem*	ActivePhysicsScene;
		Camera*					ActiveCamera;
		RenderWindow*			ActiveWindow;

		float4				ClearColor;
		EDEFERREDPASSMODE	DP_DrawMode;

		Console				Console;
		ImmediateRender		Immediate;
		EngineMemory*		Engine;


		static_vector<MouseHandler>		MouseHandlers;
		static_vector<FrameworkState*>	SubStates;


		double				PhysicsUpdateTimer;
		bool				ConsoleActive;
		bool				OcclusionCulling;
		bool				ScreenSpaceReflections;
		bool				DrawTerrain;
		bool				DrawTerrainDebug;
		bool				DrawDebug;
		bool				DrawDebugStats;
		bool				DrawPhysicsDebug;
		bool				Quit;

		size_t				TerrainSplits;

		double TimeRunning;

		struct FrameStats
		{
			double T;
			double Fps_T;
			size_t FPS;
			size_t FPS_Counter;
			size_t ObjectsDrawnLastFrame;
		}Stats;
	};


	/************************************************************************************************/


	class FrameworkState
	{
	public:
		virtual ~FrameworkState() {}

		virtual bool  Update(EngineMemory* Engine, double dT)			{ return true; };
		virtual bool  DebugDraw(EngineMemory* Engine, double dT)		{ return true; };
		virtual bool  PreDrawUpdate(EngineMemory* Engine, double dT)	{ return true; };
		virtual bool  PostDrawUpdate(EngineMemory* Engine, double dT)	{ return true; };

		virtual bool  EventHandler(Event evt) { return true; };

		GameFramework*	Framework;
	};


	/************************************************************************************************/


	bool			LoadScene		 (EngineMemory* Engine, GraphicScene* Scene, const char* SceneName);
	void			DrawMouseCursor	 (EngineMemory* Engine, GameFramework* State, float2 CursorPos, float2 CursorSize);

	void			PushSubState	 (GameFramework* _ptr, FrameworkState* SS);
	void			PopSubState		 (GameFramework* _ptr);

	void			UpdateGameFramework	 (EngineMemory* Engine, GameFramework* _ptr, double dT);
	void			PreDrawGameFramework (EngineMemory* Engine, GameFramework* _ptr, double dT);
	void			ReleaseGameFramework (EngineMemory* Engine, GameFramework* _ptr );


	/************************************************************************************************/


	GameFramework*	InitiateFramework	( EngineMemory* Engine );
	void			Update				( EngineMemory* Engine, GameFramework* Framework, double dT );
	void			UpdateFixed			( EngineMemory* Engine, double dt, GameFramework* State );
	void			UpdateAnimations	( EngineMemory* Engine, iAllocator* TempMemory, double dt, GameFramework* _ptr );
	void			UpdatePreDraw		( EngineMemory* Engine, iAllocator* TempMemory, double dt, GameFramework* Framework );
	void			Draw				( EngineMemory* Engine, iAllocator* TempMemory, GameFramework* Framework );
	void			PostDraw			( EngineMemory* Engine, iAllocator* TempMemory, double dt, GameFramework* State );
	void			Cleanup				( EngineMemory* Engine, GameFramework* Framework );
	void			PostPhysicsUpdate	( GameFramework* );
	void			PrePhysicsUpdate	( GameFramework* );


}	/************************************************************************************************/
#endif