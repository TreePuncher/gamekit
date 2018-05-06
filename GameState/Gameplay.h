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


enum PlayerDirection
{
	UP, DOWN, LEFT, RIGHT
};


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


	FlexKit::int2	XY				= {1, 1};
	FlexKit::float2 Offset			= {0.f, 0.f};
	PlayerDirection	FacingDirection = PlayerDirection::DOWN;
};


/************************************************************************************************/


typedef FlexKit::uint2 GridID_t;


struct GridObject
{
	FlexKit::int2 XY = {0, 0};
};

enum class EBombType
{
	Regular
};

typedef uint64_t BombID_t;

struct GridBomb
{
	GridID_t	XY		= { 0, 0 };
	EBombType	Type	= EBombType::Regular;
	float		T		= 0.0f;
	BombID_t	ID		= -1;
};



/************************************************************************************************/


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
		Bombs  { memory },
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
		Destroyed,
		InUse
	};

	Player_Handle		CreatePlayer	(GridID_t CellID);
	GridObject_Handle	CreateGridObject(GridID_t CellID);
	void				CreateBomb		(EBombType Type, GridID_t CellID, BombID_t ID);

	bool MovePlayer		(Player_Handle Player, GridID_t GridID);
	bool IsCellClear	(GridID_t GridID);
	void Update			(const double dt, iAllocator* Memory);
	bool MarkCell		(GridID_t CellID, EState State);
	void Resize			(uint2 wh);



	FlexKit::uint2					WH;	// Width Height

	FlexKit::Vector<GridPlayer>		Players;
	FlexKit::Vector<GridBomb>		Bombs;
	FlexKit::Vector<GridObject>		Objects;
	FlexKit::Vector<iGridTask*>		Tasks;
	FlexKit::Vector<EState>			Grid;

	iAllocator* Memory;
};


/************************************************************************************************/


void DrawGameGrid(
	double					dt,
	float					AspectRatio,
	GameGrid&				Grid,
	FrameGraph&				FrameGraph,
	ConstantBufferHandle	ConstantBuffer,
	VertexBufferHandle		VertexBuffer,
	TextureHandle			RenderTarget,
	iAllocator*				TempMem);


/************************************************************************************************/


enum PLAYER_EVENTS : int64_t
{
	PLAYER1_UP		= GetCRCGUID(PLAYER1_UP),
	PLAYER1_LEFT	= GetCRCGUID(PLAYER1_LEFT),
	PLAYER1_DOWN	= GetCRCGUID(PLAYER1_DOWN),
	PLAYER1_RIGHT	= GetCRCGUID(PLAYER1_RIGHT),
	PLAYER1_ACTION1 = GetCRCGUID(PLAYER1_ACTION1),
	PLAYER1_HOLD	= GetCRCGUID(PLAYER1_HOLD),
	PLAYER1_UNKNOWN,
};


/************************************************************************************************/


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
				case PLAYER_EVENTS::PLAYER1_ACTION1:
				{
					if ((evt.Action == Event::Release))
					{
						int2 GridPOS = Game.Players[0].XY;
						switch (Game.Players[0].FacingDirection)
						{
							case UP:
								GridPOS += int2{  0, -1  };
								break;
							case DOWN: 
								GridPOS += int2{  0,  1  };
								break;
							case LEFT:
								GridPOS += int2{ -1,  0  };
								break;
							case RIGHT:
								GridPOS += int2{  1,  0  };
								break;
							default:
								FK_ASSERT(0, "!!!!!");
						}

						if (!Game.IsCellClear(GridPOS))
							return;

						size_t ID = chrono::high_resolution_clock::now().time_since_epoch().count();
						Game.CreateBomb(EBombType::Regular, GridPOS, ID);
						break;
					}
				}	break;
				default:
					break;
				}


				if (evt.Action == Event::Release)
				{
					std::cout << "Key Released!\n";
					switch ((PLAYER_EVENTS)ReMapped_Event.mData1.mINT[0])
					{
					case PLAYER_EVENTS::PLAYER1_UP:
					case PLAYER_EVENTS::PLAYER1_DOWN:
					case PLAYER_EVENTS::PLAYER1_LEFT:
					case PLAYER_EVENTS::PLAYER1_RIGHT:
						InputState.PreferredDirection = -1;
						break;
					default:
						break;
					}
				}


				if (evt.Action == Event::Pressed)
				{
					switch ((PLAYER_EVENTS)ReMapped_Event.mData1.mINT[0])
					{
					case PLAYER_EVENTS::PLAYER1_UP:
						InputState.PreferredDirection = (int64_t)PLAYER_EVENTS::PLAYER1_UP;			break;
					case PLAYER_EVENTS::PLAYER1_LEFT:
						InputState.PreferredDirection = (int64_t)PLAYER_EVENTS::PLAYER1_LEFT;		break;
					case PLAYER_EVENTS::PLAYER1_DOWN:
						InputState.PreferredDirection = (int64_t)PLAYER_EVENTS::PLAYER1_DOWN;		break;
					case PLAYER_EVENTS::PLAYER1_RIGHT:
						InputState.PreferredDirection = (int64_t)PLAYER_EVENTS::PLAYER1_RIGHT;		break;
					default:
						break;
					}
				}
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
			switch ((int64_t)InputState.PreferredDirection)
			{
				case (int64_t)PLAYER_EVENTS::PLAYER1_UP:
					MoveUp();
					break;
				case (int64_t)PLAYER_EVENTS::PLAYER1_LEFT:
					MoveLeft();
					break;
				case (int64_t)PLAYER_EVENTS::PLAYER1_DOWN:
					MoveDown();
					break;
				case (int64_t)PLAYER_EVENTS::PLAYER1_RIGHT:
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

		int64_t	PreferredDirection;// if -1 then a preferred direction is ignored

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


class RegularBombTask :
	public iGridTask
{
public:
	explicit RegularBombTask(BombID_t IN_Bomb, GameGrid* IN_Grid) :
		Bomb		{ IN_Bomb	},
		T			{ 0.0f		},
		Completed	{ false		},
		Grid		{ IN_Grid	}
	{}


	void Update(const double dt) override;
	bool Complete() { return Completed; }


private:
	bool		Completed;
	float		T;
	BombID_t	Bomb;
	GameGrid*	Grid;
};


/************************************************************************************************/

#endif