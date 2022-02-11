#pragma once



/**********************************************************************

Copyright (c) 2021 Robert May

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

#include "AnimationComponents.h"
#include "Application.h"
#include "GameFramework.h"
#include "EngineCore.h"
#include "Materials.h"
#include "WorldRender.h"
#include "TextRendering.h"
#include "defaultpipelinestates.h"
#include "Win32Graphics.h"
#include "TextureStreamingUtilities.h"
#include "RayTracingUtilities.h"
#include "DebugUI.h"

#include <angelscript.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>
#include <angelscript/scriptbuilder/scriptbuilder.h>
#include <imgui.h>
#include <fmod.hpp>

using FlexKit::WorldRender;
using FlexKit::FKApplication;


/************************************************************************************************/


#pragma comment(lib, "fmod_vc.lib")

using namespace FlexKit;

typedef uint32_t SoundHandle;


class SoundSystem
{
public:
	SoundSystem(ThreadManager& IN_Threads, iAllocator* memory) :
		threads	{	IN_Threads	}
	{
		result			= FMOD::System_Create(&system);
		auto initres	= system->init(32, FMOD_INIT_NORMAL, nullptr);

		auto& Work = CreateWorkItem(
			[this](iAllocator&) {
			//auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &sound1);
			if(sound1)
				system->playSound(sound1, nullptr, false, &channel);
		}, memory, memory);

        Work._debugID = "FMOD: Play Sound";
		threads.AddWork(Work);
	}


	~SoundSystem()
	{
		sound1->release();
		system->release();
		system = nullptr;
	}


	SoundHandle LoadSoundFromDisk(const char* str)
	{
		return -1;
	}


	void Update(iAllocator* memory)
	{
		auto& Work = CreateWorkItem(
			[this](iAllocator&) {
				auto result = system->update();
			}, 
			memory);

        Work._debugID = "FMOD: Update Sound";
		threads.AddWork(Work);
	}


	FlexKit::Vector<FMOD::Sound*> Sounds;

	ThreadManager&	threads;

	FMOD::System*	system	= nullptr;
	FMOD::Sound*	sound1	= nullptr;
	FMOD::Sound*	sound2	= nullptr;
	FMOD::Sound*	sound3	= nullptr;
	FMOD::Channel*	channel = nullptr;
	FMOD_RESULT       result;
	unsigned int      version;
};


/************************************************************************************************/


inline FlexKit::UpdateTask* QueueSoundUpdate(FlexKit::UpdateDispatcher& Dispatcher, SoundSystem* Sounds, iAllocator* allocator)
{
	struct SoundUpdateData
	{
		SoundSystem* Sounds;
	};

	SoundSystem* Sounds_ptr = nullptr;
	auto& SoundUpdate = Dispatcher.Add<SoundUpdateData>(
		[&](auto& Builder, SoundUpdateData& Data)
		{
			Data.Sounds = Sounds;
			Builder.SetDebugString("UpdateSound");
		},
		[allocator](auto& Data, iAllocator& threadAllocator)
		{
            ProfileFunction();

			FK_LOG_9("Sound Update");
			Data.Sounds->Update(allocator);
		});

	return &SoundUpdate;
}


/************************************************************************************************/



class BaseState : public FrameworkState
{
public:
	BaseState(	
		GameFramework& IN_Framework,
		FKApplication& IN_App,
        uint2 WH    = { 1920, 1080 }) :
			App				    { IN_App },
			FrameworkState	    { IN_Framework },
			depthBuffer		    { IN_Framework.core.RenderSystem, renderWindow.GetWH() },

			vertexBuffer	    { IN_Framework.core.RenderSystem.CreateVertexBuffer(MEGABYTE * 1, false) },
			constantBuffer	    { IN_Framework.core.RenderSystem.CreateConstantBuffer(MEGABYTE * 128, false) },
			asEngine		    { asCreateScriptEngine() },
			streamingEngine	    { IN_Framework.core.RenderSystem, IN_Framework.core.GetBlockMemory() },
            sounds              { IN_Framework.core.Threads,      IN_Framework.core.GetBlockMemory() },

