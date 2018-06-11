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

#include "PlayState.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\PipelineState.h"
#include "..\graphicsutilities\ImageUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\PipelineState.h"


//#define DEBUGCAMERA

#pragma comment(lib, "fmod64_vc.lib")

PlayState::PlayState(
	GameFramework*			IN_Framework,
	WorldRender*			IN_Render,
	TextureHandle			IN_DepthBuffer,
	VertexBufferHandle		IN_VertexBuffer,
	VertexBufferHandle		IN_TextBuffer,
	ConstantBufferHandle	IN_ConstantBuffer) :
		FrameworkState	{IN_Framework},

		Sound			{Framework->Core->Threads},
		DepthBuffer		{IN_DepthBuffer},
		Render			{IN_Render},
		Grid			{Framework->Core->GetBlockMemory()},
		Player1_Handler	{Grid, Framework->Core->GetBlockMemory()},
		Player2_Handler	{Grid, Framework->Core->GetBlockMemory()},
		GameInPlay		{true},
		TextBuffer		{IN_TextBuffer},
		VertexBuffer	{IN_VertexBuffer},
		EventMap		{Framework->Core->GetBlockMemory()},
		Scene			
			{
				Framework->Core->GetBlockMemory(),
				Framework->Core->GetTempMemory()
			}
{
	InitiateOrbitCameras(Framework->Core->GetBlockMemory());

	Player1_Handler.SetActive(Grid.CreatePlayer({ 11, 11 }));
	Grid.CreateGridObject({10, 5});

#ifndef DEBUGCAMERA
	EventMap.MapKeyToEvent(KEYCODES::KC_W,			PLAYER_EVENTS::PLAYER_UP);
	EventMap.MapKeyToEvent(KEYCODES::KC_A,			PLAYER_EVENTS::PLAYER_LEFT);
	EventMap.MapKeyToEvent(KEYCODES::KC_S,			PLAYER_EVENTS::PLAYER_DOWN);
	EventMap.MapKeyToEvent(KEYCODES::KC_D,			PLAYER_EVENTS::PLAYER_RIGHT);
	EventMap.MapKeyToEvent(KEYCODES::KC_LEFTSHIFT,	PLAYER_EVENTS::PLAYER_HOLD);
	EventMap.MapKeyToEvent(KEYCODES::KC_SPACE,		PLAYER_EVENTS::PLAYER_ACTION1);
#endif

	// Debug Orbit Camera
#ifdef DEBUGCAMERA
	EventMap.MapKeyToEvent(KEYCODES::KC_W, PLAYER_EVENTS::DEBUG_PLAYER_UP);
	EventMap.MapKeyToEvent(KEYCODES::KC_A, PLAYER_EVENTS::DEBUG_PLAYER_LEFT);
	EventMap.MapKeyToEvent(KEYCODES::KC_S, PLAYER_EVENTS::DEBUG_PLAYER_DOWN);
	EventMap.MapKeyToEvent(KEYCODES::KC_D, PLAYER_EVENTS::DEBUG_PLAYER_RIGHT);
#endif

	Framework->Core->RenderSystem.PipelineStates.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO, LoadSpriteTextPSO );
	Framework->Core->RenderSystem.PipelineStates.QueuePSOLoad(DRAW_SPRITE_TEXT_PSO);
}


/************************************************************************************************/


