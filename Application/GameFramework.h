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

#include "..\graphicsutilities\FrameGraph.h"
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


	/************************************************************************************************/


	class GameFramework
	{
	public:
		void Update				(double dT);
		void UpdateFixed		(double dT);
		void UpdateAnimations	(iAllocator* TempMemory, double dT);
		void UpdatePreDraw		(iAllocator* TempMemory, double dT);
		void Draw				(iAllocator* TempMemory);
		void PostDraw			(iAllocator* TempMemory, double dT);
		void Cleanup			();

		void PostPhysicsUpdate	();
		void PrePhysicsUpdate	();

		struct {
			FontAsset*			Font;
			Texture2D			Terrain;
		}DefaultAssets;

		MouseInputState			MouseState;

		GraphicScene*			ActiveScene;
		PhysicsComponentSystem*	ActivePhysicsScene;
		RenderWindow*			ActiveWindow;

		float4					ClearColor;
		EDEFERREDPASSMODE		DP_DrawMode;

		Console					Console;
		EngineCore*				Core;


		static_vector<MouseHandler>		MouseHandlers;
		static_vector<FrameworkState*>	SubStates;

		double	PhysicsUpdateTimer;
		bool	ConsoleActive;
		bool	DrawDebug;
		bool	DrawDebugStats;
		bool	DrawPhysicsDebug;
		bool	Quit;

		size_t	TerrainSplits;

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
		FrameworkState(const FrameworkState& in) = delete;

		FrameworkState& operator = (const FrameworkState& in) = delete;

		virtual ~FrameworkState() {}

		virtual bool Update			(EngineCore* Engine, double dT) { return true; };
		virtual bool DebugDraw		(EngineCore* Engine, double dT) { return true; };
		virtual bool PreDrawUpdate	(EngineCore* Engine, double dT) { return true; };
		virtual bool Draw			(EngineCore* Engine, double dT, FrameGraph& Graph) { return true; };
		virtual bool PostDrawUpdate	(EngineCore* Engine, double dT) { return true; };

		virtual bool EventHandler	(Event evt) { return true; };

		GameFramework*	Framework;

	protected: 		
		FrameworkState(GameFramework* framework) : Framework(framework) {}
	};


	/************************************************************************************************/


	bool LoadScene		 (EngineCore* Engine, GraphicScene* Scene, const char* SceneName);
	bool LoadScene		 (EngineCore* Engine, GraphicScene* Scene, GUID_t SceneID);
	void DrawMouseCursor (EngineCore* Engine, GameFramework* State, float2 CursorPos, float2 CursorSize);
 
	void PushSubState	 (GameFramework* _ptr, FrameworkState* SS);
	void PopSubState	 (GameFramework* _ptr);
 
	void UpdateGameFramework  (EngineCore* Engine, GameFramework* _ptr, double dT);
	void PreDrawGameFramework (EngineCore* Engine, GameFramework* _ptr, double dT);
	void ReleaseGameFramework (EngineCore* Engine, GameFramework* _ptr);


	/************************************************************************************************/


	void InitiateFramework	(EngineCore* Engine, GameFramework& Framework);


}	/************************************************************************************************/
#endif