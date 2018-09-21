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


/************************************************************************************************/


using FlexKit::Event;
using FlexKit::FrameGraph;
using FlexKit::GameFramework;

using FlexKit::iAllocator;
using FlexKit::ConstantBufferHandle;
using FlexKit::VertexBufferHandle;
using FlexKit::TextureHandle;

using FlexKit::int2;
using FlexKit::uint2;
using FlexKit::float2;
using FlexKit::float3;


/************************************************************************************************/


typedef size_t Player_Handle;
typedef size_t GridObject_Handle;
typedef size_t GridSpace_Handle;


/************************************************************************************************/


enum class EBombType
{
	Regular
};


enum PLAYER_FACING_DIRECTION : int
{
	PF_UP		= 0, 
	PF_RIGHT	= 1,
	PF_DOWN		= 2, 
	PF_LEFT		= 3,
	PF_COUNT	= 4,
	PF_UNKNOWN	= -1
};


class GridPlayer
{
public:
	GridPlayer()
	{
		BombSlots.push_back(EBombType::Regular);
		BombCounts.push_back(256);
	}


	void Update(const double dt)
	{
	}

	// NO INPUT HANDLING IN HERE!
	void Handle(const Event& evt)
	{
	}

	enum PlayerState
	{
		PS_Moving,
		PS_Idle,
	}State = PS_Idle;


	bool DecrementBombSlot(EBombType Type)
	{
		auto Res = FlexKit::IsXInSet(Type, BombSlots);
		if (!Res)
			return false;

		for (size_t I = 0; I < BombSlots.size(); ++I)
		{
			if (BombSlots[I] == Type && BombCounts[I])
			{
				--BombCounts[I];
				return true;
			}
		}

		return false;
	}


	FlexKit::int2	XY						= {1, 1};
	FlexKit::float2 Offset					= {0.f, 0.f};
	FlexKit::static_vector<EBombType, 4>	BombSlots;
	FlexKit::static_vector<size_t, 4>		BombCounts;

	int	FacingDirection = PLAYER_FACING_DIRECTION::PF_DOWN;
};


/************************************************************************************************/



typedef FlexKit::uint2	GridID_t;
typedef uint64_t		BombID_t;


struct GridObject
{
	FlexKit::int2 XY = { 0, 0 };
};

struct GridSpace
{
	FlexKit::int2 XY = { 0, 0 };
};


struct GridBomb
{
	GridID_t	XY			= { 0, 0 };
	EBombType	Type		= EBombType::Regular;
	float		T			= 0.0f;
	BombID_t	ID			= -1;
	int2		Direction	= { 0, 0 };
	float2		Offset		= { 0.0f, 0.0f };
};


/************************************************************************************************/


class iGameTask
{
public:
	iGameTask() : UpdatePriority{ 0 } {}

	int UpdatePriority;

	virtual ~iGameTask() {}

	virtual iGameTask* MakeCopy(FlexKit::iAllocator* Memory) const = 0;

	virtual void Update(const double dt)	{}
	virtual bool Complete()					{ return true; }
};


/************************************************************************************************/


enum class EState
{
	Empty,
	Player,
	Object,
	Destroyed,
	Bomb,
	InUse
};


/************************************************************************************************/


typedef FlexKit::Vector<GridPlayer> PlayerList;

class Game
{
public:
	Game(FlexKit::iAllocator* memory) :
		Bombs  { memory },
		Memory { memory },
		Players{ memory },
		Objects{ memory },
		Spaces { memory },
		Tasks  { memory },
		Grid   { memory },
		WH	   { 20, 20 }
	{
		if (!memory)
		{
			WH = { 0, 0 };
			return;
		}

		Grid.resize(WH.Product());

		for (auto& Cell : Grid)
			Cell = EState::Empty;
	}


	/************************************************************************************************/


	Game& operator = (Game& rhs)
	{
		Release();

		for (auto Task : rhs.Tasks)
		{
			auto Copy = Task->MakeCopy(Memory);
			Tasks.push_back(Copy);
		}

		Bombs	= rhs.Bombs;
		Players	= rhs.Players;
		Objects = rhs.Objects;
		Spaces	= rhs.Spaces;
		Grid	= rhs.Grid;
		WH		= rhs.WH;

		return *this;
	}


	/************************************************************************************************/


