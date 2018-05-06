/**********************************************************************

Copyright (c) 2018 Robert May

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

#include "Gameplay.h"



/************************************************************************************************/


Player_Handle GameGrid::CreatePlayer(GridID_t CellID)
{
	Players.push_back(GridPlayer());
	Players.back().XY = CellID;

	MarkCell(CellID, EState::Player);

	return Players.size() - 1;
}


/************************************************************************************************/


GridObject_Handle GameGrid::CreateGridObject(GridID_t CellID)
{
	Objects.push_back(GridObject());

	Objects.back().XY = CellID;
	MarkCell(CellID, EState::Object);

	return Objects.size() - 1;
}


/************************************************************************************************/


void GameGrid::CreateBomb(EBombType Type, GridID_t CellID, BombID_t ID)
{
	Bombs.push_back({ CellID, EBombType::Regular, 0, ID });
	FK_ASSERT(false, "Un-Implemented!");
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
		0.25f,
		this);

	Tasks.push_back(Task);

	return true;
}


/************************************************************************************************/


bool GameGrid::IsCellClear(GridID_t GridID)
{
	/*
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
	*/

	if (GridID[0] >= WH[0] || GridID[0] < 0)
		return false;

	if (GridID[1] >= WH[1] || GridID[1] < 0)
		return false;


	size_t Idx = WH[0] * GridID[1] + GridID[0];

	return (Grid[Idx] == EState::Empty);
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


bool GameGrid::MarkCell(GridID_t CellID, EState State)
{
	size_t Idx = WH[0] * CellID[1] + CellID[0];

	//if (!IsCellClear(CellID))
	//	return false;

	Grid[Idx] = State;

	return true;
}


/************************************************************************************************/

// Certain State will be lost
void GameGrid::Resize(uint2 wh)
{
	WH = wh;
	Grid.resize(WH.Product());

	for (auto& Cell : Grid)
		Cell = EState::Empty;

	for (auto Obj : Objects)
		MarkCell(Obj.XY, GameGrid::EState::Object);

	for (auto Player : Players)
		MarkCell(Player.XY, GameGrid::EState::Player);
}


/************************************************************************************************/


void MovePlayerTask::Update(const double dt)
{
	T += dt / D;

	if (T < (1.0f - 1.0f/60.f))
	{
		int2 temp	= B - A;
		float2 C	= { 
			(float)temp[0], 
			(float)temp[1] };

		Grid->Players[Player].Offset = { C * T };
	}
	else
	{
		Grid->MarkCell(A, GameGrid::EState::Empty);
		Grid->MarkCell(B, GameGrid::EState::Player);

		Grid->Players[Player].Offset = { 0.f, 0.f };
		Grid->Players[Player].XY	 = B;
		Grid->Players[Player].State	 = GridPlayer::PS_Idle;
		complete = true;
	}
}


/************************************************************************************************/


void RegularBombTask::Update(const double dt)
{

}


/************************************************************************************************/


void DrawGameGrid(
	double					dt,
	float					AspectRatio,
	GameGrid&				Grid,
	FrameGraph&				FrameGraph,
	ConstantBufferHandle	ConstantBuffer,
	VertexBufferHandle		VertexBuffer,
	TextureHandle			RenderTarget,
	iAllocator*				TempMem
	)
{
	const size_t ColumnCount	= Grid.WH[1];
	const size_t RowCount		= Grid.WH[0];

	LineSegments Lines(TempMem);
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	for (size_t I = 1; I < RowCount; ++I)
		Lines.push_back({ {0, RStep  * I,1}, {1.0f, 1.0f, 1.0f}, { 1, RStep  * I, 1, 1 }, {1, 1, 1, 1} });


	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 1; I < ColumnCount; ++I)
		Lines.push_back({ { CStep  * I, 0, 0 },{ 1.0f, 1.0f, 1.0f },{ CStep  * I, 1, 0 },{ 1, 1, 1, 1 } });


	DrawShapes(EPIPELINESTATES::DRAW_LINE_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
		LineShape(Lines));


	for (auto Player : Grid.Players)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			CircleShape(
				float2{	
					CStep / 2 + Player.XY[0] * CStep + Player.Offset.x * CStep,
					RStep / 2 + Player.XY[1] * RStep + Player.Offset.y * RStep },
				min(
					(CStep / 2.0f) / AspectRatio,
					(RStep / 2.0f)),
				float4{1.0f, 1.0f, 1.0f, 1.0f}, AspectRatio));


	for (auto Bombs : Grid.Bombs)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			CircleShape(
				float2{
					CStep / 2 + Bombs.XY[0] * CStep,
					RStep / 2 + Bombs.XY[1] * RStep},
					min(
						(CStep / 2.0f) / AspectRatio,
						(RStep / 2.0f)),
				float4{ 0.0f, 1.0f, 0.0f, 0.0f }, AspectRatio, 4));


	for (auto Object : Grid.Objects)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			RectangleShape(float2{ 
				Object.XY[0] * CStep, 
				Object.XY[1] * RStep }, 
				{ CStep , RStep },
				{0.5f, 0.5f, 0.5f, 1.0f}));
}


/************************************************************************************************/