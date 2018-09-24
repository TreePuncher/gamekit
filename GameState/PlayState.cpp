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
#include "..\graphicsutilities\TextureUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\PipelineState.h"


#define DEBUGCAMERA

#pragma comment(lib, "fmod64_vc.lib")





/************************************************************************************************/


PlayState::PlayState(
		GameFramework*			IN_Framework,
		BaseState*				Base) : 
	FrameworkState		{ IN_Framework						},

	UI					{ FlexKit::GuiSystem_Desc{}, Framework->Core->GetBlockMemory() },
	UIMainGrid			{ nullptr },
	UISubGrid_1			{ nullptr },
	UISubGrid_2			{ nullptr },

	FrameEvents			{ Framework->Core->GetBlockMemory() },
	FrameID				{ 0 },
	Sound				{ Framework->Core->Threads, Framework->Core->GetBlockMemory() },
	Render				{ &Base->render				},
	LocalGame			{ Framework->Core->GetBlockMemory() },
	Player1_Handler		{ LocalGame, Framework->Core->GetBlockMemory() },
	Player2_Handler		{ LocalGame, Framework->Core->GetBlockMemory() },
	GameInPlay			{ true					},
	UseDebugCamera		{ false					},

	ConstantBuffer		{ Base->constantBuffer	}, 
	DepthBuffer			{ Base->depthBuffer		},
	TextBuffer			{ Base->textBuffer		},
	VertexBuffer		{ Base->vertexBuffer	},

	eventMap			{ Framework->Core->GetBlockMemory() },
	DebugCameraInputMap	{ Framework->Core->GetBlockMemory() },
	DebugEventsInputMap	{ Framework->Core->GetBlockMemory() },
	Puppet				{ CreatePlayerPuppet(Scene)			},
	Scene
						{
							Framework->Core->RenderSystem,
							Framework->Core->GetBlockMemory(),
							Framework->Core->GetTempMemory()
						}
{
	AddResourceFile("CharacterBase.gameres");
	CharacterModel = FlexKit::LoadTriMeshIntoTable(Framework->Core->RenderSystem, "PlayerMesh");

	Player1_Handler.SetActive(LocalGame.CreatePlayer({ 11, 11 }));
	Player1_Handler.SetPlayerCameraAspectRatio(GetWindowAspectRatio(Framework->Core));

	LocalGame.CreateGridObject({ 10, 5 });

	OrbitCamera.SetCameraAspectRatio(GetWindowAspectRatio(Framework->Core));
	OrbitCamera.TranslateWorld({ 0, 2, -5 });
	OrbitCamera.Yaw(float(pi));


	eventMap.MapKeyToEvent(KEYCODES::KC_W,			PLAYER_EVENTS::PLAYER_UP);
	eventMap.MapKeyToEvent(KEYCODES::KC_A,			PLAYER_EVENTS::PLAYER_LEFT);
	eventMap.MapKeyToEvent(KEYCODES::KC_S,			PLAYER_EVENTS::PLAYER_DOWN);
	eventMap.MapKeyToEvent(KEYCODES::KC_D,			PLAYER_EVENTS::PLAYER_RIGHT);
	eventMap.MapKeyToEvent(KEYCODES::KC_Q,			PLAYER_EVENTS::PLAYER_ROTATE_LEFT);
	eventMap.MapKeyToEvent(KEYCODES::KC_E,			PLAYER_EVENTS::PLAYER_ROTATE_RIGHT);
	eventMap.MapKeyToEvent(KEYCODES::KC_LEFTSHIFT,	PLAYER_EVENTS::PLAYER_HOLD);
	eventMap.MapKeyToEvent(KEYCODES::KC_SPACE,		PLAYER_EVENTS::PLAYER_ACTION1);


	// Debug Orbit Camera
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_I, PLAYER_EVENTS::DEBUG_PLAYER_UP);
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_J, PLAYER_EVENTS::DEBUG_PLAYER_LEFT);
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_K, PLAYER_EVENTS::DEBUG_PLAYER_DOWN);
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_L, PLAYER_EVENTS::DEBUG_PLAYER_RIGHT);


	// Debug Events, are not stored in Frame cache
	DebugEventsInputMap.MapKeyToEvent(KEYCODES::KC_C, DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA);
	DebugEventsInputMap.MapKeyToEvent(KEYCODES::KC_X, DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY);

	Framework->Core->RenderSystem.PipelineStates.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO, LoadSpriteTextPSO);
	Framework->Core->RenderSystem.PipelineStates.QueuePSOLoad(DRAW_SPRITE_TEXT_PSO, Framework->Core->GetBlockMemory());

	UIMainGrid	= &UI.CreateGrid(nullptr);
	UIMainGrid->SetGridDimensions({ 3, 3 });


	UIMainGrid->WH = { 0.5f, 1.0f };
	UIMainGrid->XY = { 0.25f, 0.0f };

	UISubGrid_1		= &UI.CreateGrid(UIMainGrid, { 0, 0 });
	UISubGrid_1->SetGridDimensions({ 2, 2 });
	UISubGrid_1->WH = { 1.0f, 1.0f };
	UISubGrid_1->XY = { 0.0f, 0.0f };
	
	UISubGrid_2 = &UI.CreateGrid(UIMainGrid, {2, 2});
	UISubGrid_2->SetGridDimensions({ 5, 5 });
	UISubGrid_2->WH = { 1.0f, 1.0f };
	UISubGrid_2->XY = { 0.0f, 0.0f };
	
	auto& Btn = UI.CreateButton(UISubGrid_2, {0, 0});
}


