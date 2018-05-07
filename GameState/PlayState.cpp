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

#pragma comment(lib, "fmod64_vc.lib")

PlayState::PlayState(
		GameFramework* framework,
		FlexKit::FKApplication* IN_App) :
	App				{ IN_App },
	Sound			{ framework->Core->Threads },
	FrameworkState	{ framework },
	Render	(
				Framework->Core->GetTempMemory(), 
				Framework->Core->RenderSystem, 
				Framework->Core->Geometry),
	VertexBuffer	(Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 64, false)),
	ConstantBuffer	(Framework->Core->RenderSystem.CreateConstantBuffer(8096 * 2000, false)),
	Grid			{Framework->Core->GetBlockMemory()},
	Player1_Handler	{Grid, Framework->Core->GetBlockMemory() },
	Player2_Handler	{Grid, Framework->Core->GetBlockMemory() },
	GameInPlay		{true}
{
	DepthBuffer = (Framework->Core->RenderSystem.CreateDepthBuffer({ 1920, 1080 }, true));
	Framework->Core->RenderSystem.SetTag(DepthBuffer, GetCRCGUID(DEPTHBUFFER));

	Player1_Handler.SetActive(Grid.CreatePlayer({ 11, 11 }));
	Grid.CreateGridObject({10, 5});


	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_W,			PLAYER_EVENTS::PLAYER1_UP);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_A,			PLAYER_EVENTS::PLAYER1_LEFT);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_S,			PLAYER_EVENTS::PLAYER1_DOWN);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_D,			PLAYER_EVENTS::PLAYER1_RIGHT);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_LEFTSHIFT,	PLAYER_EVENTS::PLAYER1_HOLD);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_SPACE,		PLAYER_EVENTS::PLAYER1_ACTION1);
}


/************************************************************************************************/


PlayState::~PlayState()
{

	Framework->Core->RenderSystem.ReleaseVB(VertexBuffer);
	Framework->Core->RenderSystem.ReleaseCB(ConstantBuffer);

	Framework->Core->GetBlockMemory().free(this);
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	if (Player1_Handler.Enabled)
		Player1_Handler.Handle(evt);

	if (Player2_Handler.Enabled)
		Player2_Handler.Handle(evt);

	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Engine, double dT)	
{
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

	return false;
}


/************************************************************************************************/

bool PlayState::DebugDraw(EngineCore* Core, double dT)
{
	return true;
}


/************************************************************************************************/


bool PlayState::PreDrawUpdate(EngineCore* Core, double dT)
{
	return false;
}


/************************************************************************************************/


bool PlayState::Draw(EngineCore* Core, double dt, FrameGraph& FrameGraph)
{
	FrameGraph.Resources.AddDepthBuffer(DepthBuffer);
	WorldRender_Targets Targets = {
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer
	};

	PVS	Drawables_Solid(Core->GetTempMemory());
	PVS	Drawables_Transparent(Core->GetTempMemory());


	ClearBackBuffer		(FrameGraph, 0.0f);
	ClearVertexBuffer	(FrameGraph, VertexBuffer);

	DrawGameGrid(
		dt,
		GetWindowAspectRatio(Core),
		Grid,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		Core->GetTempMemory()
	);

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