	Game& operator = (Game&& rhs)
	{
		Release();

		Bombs	= std::move(rhs.Bombs);
		Memory	= std::move(rhs.Memory);
		Players	= std::move(rhs.Players);
		Objects = std::move(rhs.Objects);
		Spaces	= std::move(rhs.Spaces);
		Grid	= std::move(rhs.Grid);
		WH		= std::move(rhs.WH);
		Tasks	= std::move(rhs.Tasks);

		return *this;
	}


	/************************************************************************************************/


	Game(Game&& rhs) :
		Bombs	{std::move(rhs.Bombs)},
		Memory	{std::move(rhs.Memory)},
		Players	{std::move(rhs.Players)},
		Objects {std::move(rhs.Objects)},
		Spaces	{std::move(rhs.Spaces)},
		Grid	{std::move(rhs.Grid)},
		WH		{std::move(rhs.WH)},
		Tasks	{std::move(rhs.Tasks)}
	{}


	/************************************************************************************************/


	void Release()
	{
		Players.Release();
		Bombs.Release();
		Objects.Release();
		Spaces.Release();
		Grid.Release();

		for (auto& T : Tasks)
			Memory->free(T);

		Tasks.Release();
	}


	/************************************************************************************************/


	Player_Handle		CreatePlayer	(GridID_t CellID);
	GridObject_Handle	CreateGridObject(GridID_t CellID);
	GridObject_Handle	CreateGridSpace (GridID_t CellID);
	void				CreateBomb		(EBombType Type, GridID_t CellID, BombID_t ID, Player_Handle PlayerID);

	bool MovePlayer		(Player_Handle Player, GridID_t GridID);
	bool MoveBomb		(GridID_t GridID, int2 Direction);

	bool IsCellClear		(GridID_t GridID);
	bool IsCellDestroyed	(GridID_t GridID);
	bool MarkCell			(GridID_t CellID, EState State);
	void Resize				(uint2 wh);
	bool RemoveBomb			(BombID_t ID);

	bool	GetBomb			(BombID_t ID, GridBomb& out);
	EState	GetCellState	(GridID_t GridPOS);

	bool	SetBomb	(BombID_t ID, const GridBomb& In);

	void	Update	(const double dt, FlexKit::iAllocator* Memory);


	/************************************************************************************************/
	// Returns Player Position Between {0 - Width, 0 - Height}
	// World Position can be produced by scaling by Cell Size
	float3 GetPlayerPosition(Player_Handle PlayerHandle)
	{
		auto& Player = Players[PlayerHandle];

		float3 Position = {
			(Player.XY[0] + Player.Offset.x),
			0.0,
			(Player.XY[1] + Player.Offset.y)};

		return Position;
	}


	/************************************************************************************************/


	void SetPlayerDirection(Player_Handle PlayerHandle, int Direction)
	{
		auto& Player = Players[PlayerHandle];

		Player.FacingDirection = Direction;
	}


	/************************************************************************************************/


	float2							Scale = { 50, 50 };
	FlexKit::uint2					WH;	// Width Height
	PlayerList						Players;
	FlexKit::Vector<GridBomb>		Bombs;
	FlexKit::Vector<GridObject>		Objects;
	FlexKit::Vector<GridSpace>		Spaces;
	FlexKit::Vector<iGameTask*>		Tasks;
	FlexKit::Vector<EState>			Grid;

	FlexKit::iAllocator* Memory;

	size_t GridID2Index(GridID_t ID)
	{
		return WH[0] * ID[1] + ID[0];
	}


	/************************************************************************************************/


	bool IDInGrid(GridID_t ID)
	{
		auto temp1 = (0 <= ID[0] && ID[0] < WH[0]);
		auto temp2 = (0 <= ID[1] && ID[1] < WH[1]);

		return (0 <= ID[0] && ID[0] < WH[0]) && ( 0 <= ID[1] && ID[1] < WH[1]);
	}
};


/************************************************************************************************/


void DrawGameGrid_Debug(
	double					dt,
	float					AspectRatio,
	Game&					Grid,
	FrameGraph&				FrameGraph,
	ConstantBufferHandle	ConstantBuffer,
	VertexBufferHandle		VertexBuffer,
	TextureHandle			RenderTarget,
	FlexKit::iAllocator*	TempMem);


