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

#include <fmod.hpp>

using FlexKit::WorldRender;
using FlexKit::ReleaseCameraTable;
using FlexKit::FKApplication;


/************************************************************************************************/


#pragma comment(lib, "fmod64_vc.lib")

using namespace FlexKit;

typedef uint32_t SoundHandle;


class SoundSystem
{
public:
	SoundSystem(ThreadManager& IN_Threads, iAllocator* Memory) :
		threads	{	IN_Threads	}
	{
		result			= FMOD::System_Create(&system);
		auto initres	= system->init(32, FMOD_INIT_NORMAL, nullptr);

		auto& Work = CreateLambdaWork_New(
			[this]() {
			//auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &sound1);
			if(sound1)
				system->playSound(sound1, nullptr, false, &channel);
		});

		threads.AddWork(Work, Memory);
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


	void Update()
	{
		auto result = system->update();
	}


	FlexKit::Vector<FMOD::Sound*> Sounds;

	ThreadManager&	threads;

	FMOD::System     *system;
	FMOD::Sound      *sound1, *sound2, *sound3;
	FMOD::Channel    *channel = 0;
	FMOD_RESULT       result;
	unsigned int      version;
};


/************************************************************************************************/


inline FlexKit::UpdateTask* QueueSoundUpdate(FlexKit::UpdateDispatcher& Dispatcher, SoundSystem* Sounds)
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
		[](auto& Data)
		{
			FK_LOG_9("Sound Update");
			Data.Sounds->Update();
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

			render	{	IN_Framework->core->GetTempMemory(),
						IN_Framework->core->RenderSystem	}
	{
		InitiateCameraTable(framework->core->GetBlockMemory());
		
		auto& RS = *IN_Framework->GetRenderSystem();
		RS.RegisterPSOLoader(FlexKit::DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS4CBVs4SRVs, FlexKit::LoadSpriteTextPSO		});
		RS.RegisterPSOLoader(FlexKit::DRAW_PSO,					{ &RS.Library.RS4CBVs4SRVs, CreateDrawTriStatePSO			});
		RS.RegisterPSOLoader(FlexKit::DRAW_TEXTURED_PSO,		{ &RS.Library.RS4CBVs4SRVs, CreateTexturedTriStatePSO		});
		RS.RegisterPSOLoader(FlexKit::DRAW_TEXTURED_DEBUG_PSO,	{ &RS.Library.RS4CBVs4SRVs, CreateTexturedTriStateDEBUGPSO	});
		RS.RegisterPSOLoader(FlexKit::DRAW_LINE_PSO,			{ &RS.Library.RS4CBVs4SRVs, CreateDrawLineStatePSO			});
		RS.RegisterPSOLoader(FlexKit::DRAW_LINE3D_PSO,			{ &RS.Library.RS4CBVs4SRVs, CreateDraw2StatePSO				});
		RS.RegisterPSOLoader(FlexKit::DRAW_SPRITE_TEXT_PSO,		{ &RS.Library.RS4CBVs4SRVs, LoadSpriteTextPSO				});

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

		ReleaseCameraTable();
	}


	FKApplication* App;

	FlexKit::WorldRender				render;
	FlexKit::TextureHandle				depthBuffer;
	FlexKit::VertexBufferHandle			vertexBuffer;
	FlexKit::VertexBufferHandle			textBuffer;
	FlexKit::ConstantBufferHandle		constantBuffer;
};


/************************************************************************************************/