            renderWindow{ std::get<0>(CreateWin32RenderWindow(IN_Framework.GetRenderSystem(), DefaultWindowDesc({ WH }) )) },

			render	{	IN_Framework.core.RenderSystem,
						streamingEngine,
                        IN_Framework.core.GetBlockMemory(),
					},

            rtEngine{ IN_Framework.core.RenderSystem.RTAvailable() ?
                (iRayTracer&)IN_Framework.core.GetBlockMemory().allocate<RTX_RayTracer>(IN_Framework.core.RenderSystem, IN_Framework.core.GetBlockMemory()) :
                (iRayTracer&)IN_Framework.core.GetBlockMemory().allocate<NullRayTracer>() },

            // Components
            animations          { framework.core.GetBlockMemory() },
            IK                  { framework.core.GetBlockMemory() },
            IKTargets           { framework.core.GetBlockMemory() },
			cameras		        { framework.core.GetBlockMemory() },
			ids			        { framework.core.GetBlockMemory() },
			brushes 	        { framework.core.GetBlockMemory(), IN_Framework.GetRenderSystem() },
            materials           { IN_Framework.core.RenderSystem, streamingEngine, framework.core.GetBlockMemory() },
			visables	        { framework.core.GetBlockMemory() },
			pointLights	        { framework.core.GetBlockMemory() },
            skeletonComponent   { framework.core.GetBlockMemory() },
            gbuffer             { renderWindow.GetWH(), framework.core.RenderSystem },
            shadowCasters       { IN_Framework.core.GetBlockMemory() },
            physics             { IN_Framework.core.Threads, IN_Framework.core.GetBlockMemory() },
            rigidBodies         { physics },
            staticBodies        { physics },
            characterControllers{ physics, framework.core.GetBlockMemory() },
            orbitCameras        { framework.core.GetBlockMemory() },
            debugUI             { framework.core.RenderSystem, framework.core.GetBlockMemory() }
	{
		auto& RS = *IN_Framework.GetRenderSystem();
		RS.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS6CBVs4SRVs, LoadSpriteTextPSO		        });
		RS.RegisterPSOLoader(DRAW_PSO,					{ &RS.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO			});
		RS.RegisterPSOLoader(DRAW_TEXTURED_PSO,		    { &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStatePSO		});
		//RS.RegisterPSOLoader(DRAW_TEXTURED_DEBUG_PSO,	{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStateDEBUGPSO	});
		RS.RegisterPSOLoader(DRAW_LINE_PSO,			    { &RS.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO			});
		RS.RegisterPSOLoader(DRAW_LINE3D_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateDraw2StatePSO				});

		RS.QueuePSOLoad(DRAW_PSO);
		RS.QueuePSOLoad(DRAW_LINE3D_PSO);
		RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);

        RS.DEBUG_AttachPIX();

        EventNotifier<>::Subscriber sub;
        sub.Notify = &EventsWrapper;
        sub._ptr = &framework;
        renderWindow.Handler->Subscribe(sub);
        renderWindow.SetWindowTitle("[S.P.A.R.]");
	}


	~BaseState()
	{
		framework.GetRenderSystem().ReleaseVB(vertexBuffer);
		framework.GetRenderSystem().ReleaseCB(constantBuffer);
        framework.core.GetBlockMemory().release_allocation(rtEngine);

        depthBuffer.Release();
        renderWindow.Release();
	}


    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
    {
        UpdateInput();
        renderWindow.UpdateCapturedMouseInput(dT);

        if(enableHud)
            debugUI.Update(renderWindow, core, dispatcher, dT);

        t += dT;

        BeginPixCapture();

        return nullptr;
    }


    void PostDrawUpdate(EngineCore& core, double dT) override
    {
        ProfileFunction();

        depthBuffer.Increment();
        renderWindow.Present(0, 0);
    }


    void Resize(const uint2 WH)
    {
        if (WH[0] * WH[1] == 0)
            return;

        auto& renderSystem  = framework.GetRenderSystem();
        auto adjustedWH     = uint2{ Max(8u, WH[0]), Max(8u, WH[1]) };

        renderWindow.Resize(adjustedWH);
        depthBuffer.Resize(adjustedWH);
        gbuffer.Resize(adjustedWH);
    }


    void DrawDebugHUD(EngineCore& core, UpdateDispatcher& dispatcher, FrameGraph& frameGraph, ReserveVertexBufferFunction& reserveVB, ReserveConstantBufferFunction& reserveCB, ResourceHandle renderTarget, double dT)
    {
        ProfileFunction();

        if (enableHud)
        {
            ImGui::NewFrame();

            DEBUG_PrintDebugStats(core);

            FlexKit::profiler.DrawProfiler(core.GetTempMemory());

            ImGui::EndFrame();
            ImGui::Render();

            debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);
        }
    }


    void DEBUG_PrintDebugStats(EngineCore& core)
    {
        ProfileFunction();

        const size_t bufferSize     = 1024;
        uint32_t VRamUsage	        = (uint32_t)(core.RenderSystem._GetVidMemUsage() / MEGABYTE);
		char* TempBuffer	        = (char*)core.GetTempMemory().malloc(bufferSize);
		auto updateTime             = framework.stats.dispatchTime;
        const char* RTFeatureStr    = core.RenderSystem.GetRTFeatureLevel() == RenderSystem::AvailableFeatures::Raytracing::RT_FeatureLevel_NOTAVAILABLE ? "Not Available" : "Available";

        const auto shadingStats         = render.GetTimingValues();
        const auto texturePassTime      = streamingEngine.debug_GetPassTime();
        const auto textureUpdateTime    = streamingEngine.debug_GetUpdateTime();

        sprintf_s(TempBuffer, bufferSize,
			"Current VRam Usage: %u MB\n"
            "FPS: %u\n"
            "dT: %fms\n"
			"Update/Draw Dispatch Time: %fms\n"
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
        ImGui::SetWindowSize({ 400, 500 });
        ImGui::Text(TempBuffer);
        ImGui::End();
    }


    void PixCapture()
    {
        PIX_SingleFrameCapture = true;
    }


    void BeginPixCapture()
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


    void EndPixCapture()
    {
        if (PIX_SingleFrameCapture && PIX_CaptureInProgress)
        {
            
        }

    }


	asIScriptEngine* asEngine;

	FKApplication&  App;


    // Debug hud
    bool                        enableHud = true;

    // counters, timers
    float                       t           = 0.0f;
    size_t                      counter     = 0;

    // Pix capture
    bool                        PIX_SingleFrameCapture  = false;
    bool                        PIX_CaptureInProgress   = false;
    uint32_t                    PIX_frameCaptures       = 0;

    // Graphics resources
    Win32RenderWindow           renderWindow;
	WorldRender					render;
    GBuffer                     gbuffer;
    DepthBuffer				    depthBuffer;
	VertexBufferHandle			vertexBuffer;
	ConstantBufferHandle		constantBuffer;

	// Components
    AnimatorComponent               animations;
    FABRIKComponent                 IK;
    FABRIKTargetComponent           IKTargets;
	SceneNodeComponent			    transforms;
	CameraComponent				    cameras;
	StringIDComponent			    ids;
	BrushComponent			        brushes;
    MaterialComponent               materials;
	SceneVisibilityComponent	    visables;
	PointLightComponent			    pointLights;
    SkeletonComponent               skeletonComponent;
    PointLightShadowMap             shadowCasters;
    PhysXComponent  	            physics;
    RigidBodyComponent              rigidBodies;
    StaticBodyComponent             staticBodies;
    CharacterControllerComponent    characterControllers;
    CameraControllerComponent       orbitCameras;
    SoundSystem			            sounds;


    ImGUIIntegrator                 debugUI;
	TextureStreamingEngine		    streamingEngine;
    iRayTracer&                     rtEngine;
};


/************************************************************************************************/
