#pragma once



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

#include "..\graphicsutilities\AnimationComponents.h"
#include "..\coreutilities\Application.h"
#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\Materials.h"
#include "..\coreutilities\WorldRender.h"
#include "..\graphicsutilities\TextRendering.h"
#include "..\graphicsutilities\defaultpipelinestates.h"

#include <angelscript.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptbuilder/scriptbuilder.h>

#include <fmod.hpp>

using FlexKit::WorldRender;
using FlexKit::FKApplication;


/************************************************************************************************/


#pragma comment(lib, "fmod_vc.lib")

#ifdef _DEBUG
#pragma comment(lib, "angelscript64d.lib")
#else
#pragma comment(lib, "angelscript64.lib")
#endif

using namespace FlexKit;

typedef uint32_t SoundHandle;


class SoundSystem
{
public:
	SoundSystem(ThreadManager& IN_Threads, iAllocator* memory) :
		threads	{	IN_Threads	}
	{
        /*
		result			= FMOD::System_Create(&system);
		auto initres	= system->init(32, FMOD_INIT_NORMAL, nullptr);

		auto& Work = CreateWorkItem(
			[this]() {
			//auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &sound1);
			if(sound1)
				system->playSound(sound1, nullptr, false, &channel);
		}, memory, memory);

        Work._debugID = "FMOD: Play Sound";
		threads.AddWork(Work, memory);
        */
	}


	~SoundSystem()
	{
		//sound1->release();
		//system->release();
		//system = nullptr;
	}


	SoundHandle LoadSoundFromDisk(const char* str)
	{
		return -1;
	}


	void Update(iAllocator* memory)
	{
        /*
		auto& Work = CreateWorkItem(
			[this]() {
				auto result = system->update();
			}, 
			memory);

        Work._debugID = "FMOD: Update Sound";
		threads.AddWork(Work, memory);
        */
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
		FKApplication& IN_App	) :
			App				{ IN_App },
			FrameworkState	{ IN_Framework },
			depthBuffer		{ IN_Framework.core.RenderSystem.CreateDepthBuffer(IN_Framework.ActiveWindow->WH,	true) },