PlayState::~PlayState()
{
	ReleaseOrbitCameras(Framework->Core->GetBlockMemory());

	// TODO: Make this not stoopid 
	Framework->Core->GetBlockMemory().free(this);
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
#ifndef DEBUGCAMERA
	Event Remapped;
	if (Player1_Handler.Enabled && EventMap.Map(evt, Remapped))
		Player1_Handler.Handle(Remapped);

	//if (Player2_Handler.Enabled)
	//	Player2_Handler.Handle(evt);
#else
	Event Remapped;
	//if(EventMap.Map(evt, Remapped))
	//	DebugCamera.EventHandler(Remapped);
#endif

	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
{
#ifndef DEBUGCAMERA
	// Check if any players Died
	for (auto& P : Grid.Players)
		if (Grid.IsCellDestroyed(P.XY))
			Framework->PopState();

	if (Player1_Handler.Enabled)
		Player1_Handler.Update(dT);

	if (Player2_Handler.Enabled)
		Player2_Handler.Update(dT);
	Grid.Update(dT, Engine->GetTempMemory());

	Sound.Update();


	struct SoundUpdateData
	{
		FMOD_SoundSystem* Sounds;
	};

	FMOD_SoundSystem* Sounds_ptr = nullptr;
	auto& SoundUpdate = Dispatcher.Add<SoundUpdateData>(
		[&](auto& Builder, SoundUpdateData& Data)
		{
			Data.Sounds = &Sound;
			Builder.SetDebugString("UpdateSound");
	},
		[](auto& Data)
		{
			Data.Sounds->Update();
			std::cout << "Updating Sound!\n";
		});

	struct SomeAfterSoundUpdateTask
	{
	};

	auto& RandomTask1 = Dispatcher.Add<SomeAfterSoundUpdateTask>(
		[&](auto& Builder, SomeAfterSoundUpdateTask& Data)
		{
			Builder.AddInput(SoundUpdate);
			Builder.SetDebugString("SomeAfterSoundUpdateTask");
	},
		[](SomeAfterSoundUpdateTask& Data)
		{
			std::cout << "This happened after sound update!\n";
		});


	struct SomeBeforeSoundUpdateTask
	{
	};

	auto& RandomTask2 = Dispatcher.Add<SomeBeforeSoundUpdateTask>(
		[&](auto& Builder, SomeBeforeSoundUpdateTask& Data)
		{
			Builder.AddOutput(SoundUpdate);
			Builder.SetDebugString("SomeBeforeSoundUpdateTask");
		}, 
		[](SomeBeforeSoundUpdateTask& Data)
		{
			std::cout << "This happened before sound update even though I was scheduled after the sound Update!\n";
		});

#else
	float HorizontalMouseMovement	= float(Framework->MouseState.dPos[0]) / GetWindowWH(Framework->Core)[0];
	float VerticalMouseMovement		= float(Framework->MouseState.dPos[1]) / GetWindowWH(Framework->Core)[1];

	Framework->MouseState.Normalized_dPos = { HorizontalMouseMovement, VerticalMouseMovement };
	//DebugCamera.Yaw(Framework->MouseState.Normalized_dPos[0]);
	//DebugCamera.Pitch(Framework->MouseState.Normalized_dPos[1]);

	//DebugCamera.Update(dT);
	//LocalPlayer.Update(dT);
#endif



	return false;
}


/************************************************************************************************/

bool PlayState::DebugDraw(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool PlayState::PreDrawUpdate(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
	return false;
}


/************************************************************************************************/


bool PlayState::Draw(EngineCore* Core, UpdateDispatcher& Dispatcher, double dt, FrameGraph& FrameGraph)
{
	FrameGraph.Resources.AddDepthBuffer(DepthBuffer);

	WorldRender_Targets Targets = {
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer
	};

	PVS	Drawables_Solid(Core->GetTempMemory());
	PVS	Drawables_Transparent(Core->GetTempMemory());

	ClearVertexBuffer	(FrameGraph, VertexBuffer);
	ClearVertexBuffer	(FrameGraph, TextBuffer);

	ClearBackBuffer	(FrameGraph, 0.0f);
	ClearDepthBuffer(FrameGraph, DepthBuffer, 1.0f);

#ifndef DEBUGCAMERA
	DrawGameGrid_Debug(
		dt,
		GetWindowAspectRatio(Core),
		Grid,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		Core->GetTempMemory()
	);
#else
	Camera* ActiveCamera = nullptr;
	PVS Drawables				{Core->GetTempMemory()};
	PVS TransparentDrawables	{Core->GetTempMemory()};

	GetGraphicScenePVS(Scene, ActiveCamera, &Drawables, &TransparentDrawables);

	Render->DefaultRender(
		Drawables, 
		*ActiveCamera, 
		Targets,
		FrameGraph, 
		Core->GetTempMemory());
#endif

	DrawSprite_Text(
		"!!!!This is sample text rendered using Sprites Text!!!!",
		FrameGraph, 
		*Framework->DefaultAssets.Font,
		TextBuffer,
		GetCurrentBackBuffer(&Core->Window),
		Core->GetTempMemory());

	PresentBackBuffer	(FrameGraph, &Core->Window);

	return true;
}


/************************************************************************************************/


void PlayState::BindPlayer1()
{

}


/************************************************************************************************/


void PlayState::BindPlayer2()
{

}


/************************************************************************************************/


void PlayState::ReleasePlayer1()
{
}


/************************************************************************************************/


void PlayState::ReleasePlayer2()
{

}


/************************************************************************************************/
