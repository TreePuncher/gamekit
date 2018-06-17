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

#include "..\Application\GameFramework.h"
#include "..\Application\CameraUtilities.h"
#include "..\Application\GameMemory.h"
#include "..\Application\WorldRender.h"
#include "..\coreutilities\Components.h"

#include "Gameplay.h"

#include <fmod.hpp>

/************************************************************************************************/


typedef uint32_t SoundHandle;

class FMOD_SoundSystem
{
public:
	FMOD_SoundSystem(FlexKit::ThreadManager& IN_Threads) : 
		Threads	{	IN_Threads	}
	{
		result			= FMOD::System_Create(&system);
		auto initres	= system->init(32, FMOD_INIT_NORMAL, nullptr);

		Threads.AddWork(CreateLambdaWork_New(
			[this]() {
				//auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &sound1);
				//if(sound1)
				//	system->playSound(sound1, nullptr, false, &channel);
		}));
		
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

	Vector<FMOD::Sound*> Sounds;

	FlexKit::ThreadManager& Threads;

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

/*
class PlayerCameraController : 
	public FlexKit::CameraBehavior,
	public FlexKit::SceneNodeBehavior
{
public:
	PlayerCameraController(ComponentListInterface& IN_Components) :
			Components			{IN_Components},
			CameraBehavior		{IN_Components},
			SceneNodeBehavior	{IN_Components}{}

	ComponentListInterface& Components;

	void Update(float dt)
	{

	}
};

ComponentListInterface& InitiatePlayerCameraController(
	CameraComponentSystem*		CameraSystem,
	SceneNodeComponentSystem*	SceneNodeSystem,
	ComponentList<>&			Components)
{
	auto SceneNode = SceneNodeSystem->GetNewNode();

	InitiateComponentList(Components,
		CreateCameraComponent(
			CameraSystem,
			1.6666f,
			0.01f,
			10000.0f,
			SceneNode,
			false));

	return Components;
}
*/

/************************************************************************************************/


class DebugCameraController : 
	public OrbitCameraBehavior
{
public:
	void Initiate()
	{
	}

	bool EventHandler(Event evt)
	{
		if (evt.InputSource == Event::Keyboard)
		{
			if (evt.Action == Event::Pressed)
			{
				switch (evt.mData1.mINT[0])
				{
				case PLAYER_EVENTS::DEBUG_PLAYER_UP:
					MoveForward		= true;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_LEFT:
					MoveLeft		= true;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_DOWN:
					MoveBackward	= true;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_RIGHT:
					MoveRight		= true;
					break;
				default:
					break;
				};
			}

			if (evt.Action == Event::Release)
			{
				switch (evt.mData1.mINT[0])
				{
				case PLAYER_EVENTS::DEBUG_PLAYER_UP:
					MoveForward		= false;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_DOWN:
					MoveBackward	= false;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_LEFT:
					MoveLeft		= false;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_RIGHT:
					MoveRight		= false;
					break;
				default:
					break;
				}
			}
		}
		return false;
	}


	void Update(float dt)
	{
		FlexKit::float3 Direction = { 0, 0, 0 };

		if (MoveForward)
			Direction += float3( 0,  0, -1);
		if (MoveBackward)
			Direction += float3( 0,  0,  1);
		if (MoveLeft)
			Direction += float3(-1, 0, 0);
		if (MoveRight)
			Direction += float3( 1, 0, 0);

		Direction = GetOrientation() * Direction;

		TranslateWorld(Direction * dt * MovingSpeed);
	}

	float		MovingSpeed		= 30.0f;
	bool		MoveForward		= false;
	bool		MoveBackward	= false;
	bool		MoveLeft		= false;
	bool		MoveRight		= false;
};


/************************************************************************************************/


class PlayerPuppet:
		public DrawableBehavior,
		public SceneNodeBehavior
{
public: 
	PlayerPuppet(GraphicScene* ParentScene, EntityHandle Handle) :
		DrawableBehavior	{ParentScene, Handle},
		SceneNodeBehavior	{GetNode()}
	{}


	PlayerPuppet(const PlayerPuppet&) = delete;


	PlayerPuppet(PlayerPuppet && rhs)
	{
		Node		 = rhs.Node;
		Entity		 = rhs.Entity;
		ParentScene	 = rhs.ParentScene;
	}


	void Update(float dt)
	{
		Yaw(dt * pi);
	}
};


PlayerPuppet CreatePlayerPuppet(GraphicScene* ParentScene)
{
	return 	PlayerPuppet(
		ParentScene,
		ParentScene->CreateDrawableAndSetMesh("Flower"));
}


/************************************************************************************************/


enum ModelHandles
{
	MH_Floor,
	MH_BrokenFloor,
	MH_Obstacle,
	MH_Player,
	MH_RegularBomb,
};


/************************************************************************************************/


typedef Vector<Event> EventList;

class FrameSnapshot
{
public:
	FrameSnapshot(Game* IN = nullptr, EventList* IN_FrameEvents = nullptr, size_t IN_FrameID = (size_t)-1, iAllocator* IN_Memory = nullptr);
	~FrameSnapshot();

	FrameSnapshot(const FrameSnapshot&)					= delete;
	FrameSnapshot& operator = (const FrameSnapshot&)	= delete;

	FrameSnapshot& operator = (FrameSnapshot&& rhs)
	{
		FrameCopy	= std::move(rhs.FrameCopy);
		FrameEvents = std::move(rhs.FrameEvents);

		return *this;
	}

	void Restore	(Game* out);
	
	EventList		FrameEvents;
	Game			FrameCopy;
	size_t			FrameID;

	iAllocator* Memory;
};


/************************************************************************************************/


class PlayState : public FrameworkState
{
public:
	PlayState(
		GameFramework*			Framework,
		WorldRender*			Render,
		TextureHandle			DepthBuffer,
		VertexBufferHandle		VertexBuffer,
		VertexBufferHandle		TextBuffer,
		ConstantBufferHandle	ConstantBuffer);

	~PlayState();

	bool Update			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool PreDrawUpdate	(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool Draw			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;
	bool DebugDraw		(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;

	bool EventHandler	(Event evt)	final;

	void BindPlayer1();
	void BindPlayer2();

	void ReleasePlayer1();
	void ReleasePlayer2();

	bool GameInPlay;

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

	InputMap EventMap;
	InputMap DebugCameraInputMap;
	size_t	 FrameID;
};


/************************************************************************************************/

#endif