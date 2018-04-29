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

/*
TODO's
*/


#ifndef GAMEPLAY_H_INCLUDED
#define GAMEPLAY_H_INCLUDED

#include "..\coreutilities\containers.h"
#include "..\coreutilities\Components.h"
#include "..\coreutilities\Events.h"
#include "..\coreutilities\MathUtils.h"
#include "..\graphicsutilities\FrameGraph.h"

using FlexKit::GameFramework;


typedef size_t Player_Handle;
typedef size_t GridObject_Handle;


/************************************************************************************************/


class GridPlayer :
	public FlexKit::iEventReceiver,
	public FlexKit::iUpdatable
{
public:
	GridPlayer()
	{
	}

	void Update(const double dt) override
	{
	}

	// NO INPUT HANDLING IN HERE!
	void Handle(const Event& evt) override
	{
	}


	enum PlayerState
	{
		PS_Moving,
		PS_Idle,
	}State = PS_Idle;

	FlexKit::int2	XY		= {1, 1};
	FlexKit::float2 Offset	= {0.f, 0.f};
};


/************************************************************************************************/


struct GridObject
{
	FlexKit::int2 XY = {0, 0};
};

typedef FlexKit::uint2 GridID_t;


class iGridTask
{
public:
	iGridTask() {}

	virtual ~iGridTask() {}

	virtual void Update(const double dt)	{}
	virtual bool Complete()					{ return true; }
};


/************************************************************************************************/


class GameGrid
{
public:
	GameGrid(FlexKit::iAllocator* memory) :
		Memory { memory },
		Players{ memory },
		Objects{ memory },
		Tasks  { memory },
		Grid   { memory },
		WH	   { 20, 20 }
	{
		Grid.resize(WH.Product());

		for (auto& Cell : Grid)
			Cell = EState::Empty;
	}

	enum class EState
	{
		Empty,
		Player,
		Object,
		InUse
	};

	Player_Handle		CreatePlayer();
	GridObject_Handle	CreateGridObject();

	bool MovePlayer		(Player_Handle Player, GridID_t GridID);
	bool IsCellClear	(GridID_t GridID);
	void Update			(const double dt, iAllocator* Memory);
	bool MarkCell		(GridID_t CellID, EState State);

	FlexKit::uint2					WH;	// Width Height

	FlexKit::Vector<GridPlayer>		Players;
	FlexKit::Vector<GridObject>		Objects;
	FlexKit::Vector<iGridTask*>		Tasks;
	FlexKit::Vector<EState>			Grid;

	iAllocator* Memory;
};


/************************************************************************************************/


inline void DrawGameGrid(
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
	const size_t ColumnCount	= 20;
	const size_t RowCount		= 20;
	LineSegments Lines(TempMem);
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	for (size_t I = 0; I < RowCount; ++I)
		Lines.push_back({ {0, RStep  * I,1}, {1.0f, 1.0f, 1.0f}, { 1, RStep  * I, 1, 1 }, {1, 1, 1, 1} });

	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 0; I < ColumnCount; ++I)
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
				float4{1.0f}, AspectRatio));


	for (auto Object : Grid.Objects)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			RectangleShape(float2{ 
				Object.XY[0] * CStep, 
				Object.XY[1] * RStep }, 
				{ CStep , RStep }));
}


/************************************************************************************************/


enum class PLAYER_EVENTS
{
	PLAYER1_UP,
	PLAYER1_LEFT,
	PLAYER1_DOWN,
	PLAYER1_RIGHT,
	PLAYER1_UNKNOWN,
};


