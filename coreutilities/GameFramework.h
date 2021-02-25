#ifndef GameFramework_H
#define GameFramework_H

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

#ifdef COMPILE_GAMESTATE
#define GAMESTATEAPI __declspec(dllexport)
#else
#define GAMESTATEAPI __declspec(dllimport)
#endif

#include "Assets.h"
#include "Components.h"
#include "Console.h"
#include "EngineCore.h"
#include "Events.h"
#include "FrameGraph.h"
#include "Graphics.h"
#include "GraphicScene.h"
#include "GraphicsComponents.h"
#include "Logging.h"
#include "SSReflections.h"
#include "TerrainRendering.h"

#include <iostream>

namespace FlexKit
{
	/************************************************************************************************/


	struct MouseHandler
	{
	};

	class FrameworkState;


    /************************************************************************************************/


    class FixedUpdate
    {
    public:
        FixedUpdate(double IN_updateRate) : updateRate{ 1.0 / IN_updateRate } {}

        template<typename TY_FN> requires std::invocable<TY_FN, double>
            void operator() (double dT, TY_FN FN)
            {
                T += dT;

                while (T >= updateRate) {
                    FN(updateRate);

                    T -= updateRate;
                }
            }

            double GetdT() const { return updateRate; }

            double updateRate = 0.0;
            double T = 0.0;
    };


	/************************************************************************************************/


	void PushMessageToConsole(void* User, const char* Str, size_t StrLen);


	class GameFramework
	{
	public:
		GameFramework(EngineCore& core);

		GameFramework				(const GameFramework&) = delete;
		GameFramework& operator =	(const GameFramework&) = delete;

		void Initiate();

		UpdateTask* Update      (UpdateDispatcher& dispatcher, double dT);
        UpdateTask* Draw	    (UpdateTask* update, UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dT);

		void        PostDraw    (iAllocator* TempMemory, double dT);

		void Release		();

		void DrawFrame(double dT);

		bool DispatchEvent(const Event& evt);

		void DrawDebugHUD(double dT, VertexBufferHandle TextBuffer, ResourceHandle renderTarget, FrameGraph& Graph);

		void PostPhysicsUpdate	();
		void PrePhysicsUpdate	();

		void PopState();

		RenderSystem&	GetRenderSystem()	{ return core.RenderSystem; }

		template<typename TY_INITIALSTATE, typename ... TY_ARGS>
		TY_INITIALSTATE& PushState(TY_ARGS&& ... args)
		{
			FK_LOG_9("Pushing New State");

			auto& State = core.GetBlockMemory().allocate_aligned<TY_INITIALSTATE>(*this, std::forward<TY_ARGS>(args)...);

			subStates.push_back(&State);

            pushOccured = true;

			return State;
		}

        bool    PushPopDetected() const {
            return pushOccured;
        }

		LogCallback				logMessagePipe = {
			"INFO",
			this,
			PushMessageToConsole
		};

		EngineCore&				core;
		NodeHandle				rootNode;

		static_vector<MouseHandler>		mouseHandlers;
		static_vector<FrameworkState*>	subStates;
        static_vector<FrameworkState*>	delayedFrees;

		double	physicsUpdateTimer;
		bool	consoleActive;
		bool	drawDebugStats;
		bool	drawPhysicsDebug;
		bool	quit;
        bool    pushOccured = false;

		double runningTime			= 0.0;
		double fixStepAccumulator	= 0.0;
		double fixedTimeStep		= 1.0 / 60.0;

        struct TimePoint
        {
            double T;
            double duration;
        };

		struct FrameStats
		{
			double t;
			double fpsT;
			size_t fps;
			size_t fpsCounter;
			size_t objectsDrawnLastFrame;

            CircularBuffer<TimePoint, 120>  frameTimes;
            CircularBuffer<TimePoint, 120>  shadingTimes;
            CircularBuffer<TimePoint, 120>  feedbackTimes;
            CircularBuffer<TimePoint, 120>  presentTimes;
		}stats;

		struct {
			SpriteFontAsset*	Font;
			Texture2D			Terrain;
		}DefaultAssets =
		{
			LoadFontAsset(
				"assets\\fonts\\",
				"fontTest.fnt",
				core.RenderSystem,
				core.GetTempMemory(),
				core.GetBlockMemory())
		};

		Console					console;
	};


    /************************************************************************************************/


	class FrameworkState
	{
	public:
		FrameworkState(const FrameworkState& in)				= delete;

		FrameworkState& operator = (const FrameworkState& in)	= delete;

		virtual ~FrameworkState() {}

		virtual UpdateTask* Update			(                       EngineCore&, UpdateDispatcher&, double dT) { return nullptr; }
		virtual UpdateTask* Draw			(UpdateTask* update,    EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph) { return nullptr;}

		virtual void        PostDrawUpdate	(EngineCore&, double dT) {}

		virtual bool EventHandler	(Event evt) { return false;}

		GameFramework&	framework;
	protected:

		FrameworkState(GameFramework& in_framework) : framework(in_framework) {}

		RenderSystem*	GetRenderSystem() { return framework.GetRenderSystem(); }
	};


	/************************************************************************************************/


	bool LoadScene		 (EngineCore& Engine, GraphicScene& Scene, const char* SceneName);
	bool LoadScene		 (EngineCore& Engine, GraphicScene& Scene, GUID_t SceneID);
 
	void PushSubState	 (GameFramework& _ptr, FrameworkState& SS);
	void PopSubState	 (GameFramework& _ptr);
 
	void UpdateGameFramework  (EngineCore& Engine, GameFramework& _ptr, double dT);
	void PreDrawGameFramework (EngineCore& Engine, GameFramework& _ptr, double dT);
	void ReleaseGameFramework (EngineCore& Engine, GameFramework& _ptr);

	void InitiateFramework	(EngineCore& Engine, GameFramework& framework);


	void DrawMouseCursor(
		float2					CursorPos,
		float2					CursorSize,
		VertexBufferHandle		vertexBuffer,
		ConstantBufferHandle	constantBuffer,
		ResourceHandle			renderTarget,
		iAllocator*				tempMemory,
		FrameGraph*				frameGraph);


    void EventsWrapper(const Event& evt, void* _ptr);


}	/************************************************************************************************/
#endif
