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



/************************************************************************************************/




Player_Handle GameGrid::CreatePlayer()
{
	Players.push_back(GridPlayer());

	return Players.size() - 1;
}


/************************************************************************************************/


GridObject_Handle GameGrid::CreateGridObject()
{
	Objects.push_back(GridObject());

	return Objects.size() - 1;
}


/************************************************************************************************/


bool GameGrid::MovePlayer(Player_Handle Player, GridID_t GridID)
{
	if (Players[Player].State != GridPlayer::PS_Idle)
		return false;

	if (!IsCellClear(GridID))
		return false;

	auto OldPOS = Players[Player].XY;

	auto* Task = &Memory->allocate_aligned<MovePlayerTask>(
		OldPOS,
		GridID,
		Player,
		this);

	Tasks.push_back(Task);

	return true;
}


/************************************************************************************************/


bool GameGrid::IsCellClear(GridID_t GridID)
{
	for (auto Obj : Objects)
	{
		if (Obj.XY == GridID)
			return false;
	}

	for (auto P : Players)
	{
		if (P.XY == GridID)
			return false;
	}

	return true;
}


/************************************************************************************************/


void GameGrid::Update(const double dt, iAllocator* TempMemory)
{
	auto RemoveList = FlexKit::Vector<iGridTask**>(TempMemory);

	for (auto& Task : Tasks)
	{
		Task->Update(dt);

		if (Task->Complete())
			RemoveList.push_back(&Task);
	}

	for (auto* Task : RemoveList)
	{	// Ugh!
		(*Task)->~iGridTask();
		Memory->free(*Task);
		Tasks.remove_stable(Task);
	}
}


/************************************************************************************************/


void MovePlayerTask::Update(const double dt)
{
	if (T < 1.0f)
	{
		int2 temp	= B - A;
		float2 C	= { 
			(float)temp[0], 
			(float)temp[1] };

		Grid->Players[Player].Offset = { C * T };
	}
	else
	{
		Grid->Players[Player].Offset = { 0.f, 0.f };
		Grid->Players[Player].XY	 = B;
		Grid->Players[Player].State	 = GridPlayer::PS_Idle;
		complete = true;
	}

	T += dt / D;
}


/************************************************************************************************/


PlayState::PlayState(GameFramework* framework) :
	FrameworkState	(framework),
	Input			(Framework),
	TPC				(Framework, Input, Framework->Core->Cameras),
	OrbitCameras	(Framework, Input),
	Render			(
		 Framework->Core->GetTempMemory(), 
		 Framework->Core->RenderSystem, 
		 Framework->Core->Geometry),
	Scene		(
		 Framework->Core->RenderSystem, 
		&Framework->Core->Assets, 
		 Framework->Core->Nodes, 
		 Framework->Core->Geometry, 
		 Framework->Core->GetBlockMemory(), 
		 Framework->Core->GetTempMemory()),
	Drawables		(&Scene, Framework->Core->Nodes),
	Lights			(&Scene, Framework->Core->Nodes),
	Physics			(&Framework->Core->Physics, Framework->Core->Nodes, Framework->Core->GetBlockMemory()),
	VertexBuffer	(Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 4, false)),
	ConstantBuffer	(Framework->Core->RenderSystem.CreateConstantBuffer(8096 * 2000, false)),
	Grid			{Framework->Core->GetBlockMemory()},
	Player1_Handler	{Grid},
	Player2_Handler	{Grid}
{
	Framework->ActivePhysicsScene	= &Physics;
	Framework->ActiveScene			= &Scene;


	bool res = LoadScene(
		Framework->Core->RenderSystem, 
		Framework->Core->Nodes, 
		&Framework->Core->Assets, 
		&Framework->Core->Geometry, 
		201, 
		&Scene, 
		Framework->Core->GetTempMemory());

	DepthBuffer = (Framework->Core->RenderSystem.CreateDepthBuffer({ 1920, 1080 }, true));
	Framework->Core->RenderSystem.SetTag(DepthBuffer, GetCRCGUID(DEPTHBUFFER));

	Grid.CreatePlayer();
	Grid.CreateGridObject();
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	if (evt.InputSource == Event::Keyboard)
	{
		switch (evt.Action)
		{
		case Event::Pressed:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				Input.KeyState.Shield   = true;
				break;
			case KC_W:
				Player1_Handler.MoveUp();
				Input.KeyState.Forward  = true;
				break;
			case KC_S:
				Player1_Handler.MoveDown();
				Input.KeyState.Backward = true;
				break;
			case KC_A:
				Player1_Handler.MoveLeft();
				Input.KeyState.Left     = true;
				break;
			case KC_D:
				Player1_Handler.MoveRight();
				Input.KeyState.Right    = true;
				break;
			}
		}	break;
		case Event::Release:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_R:
				//this->Framework->Core->RenderSystem.QueuePSOLoad(FORWARDDRAW);
				break;
			case KC_E:
				Input.KeyState.Shield   = false;
				break;
			case KC_W:
				Input.KeyState.Forward  = false;
				break;
			case KC_S:
				Input.KeyState.Backward = false;
				break;
			case KC_A:
				Input.KeyState.Left     = false;
				break;
			case KC_D: 
				Input.KeyState.Right    = false;
				break;
			}
		}	break;
		}
	}

	if (Player1_Handler.Enabled)
		Player1_Handler.Handle(evt);

	if (Player2_Handler.Enabled)
		Player2_Handler.Handle(evt);

	return true;
}


/************************************************************************************************/

float3 CameraPOS = {8000,1000,8000};

bool PlayState::Update(EngineCore* Engine, double dT)
{
	Input.Update(dT, Framework->MouseState, Framework );
	
	if (Player1_Handler.Enabled)
		Player1_Handler.Update(dT);

	if (Player2_Handler.Enabled)
		Player2_Handler.Update(dT);

	Grid.Update(dT, Engine->GetTempMemory());

	OrbitCameras.Update(dT);
	Physics.UpdateSystem(dT);
	TPC.Update(dT);

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
	Physics.UpdateSystem_PreDraw(dT);

	return false;
}


/************************************************************************************************/


PlayState::~PlayState()
{
	Physics.Release();

	Framework->Core->RenderSystem.ReleaseVB(VertexBuffer);
	Framework->Core->RenderSystem.ReleaseCB(ConstantBuffer);

	ReleaseGraphicScene(&Scene);
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


	DrawShapes(EPIPELINESTATES::DRAW_LINE_PSO, FrameGraph, VertexBuffer, ConstantBuffer, GetCurrentBackBuffer(&Core->Window), Core->GetTempMemory(),
		LineShape(Lines));


	for (auto Player : Grid.Players)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, GetCurrentBackBuffer(&Core->Window), Core ->GetTempMemory(),
			CircleShape(
				float2{	
					CStep / 2 + Player.XY[0] * CStep + Player.Offset.x * CStep,
					RStep / 2 + Player.XY[1] * RStep + Player.Offset.y * RStep },
				min(
					CStep / 2.0f, 
					RStep / 2.0f), 
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