class LocalPlayerHandler :
	public FlexKit::iEventReceiver,
	public FlexKit::iUpdatable
{
public:
	LocalPlayerHandler(GameGrid& grid, iAllocator* memory) :
		Game		{ grid },
		Map			{ memory },
		InputState	{ false, false, false, false, -1 }
	{}

	void Handle(const Event& evt) override
	{
		if (evt.InputSource == Event::Keyboard)
		{
			Event ReMapped_Event;
			auto Res = Map(evt, ReMapped_Event);

			if (Res)
			{
				switch ((PLAYER_EVENTS)ReMapped_Event.mData1.mINT[0])
				{
				case PLAYER_EVENTS::PLAYER1_UP:
					InputState.UP		=	
						 (evt.Action == Event::Pressed) ? true : 
						((evt.Action == Event::Release) ? false : InputState.UP);
					break;
				case PLAYER_EVENTS::PLAYER1_LEFT:
					InputState.LEFT		= 
						 (evt.Action == Event::Pressed) ? true : 
						((evt.Action == Event::Release) ? false : InputState.LEFT);
					break;
				case PLAYER_EVENTS::PLAYER1_DOWN:
					InputState.DOWN		= 
						 (evt.Action == Event::Pressed) ? true : 
						((evt.Action == Event::Release) ? false : InputState.DOWN);
					break;
				case PLAYER_EVENTS::PLAYER1_RIGHT:
					InputState.RIGHT	= 
						 (evt.Action == Event::Pressed) ? true : 
 						((evt.Action == Event::Release) ? false : InputState.RIGHT);
					break;
				default:
					break;
				}

				if (evt.Action == Event::Pressed)
				{
					switch ((PLAYER_EVENTS)ReMapped_Event.mData1.mINT[0])
					{
					case PLAYER_EVENTS::PLAYER1_UP:
						InputState.PreferredDirection = (int)PLAYER_EVENTS::PLAYER1_UP;
						break;
					case PLAYER_EVENTS::PLAYER1_LEFT:
						InputState.PreferredDirection = (int)PLAYER_EVENTS::PLAYER1_LEFT;
						break;
					case PLAYER_EVENTS::PLAYER1_DOWN:
						InputState.PreferredDirection = (int)PLAYER_EVENTS::PLAYER1_DOWN;
						break;
					case PLAYER_EVENTS::PLAYER1_RIGHT:
						InputState.PreferredDirection = (int)PLAYER_EVENTS::PLAYER1_RIGHT;
						break;
					default:
						break;
					}
				}

				if (evt.Action == Event::Release)
					InputState.PreferredDirection = -1;
			}
		}
	}

	void SetActive(Player_Handle P)
	{
		Player	= P;
		Enabled = true;
	}

	void MovePlayer(FlexKit::int2 XY)
	{
		Game.MovePlayer(Player, XY);
	}

	void MoveUp()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{  0, -1 });
	}

	void MoveDown()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{  0,  1 });
	}

	void MoveLeft()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{ -1,  0 });
	}

	void MoveRight()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{  1,  0 });
	}

	bool Enabled = false;

	void Update(const double dt) override
	{

		if (InputState.PreferredDirection != -1)
		{
			switch ((int)InputState.PreferredDirection)
			{
				case (int)PLAYER_EVENTS::PLAYER1_UP:
					MoveUp();
					break;
				case (int)PLAYER_EVENTS::PLAYER1_LEFT:
					MoveLeft();
					break;
				case (int)PLAYER_EVENTS::PLAYER1_DOWN:
					MoveDown();
					break;
				case (int)PLAYER_EVENTS::PLAYER1_RIGHT:
					MoveRight();
					break;
			}
		}
		else
		{
			if (InputState.UP)
			{
				MoveUp();
			}
			else if (InputState.DOWN)
			{
				MoveDown();
			}
			else if (InputState.LEFT)
			{
				MoveLeft();
			}
			else if (InputState.RIGHT)
			{
				MoveRight();
			}
		}
	}

	struct
	{
		bool UP;
		bool DOWN;
		bool LEFT;
		bool RIGHT;

		int	PreferredDirection;// if -1 then a preferred direction is ignored

		operator bool()	{ return UP | DOWN | LEFT | RIGHT; }
	}InputState;


	InputMap		Map;
	Player_Handle	Player;
	GameGrid&		Game;
};


/************************************************************************************************/


class MovePlayerTask : 
	public iGridTask
{
public:
	explicit MovePlayerTask(
		FlexKit::int2	a, 
		FlexKit::int2	b,
		Player_Handle	player,
		float			Duration,
		GameGrid*		grid
			) :
			A			{a},
			B			{b},
			D			{Duration},
			Player		{player},
			complete	{false},
			T			{0.0f},
			Grid		{grid}
	{
		Grid->Players[Player].State = GridPlayer::PS_Moving;
		Grid->MarkCell(A, GameGrid::EState::InUse);

	}

	MovePlayerTask& operator = (MovePlayerTask& rhs) = delete;

	void Update(const double dt) override;

	bool Complete() { return complete; }

	float			T;
	float			D;

	FlexKit::int2	A, B;

	GameGrid*		Grid;
	Player_Handle	Player;
	bool			complete;
};


/************************************************************************************************/

#endif