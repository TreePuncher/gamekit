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



PlayState::PlayState(GameFramework* framework) :
	FrameworkState	(framework),
	Render			(
		 Framework->Core->GetTempMemory(), 
		 Framework->Core->RenderSystem, 
		 Framework->Core->Geometry),
	VertexBuffer	(Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 4, false)),
	ConstantBuffer	(Framework->Core->RenderSystem.CreateConstantBuffer(8096 * 2000, false)),
	Grid			{Framework->Core->GetBlockMemory()},
	Player1_Handler	{Grid, Framework->Core->GetBlockMemory() },
	Player2_Handler	{Grid, Framework->Core->GetBlockMemory() }
{
	DepthBuffer = (Framework->Core->RenderSystem.CreateDepthBuffer({ 1920, 1080 }, true));
	Framework->Core->RenderSystem.SetTag(DepthBuffer, GetCRCGUID(DEPTHBUFFER));

	Player1_Handler.SetActive(Grid.CreatePlayer());
	Grid.CreateGridObject();

	const uint32_t UPEVENT = GetCRCGUID(PLAYER1_UP);

	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_W, (int32_t)PLAYER_EVENTS::PLAYER1_UP);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_A, (int32_t)PLAYER_EVENTS::PLAYER1_LEFT);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_S, (int32_t)PLAYER_EVENTS::PLAYER1_DOWN);
	Player1_Handler.Map.MapKeyToEvent(KEYCODES::KC_D, (int32_t)PLAYER_EVENTS::PLAYER1_RIGHT);
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
	if (Player1_Handler.Enabled)
		Player1_Handler.Update(dT);

	if (Player2_Handler.Enabled)
		Player2_Handler.Update(dT);

	Grid.Update(dT, Engine->GetTempMemory());

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


PlayState::~PlayState()
{

	Framework->Core->RenderSystem.ReleaseVB(VertexBuffer);
	Framework->Core->RenderSystem.ReleaseCB(ConstantBuffer);

	Framework->Core->GetBlockMemory().free(this);
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

	// Draw Grid
	const size_t ColumnCount	= 20;
	const size_t RowCount		= 20;
	LineSegments Lines(Core->GetTempMemory());
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	for (size_t I = 0; I < RowCount; ++I)
		Lines.push_back({ {0, RStep  * I,1}, {1.0f, 1.0f, 1.0f}, { 1, RStep  * I, 1, 1 }, {1, 1, 1, 1} });

	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 0; I < ColumnCount; ++I)
		Lines.push_back({ { CStep  * I, 0, 0 },{ 1.0f, 1.0f, 1.0f },{ CStep  * I, 1, 0 },{ 1, 1, 1, 1 } });


	auto Aspect = GetWindowAspectRatio(Core);

	DrawShapes(EPIPELINESTATES::DRAW_LINE_PSO, FrameGraph, VertexBuffer, ConstantBuffer, GetCurrentBackBuffer(&Core->Window), Core->GetTempMemory(),
		LineShape(Lines));


	for (auto Player : Grid.Players)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, GetCurrentBackBuffer(&Core->Window), Core ->GetTempMemory(),
			CircleShape(
				float2{	
					CStep / 2 + Player.XY[0] * CStep + Player.Offset.x * CStep,
					RStep / 2 + Player.XY[1] * RStep + Player.Offset.y * RStep },
				min(
					(CStep / 2.0f) / Aspect,
					(RStep / 2.0f)),
				float4{1.0f}, 1.33f));


	for (auto Object : Grid.Objects)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, GetCurrentBackBuffer(&Core->Window), Core->GetTempMemory(),
			RectangleShape(float2{ 
				Object.XY[0] * CStep, 
				Object.XY[1] * RStep }, 
				{ CStep , RStep }));

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
