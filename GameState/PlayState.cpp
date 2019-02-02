#include "PlayState.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\PipelineState.h"
#include "..\graphicsutilities\TextureUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\PipelineState.h"


#define DEBUGCAMERA


/************************************************************************************************/


PlayState::PlayState(
		GameFramework*			IN_Framework,
		BaseState*				IN_base) : 
	FrameworkState		{ IN_Framework						},

	ui					{ FlexKit::GuiSystem_Desc{}, framework->core->GetBlockMemory()	},
	uiMainGrid			{ nullptr														},
	uiSubGrid_1			{ nullptr														},
	uiSubGrid_2			{ nullptr														},

	frameID				{ 0 },
	sound				{ framework->core->Threads, framework->core->GetBlockMemory()	},
	render				{ IN_base->render												},
	base				{ *IN_base														},
	eventMap			{ framework->core->GetBlockMemory()								},
	debugCameraInputMap	{ framework->core->GetBlockMemory()								},
	debugEventsInputMap	{ framework->core->GetBlockMemory()								},

	scene				{
							framework->core->RenderSystem,
							framework->core->GetBlockMemory(),
							framework->core->GetTempMemory()
						}
{
	AddResourceFile("CharacterBase.gameres");
	characterModel = FlexKit::GetMesh(GetRenderSystem(), "Flower");

	eventMap.MapKeyToEvent(KEYCODES::KC_W,			PLAYER_EVENTS::PLAYER_UP			);
	eventMap.MapKeyToEvent(KEYCODES::KC_A,			PLAYER_EVENTS::PLAYER_LEFT			);
	eventMap.MapKeyToEvent(KEYCODES::KC_S,			PLAYER_EVENTS::PLAYER_DOWN			);
	eventMap.MapKeyToEvent(KEYCODES::KC_D,			PLAYER_EVENTS::PLAYER_RIGHT			);
	eventMap.MapKeyToEvent(KEYCODES::KC_Q,			PLAYER_EVENTS::PLAYER_ROTATE_LEFT	);
	eventMap.MapKeyToEvent(KEYCODES::KC_E,			PLAYER_EVENTS::PLAYER_ROTATE_RIGHT	);
	eventMap.MapKeyToEvent(KEYCODES::KC_LEFTSHIFT,	PLAYER_EVENTS::PLAYER_HOLD			);
	eventMap.MapKeyToEvent(KEYCODES::KC_SPACE,		PLAYER_EVENTS::PLAYER_ACTION1		);

	// Debug Orbit Camera
	debugCameraInputMap.MapKeyToEvent(KEYCODES::KC_I, PLAYER_EVENTS::DEBUG_PLAYER_UP	);
	debugCameraInputMap.MapKeyToEvent(KEYCODES::KC_J, PLAYER_EVENTS::DEBUG_PLAYER_LEFT	);
	debugCameraInputMap.MapKeyToEvent(KEYCODES::KC_K, PLAYER_EVENTS::DEBUG_PLAYER_DOWN	);
	debugCameraInputMap.MapKeyToEvent(KEYCODES::KC_L, PLAYER_EVENTS::DEBUG_PLAYER_RIGHT	);

	// Debug Events, are not stored in Frame cache
	debugEventsInputMap.MapKeyToEvent(KEYCODES::KC_C, DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA		);
	debugEventsInputMap.MapKeyToEvent(KEYCODES::KC_X, DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY	);

	framework->core->RenderSystem.PipelineStates.RegisterPSOLoader	(DRAW_SPRITE_TEXT_PSO, { nullptr, LoadSpriteTextPSO });
	framework->core->RenderSystem.PipelineStates.QueuePSOLoad		(DRAW_SPRITE_TEXT_PSO, framework->core->GetBlockMemory());

	uiMainGrid	= &ui.CreateGrid(nullptr);
	uiMainGrid->SetGridDimensions({ 3, 3 });

	uiMainGrid->WH = { 0.5f, 1.0f  };
	uiMainGrid->XY = { 0.25f, 0.0f };

	uiSubGrid_1		= &ui.CreateGrid(uiMainGrid, { 0, 0 });
	uiSubGrid_1->SetGridDimensions({ 2, 2 });
	uiSubGrid_1->WH = { 1.0f, 1.0f };
	uiSubGrid_1->XY = { 0.0f, 0.0f };
	
	uiSubGrid_2 = &ui.CreateGrid(uiMainGrid, {2, 2});
	uiSubGrid_2->SetGridDimensions({ 5, 5 });
	uiSubGrid_2->WH = { 1.0f, 1.0f };
	uiSubGrid_2->XY = { 0.0f, 0.0f };
	
	auto& Btn = ui.CreateButton(uiSubGrid_2, {0, 0});
}


/************************************************************************************************/


PlayState::~PlayState()
{
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
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
	return true;
}


/************************************************************************************************/


bool PlayState::PostDrawUpdate(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	return true;
}


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