            temporaryBuffers{
                { IN_Framework.core.RenderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(IN_Framework.ActiveWindow->WH, DeviceFormat::R16G16B16A16_FLOAT))	},
                { IN_Framework.core.RenderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(IN_Framework.ActiveWindow->WH, DeviceFormat::R16G16B16A16_FLOAT))	}},

            temporaryBuffers_2Channel{
                { IN_Framework.core.RenderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(IN_Framework.ActiveWindow->WH, DeviceFormat::R16G16_FLOAT)) },
                { IN_Framework.core.RenderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(IN_Framework.ActiveWindow->WH, DeviceFormat::R16G16_FLOAT)) } },


			vertexBuffer	{ IN_Framework.core.RenderSystem.CreateVertexBuffer(MEGABYTE * 1, false) },
			constantBuffer	{ IN_Framework.core.RenderSystem.CreateConstantBuffer(MEGABYTE * 128, false) },
			asEngine		{ asCreateScriptEngine() },
			streamingEngine	{ IN_Framework.core.RenderSystem, IN_Framework.core.GetBlockMemory() },
            sounds          { IN_Framework.core.Threads,      IN_Framework.core.GetBlockMemory()    },

			render	{	IN_Framework.core.GetTempMemory(),
						IN_Framework.core.RenderSystem,
						streamingEngine,
						IN_Framework.ActiveWindow->WH
					},

			cameras		        { framework.core.GetBlockMemory() },
			ids			        { framework.core.GetBlockMemory() },
			drawables	        { framework.core.GetBlockMemory(), IN_Framework.GetRenderSystem() },
            materials           { IN_Framework.core.RenderSystem, streamingEngine, framework.core.GetBlockMemory() },
			visables	        { framework.core.GetBlockMemory() },
			pointLights	        { framework.core.GetBlockMemory() },
            skeletonComponent   { framework.core.GetBlockMemory() },
            gbuffer             { IN_Framework.ActiveWindow->WH, framework.core.RenderSystem },
            shadowCasters       { IN_Framework.core.GetBlockMemory() },
            physics             { IN_Framework.core.Threads, IN_Framework.core.GetBlockMemory() },
            rigidBodies         { physics },
            staticBodies        { physics },
            characterControllers{ physics, framework.core.GetBlockMemory() },
            orbitCameras        { framework.core.GetBlockMemory() }
	{
		auto& RS = *IN_Framework.GetRenderSystem();
		RS.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS6CBVs4SRVs, LoadSpriteTextPSO		        });
		RS.RegisterPSOLoader(DRAW_PSO,					{ &RS.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO			});
		RS.RegisterPSOLoader(DRAW_TEXTURED_PSO,		    { &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStatePSO		});
		RS.RegisterPSOLoader(DRAW_TEXTURED_DEBUG_PSO,	{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStateDEBUGPSO	});
		RS.RegisterPSOLoader(DRAW_LINE_PSO,			    { &RS.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO			});
		RS.RegisterPSOLoader(DRAW_LINE3D_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateDraw2StatePSO				});

		RS.QueuePSOLoad(DRAW_PSO);
		RS.QueuePSOLoad(DRAW_LINE3D_PSO);
		RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);
	}


	~BaseState()
	{
		framework.GetRenderSystem().ReleaseVB(vertexBuffer);
		framework.GetRenderSystem().ReleaseCB(constantBuffer);
        framework.GetRenderSystem().ReleaseTexture(depthBuffer);
        framework.GetRenderSystem().ReleaseTexture(irradianceMap);
        framework.GetRenderSystem().ReleaseTexture(GGXMap);
	}


    void Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
    {
        physics.Update(dT, core.GetTempMemory());
        t += dT;
    }


    void Resize(const uint2 WH)
    {
        if (WH[0] * WH[1] == 0)
            return;

        auto& renderSystem  = framework.GetRenderSystem();
        auto adjustedWH     = uint2{ max(8u, WH[0]), max(8u, WH[1]) };

        framework.core.Window.Resize(adjustedWH, renderSystem);

        for (auto& temp : temporaryBuffers) {
            renderSystem.ReleaseTexture(temp);
            temp = renderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(adjustedWH, DeviceFormat::R16G16B16A16_FLOAT));
        }

        for (auto& temp : temporaryBuffers_2Channel) {
            renderSystem.ReleaseTexture(temp);
            temp = renderSystem.CreateGPUResource(GPUResourceDesc::RenderTarget(adjustedWH, DeviceFormat::R16G16_FLOAT));
        }

        renderSystem.ReleaseTexture(depthBuffer);
        depthBuffer     = renderSystem.CreateDepthBuffer(adjustedWH, true);
        gbuffer.Resize(adjustedWH);
    }


	asIScriptEngine* asEngine;

	FKApplication& App;

    // counters, timers
    float                       t = 0.0f;


    // Scene Resources
    ResourceHandle             irradianceMap;
    ResourceHandle             GGXMap;

    // render resources
	WorldRender					render;
    GBuffer                     gbuffer;
	ResourceHandle				depthBuffer;
	VertexBufferHandle			vertexBuffer;
	ConstantBufferHandle		constantBuffer;
    ResourceHandle              temporaryBuffers[2];
    ResourceHandle              temporaryBuffers_2Channel[2];
	
    SoundSystem			        sounds;

	// Components
	SceneNodeComponent			    transforms;
	CameraComponent				    cameras;
	StringIDComponent			    ids;
	DrawableComponent			    drawables;
    MaterialComponent               materials;
	SceneVisibilityComponent	    visables;
	PointLightComponent			    pointLights;
    SkeletonComponent               skeletonComponent;
    PointLightShadowCaster          shadowCasters;
    PhysXComponent  	            physics;
    RigidBodyComponent              rigidBodies;
    StaticBodyComponent             staticBodies;
    CharacterControllerComponent    characterControllers;
    CameraControllerComponent       orbitCameras;

	TextureStreamingEngine		streamingEngine;
};


/************************************************************************************************/
