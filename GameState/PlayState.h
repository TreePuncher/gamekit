#ifndef PLAYSTATE_H
#define PLAYSTATE_H

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
#include "..\coreutilities\CameraUtilities.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\WorldRender.h"
#include "..\coreutilities\Components.h"

#include "Gameplay.h"
#include "BaseState.h"

#include <fmod.hpp>

/************************************************************************************************/


using namespace FlexKit;

typedef uint32_t SoundHandle;

class FMOD_SoundSystem
{
public:
	FMOD_SoundSystem(ThreadManager& IN_Threads, iAllocator* Memory) : 
		Threads	{	IN_Threads	}
	{
		result			= FMOD::System_Create(&system);
		auto initres	= system->init(32, FMOD_INIT_NORMAL, nullptr);

		auto& Work = CreateLambdaWork_New(
			[this]() {
			//auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &sound1);
			if(sound1)
				system->playSound(sound1, nullptr, false, &channel);
		});

		Threads.AddWork(Work, Memory);
	}

	~FMOD_SoundSystem()
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

	ThreadManager& Threads;

	FMOD::System     *system;
	FMOD::Sound      *sound1, *sound2, *sound3;
	FMOD::Channel    *channel = 0;
	FMOD_RESULT       result;
	unsigned int      version;
};


/************************************************************************************************/


inline FlexKit::UpdateTask* QueueSoundUpdate(FlexKit::UpdateDispatcher& Dispatcher, FMOD_SoundSystem* Sounds)
{
	struct SoundUpdateData
	{
		FMOD_SoundSystem* Sounds;
	};

	FMOD_SoundSystem* Sounds_ptr = nullptr;
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


class PlayState : public FrameworkState
{
public:
	PlayState(
		GameFramework*	IN_Framework, 
		BaseState*		Base);
	

	~PlayState();

	bool Update			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool PreDrawUpdate	(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool Draw			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;
	bool PostDrawUpdate	(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;

	bool DebugDraw		(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;

	bool EventHandler	(Event evt)	final;

	void BindPlayer1();
	void BindPlayer2();

	void ReleasePlayer1();
	void ReleasePlayer2();

	bool GameInPlay;
	bool UseDebugCamera;


	GraphicScene			Scene;

	WorldRender*			Render;
	TextureHandle			DepthBuffer;
	ConstantBufferHandle	ConstantBuffer;
	VertexBufferHandle		VertexBuffer;
	VertexBufferHandle		TextBuffer;

	Game					LocalGame;
	LocalPlayerHandler		Player1_Handler;
	LocalPlayerHandler		Player2_Handler;
	DebugCameraController	OrbitCamera;

	FMOD_SoundSystem	Sound;
	PlayerPuppet		Puppet;

	EventList									FrameEvents;
	FlexKit::CircularBuffer<FrameSnapshot, 120>	FrameCache;

	FlexKit::GuiSystem	UI;
	FlexKit::GUIGrid*	UIMainGrid;
	FlexKit::GUIGrid*	UISubGrid_1;
	FlexKit::GUIGrid*	UISubGrid_2;

	InputMap		eventMap;
	InputMap		DebugCameraInputMap;
	InputMap		DebugEventsInputMap;
	size_t			FrameID;
	TriMeshHandle	CharacterModel;
};


/************************************************************************************************/

#endif