/************************************************************************************************/


PlayState::~PlayState()
{
	// TODO: Make this not stoopid 
	Framework->Core->GetBlockMemory().free(this);
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	Event Remapped;
	if (DebugEventsInputMap.Map(evt, Remapped))
	{
		if (Remapped.InputSource == Event::Keyboard &&
			Remapped.Action		== Event::Pressed)
		{
			switch ((DEBUG_EVENTS)Remapped.mData1.mINT[0])
			{
			case DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA:
				UseDebugCamera	= !UseDebugCamera;
				break;
			case DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY:
				Framework->DrawDebug = !Framework->DrawDebug;
				break;
			default:
				break;
			}
		}
	}
	else
		FrameEvents.push_back(evt);

	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
	FrameCache.push_back(FrameSnapshot(&LocalGame, &FrameEvents, ++FrameID, Core->GetBlockMemory()));

	for (auto& evt : FrameEvents)
	{
		Event Remapped;
		if (Player1_Handler.Enabled && eventMap.Map(evt, Remapped))
			Player1_Handler.Handle(Remapped);

		if (Player2_Handler.Enabled)
			Player2_Handler.Handle(evt);
		if (DebugCameraInputMap.Map(evt, Remapped))
			OrbitCamera.EventHandler(Remapped);
	}

	FrameEvents.clear();

	// Check if any players Died
	for (auto& P : LocalGame.Players)
		if (LocalGame.IsCellDestroyed(P.XY))
			Framework->PopState();

	LocalGame.Update(dT, Core->GetTempMemory());

	if (Player1_Handler.Enabled)
		Player1_Handler.Update(dT);

	if (Player2_Handler.Enabled)
		Player2_Handler.Update(dT);


	float HorizontalMouseMovement	= float(Framework->MouseState.dPos[0]) / GetWindowWH(Framework->Core)[0];
	float VerticalMouseMovement		= float(Framework->MouseState.dPos[1]) / GetWindowWH(Framework->Core)[1];

	Framework->MouseState.Normalized_dPos = { HorizontalMouseMovement, VerticalMouseMovement };

	OrbitCamera.Yaw(Framework->MouseState.Normalized_dPos[0]);
	OrbitCamera.Pitch(Framework->MouseState.Normalized_dPos[1]);

	OrbitCamera.Update(dT);
	Puppet.Update(dT);

	QueueSoundUpdate(Dispatcher, &Sound);
	auto TransformTask = QueueTransformUpdateTask(Dispatcher);
	QueueCameraUpdate(Dispatcher, TransformTask);

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

	PVS	Drawables_Solid			(Core->GetTempMemory());
	PVS	Drawables_Transparent	(Core->GetTempMemory());

	ClearVertexBuffer(FrameGraph, VertexBuffer);
	ClearVertexBuffer(FrameGraph, TextBuffer);

	ClearBackBuffer		(FrameGraph, 0.0f);
	ClearDepthBuffer	(FrameGraph, DepthBuffer, 1.0f);


	FlexKit::DrawUI_Desc DrawDesk
	{
		&FrameGraph, 
		Targets.RenderTarget,
		VertexBuffer, 
		TextBuffer, 
		ConstantBuffer
	};


#ifndef DEBUGCAMERA
#if 1
	DrawGameGrid_Debug(
		dt,
		GetWindowAspectRatio(Core),
		LocalGame,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		Core->GetTempMemory()
	);
#else
	DrawGame(
		dt,
		GetWindowAspectRatio(Core),
		LocalGame,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer,
		OrbitCamera,
		Core->GetTempMemory());
#endif
#else
	PVS Drawables{ Core->GetTempMemory() };
	PVS TransparentDrawables{ Core->GetTempMemory() };

	CameraHandle ActiveCamera = UseDebugCamera ? (CameraHandle)OrbitCamera : (CameraHandle)Player1_Handler.GameCamera;

	GetGraphicScenePVS(Scene, ActiveCamera, &Drawables, &TransparentDrawables);


	if(Framework->DrawDebug)
	{
		DrawGameGrid_Debug(
			dt,
			GetWindowAspectRatio(Core),
			LocalGame,
			FrameGraph,
			ConstantBuffer,
			VertexBuffer,
			GetCurrentBackBuffer(&Core->Window),
			Core->GetTempMemory()
		);
	}

	DrawGame(
		dt,
		GetWindowAspectRatio(Core),
		LocalGame,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer,
		ActiveCamera,
		CharacterModel,
		Core->GetTempMemory());


	/*
	Render->DefaultRender(
		Drawables,
		ActiveCamera,
		Targets,
		FrameGraph,
		Core->GetTempMemory());
	*/
#endif
	
	//UI.Draw(DrawDesk, Core->GetTempMemory());


	return true;
}


/************************************************************************************************/


bool PlayState::PostDrawUpdate(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	if (Framework->DrawDebugStats)
		Framework->DrawDebugHUD(dT, TextBuffer, Graph);

	PresentBackBuffer(Graph, &Core->Window);
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