/************************************************************************************************/



enum PLAYER_EVENTS : int64_t
{
	PLAYER_UP		= GetCRCGUID(PLAYER_UP),
	PLAYER_LEFT		= GetCRCGUID(PLAYER_LEFT),
	PLAYER_DOWN		= GetCRCGUID(PLAYER_DOWN),
	PLAYER_RIGHT	= GetCRCGUID(PLAYER_RIGHT),
	PLAYER_ACTION1	= GetCRCGUID(PLAYER_ACTION1),
	PLAYER_HOLD		= GetCRCGUID(PLAYER_HOLD),

	PLAYER_ROTATE_LEFT	= GetCRCGUID(PLAYER_ROTATE_LEFT),
	PLAYER_ROTATE_RIGHT = GetCRCGUID(PLAYER_ROTATE_RIGHT),

	DEBUG_PLAYER_UP			= GetCRCGUID(DEBUG_PLAYER_UP),
	DEBUG_PLAYER_LEFT		= GetCRCGUID(DEBUG_PLAYER_LEFT),
	DEBUG_PLAYER_DOWN		= GetCRCGUID(DEBUG_PLAYER_DOWN),
	DEBUG_PLAYER_RIGHT		= GetCRCGUID(DEBUG_PLAYER_RIGHT),
	DEBUG_PLAYER_ACTION1	= GetCRCGUID(DEBUG_PLAYER_ACTION1),
	DEBUG_PLAYER_HOLD		= GetCRCGUID(DEBUG_PLAYER_HOLD),

	PLAYER_UNKNOWN,
};


enum DEBUG_EVENTS : int64_t
{
	TOGGLE_DEBUG_CAMERA  = GetCRCGUID(TOGGLE_DEBUG_CAMERA),
	TOGGLE_DEBUG_OVERLAY = GetCRCGUID(TOGGLE_DEBUG_OVERLAY)
};


/************************************************************************************************/


class MovePlayerTask : 
	public iGameTask
{
public:
	MovePlayerTask(
		FlexKit::int2	a, 
		FlexKit::int2	b,
		Player_Handle	player,
		float			Duration,
		Game*			grid) :
			A			{a				},
			B			{b				},
			D			{Duration		},
			Player		{player			},
			complete	{false			},
			T			{0.0f			},
			Grid		{grid			}
	{
		UpdatePriority = 10;
		Grid->Players[Player].State = GridPlayer::PS_Moving;
		Grid->MarkCell(B, EState::InUse);
	}


	MovePlayerTask(const MovePlayerTask& Rhs) :
		T				{Rhs.T				},
		D				{Rhs.D				},
		A				{Rhs.A				},
		Grid			{Rhs.Grid			},
		Player			{Rhs.Player			},
		complete		{Rhs.complete		}
	{
		UpdatePriority = Rhs.UpdatePriority;
	}


	iGameTask* MakeCopy(iAllocator* Memory) const
	{
		return static_cast<iGameTask*>(&Memory->allocate<MovePlayerTask>(*this));
	}

	void Update(const double dt) override;

	bool Complete() { return complete; }

	float			T;
	float			D;

	FlexKit::int2	A, B;

	Game*			Grid;
	Player_Handle	Player;
	bool			complete;
};


/************************************************************************************************/


class RegularBombTask :
	public iGameTask
{
public:
	explicit RegularBombTask(BombID_t IN_Bomb, Game* IN_Grid) :
		Bomb		{ IN_Bomb	},
		T			{ 0.0f		},
		T2			{ 0.0f		},
		Completed	{ false		},
		Grid		{ IN_Grid	}
	{}


	RegularBombTask(const RegularBombTask& Rhs) :
		Bomb		{ Rhs.Bomb			},
		T			{ Rhs.T				},
		T2			{ Rhs.T2			},
		Completed	{ Rhs.Completed		},
		Grid		{ Rhs.Grid			}
	{}


	void Update(const double dt) override;
	bool Complete() { return Completed; }

	iGameTask* MakeCopy(iAllocator* Memory) const
	{
		return static_cast<iGameTask*>(&Memory->allocate<RegularBombTask>(*this));
	}

private:
	bool		Completed;
	float		T;
	float		T2;
	BombID_t	Bomb;
	Game*		Grid;
};





#endif