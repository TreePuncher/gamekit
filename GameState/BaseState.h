#pragma once



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

#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\EngineCore.h"
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


#pragma comment(lib, "fmod64_vc.lib")

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
		result			= FMOD::System_Create(&system);
		auto initres	= system->init(32, FMOD_INIT_NORMAL, nullptr);

		auto& Work = CreateLambdaWork_New(
			[this]() {
			//auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &sound1);
			if(sound1)
				system->playSound(sound1, nullptr, false, &channel);
		}, memory, memory);

		threads.AddWork(Work, memory);
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
		auto& Work = CreateLambdaWork_New(
			[this]() {
				auto result = system->update();
			}, 
			memory);

		threads.AddWork(Work, memory);

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
		[allocator](auto& Data)
		{
			FK_LOG_9("Sound Update");
			Data.Sounds->Update(allocator);
		});

	return &SoundUpdate;
}


/************************************************************************************************/


class BaseState : public FlexKit::FrameworkState
{
public:
	BaseState(	
		FlexKit::GameFramework* IN_Framework,
		FlexKit::FKApplication* IN_App	) :
			App				{ IN_App																						},
			FrameworkState	{ IN_Framework																					},
			depthBuffer		{ IN_Framework->core->RenderSystem.CreateDepthBuffer(IN_Framework->ActiveWindow->WH,	true)	},
			vertexBuffer	{ IN_Framework->core->RenderSystem.CreateVertexBuffer(8096 * 64, false)							},
			textBuffer		{ IN_Framework->core->RenderSystem.CreateVertexBuffer(8096 * 64, false)							},
			constantBuffer	{ IN_Framework->core->RenderSystem.CreateConstantBuffer(8096 * 2000, false)						},
			asEngine		{ asCreateScriptEngine()																		},
			streamingEngine	{ IN_Framework->core->RenderSystem, IN_Framework->core->GetBlockMemory()						},

			render	{	IN_Framework->core->GetTempMemory(),
						IN_Framework->core->RenderSystem,
						streamingEngine	
					},

			cameras	{ framework->core->GetBlockMemory() }
	{
		auto& RS = *IN_Framework->GetRenderSystem();
		RS.RegisterPSOLoader(FlexKit::DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS6CBVs4SRVs, FlexKit::LoadSpriteTextPSO		});
		RS.RegisterPSOLoader(FlexKit::DRAW_PSO,					{ &RS.Library.RS6CBVs4SRVs, CreateDrawTriStatePSO			});
		RS.RegisterPSOLoader(FlexKit::DRAW_TEXTURED_PSO,		{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStatePSO		});
		RS.RegisterPSOLoader(FlexKit::DRAW_TEXTURED_DEBUG_PSO,	{ &RS.Library.RS6CBVs4SRVs, CreateTexturedTriStateDEBUGPSO	});
		RS.RegisterPSOLoader(FlexKit::DRAW_LINE_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateDrawLineStatePSO			});
		RS.RegisterPSOLoader(FlexKit::DRAW_LINE3D_PSO,			{ &RS.Library.RS6CBVs4SRVs, CreateDraw2StatePSO				});
		RS.RegisterPSOLoader(FlexKit::DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS6CBVs4SRVs, LoadSpriteTextPSO				});

		RS.QueuePSOLoad(DRAW_PSO);
		RS.QueuePSOLoad(DRAW_LINE3D_PSO);
		RS.QueuePSOLoad(DRAW_TEXTURED_DEBUG_PSO);

		AddResourceFile("assets\\testScene.gameres");
	}


	~BaseState()
	{
		framework->core->RenderSystem.ReleaseVB(vertexBuffer);
		framework->core->RenderSystem.ReleaseVB(textBuffer);
		framework->core->RenderSystem.ReleaseCB(constantBuffer);
		framework->core->RenderSystem.ReleaseDB(depthBuffer);
	}

	asIScriptEngine* asEngine;

	FKApplication* App;

	FlexKit::WorldRender				render;
	FlexKit::TextureHandle				depthBuffer;
	FlexKit::VertexBufferHandle			vertexBuffer;
	FlexKit::VertexBufferHandle			textBuffer;
	FlexKit::ConstantBufferHandle		constantBuffer;
	
	// Components
	SceneNodeComponent					transforms;
	CameraComponent						cameras;

	TextureStreamingEngine				streamingEngine;
};


/************************************************************************************************/