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
using FlexKit::CameraHandle;
using FlexKit::ConstantBufferHandle;
using FlexKit::VertexBufferHandle;
using FlexKit::TextureHandle;
using FlexKit::static_vector;

using FlexKit::CameraBehavior;

using FlexKit::int2;
using FlexKit::uint2;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::float4;
using FlexKit::Quaternion;


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


/************************************************************************************************/


class GameCameraController
{
public:
	GameCameraController() :
		CameraOffset{ 0.0f, 10.0f, 5.0f },
		FacingDirection{ PF_UP },
		YawOffset{ 0.0f },
		Pitch{ -45.0f }
	{
		Camera.SetCameraNode(FlexKit::GetZeroedNode());
	}

	void SetAngle(float Degree)
	{

	}


	void Rotate_Clockwise()
	{
		FacingDirection = (FacingDirection + 1) % PLAYER_FACING_DIRECTION::PF_COUNT;
	}


	void Rotate_CounterClockwise()
	{
		FacingDirection = (PLAYER_FACING_DIRECTION::PF_COUNT + FacingDirection - 1) % PLAYER_FACING_DIRECTION::PF_COUNT;
		int x = 0;
	}

	void Track()
	{

	}


	void Update(float dt, const float3 PlayerPosition)
	{
		float Yaw = FacingDirection * -90.0f + YawOffset;
		auto RotatedOffset = Quaternion(0, 360 - Yaw, 0) * CameraOffset;
		SetOrientation(Camera.GetCameraNode(), Quaternion(0, Yaw, 0) * Quaternion(Pitch, 0, 0));
		SetPositionW(Camera.GetCameraNode(), PlayerPosition + RotatedOffset);
	}


	operator CameraHandle ()
	{
		return Camera;
	}

	int							FacingDirection;
	float						MovementFactor;
	float						YawOffset;
	float						Pitch;
	float3						CameraOffset;
	CameraBehavior				Camera;
};


/************************************************************************************************/


class LocalPlayerHandler
{
public:
	LocalPlayerHandler(Game& grid, iAllocator* memory) :
		Game{ grid },
		//Map			{ memory },
		InputState{ false, false, false, false, PF_UNKNOWN }
	{
		FlexKit::SetCameraFOV(GameCamera, float(FlexKit::pi) / 3.0f);
		FlexKit::Yaw(FlexKit::GetCameraNode(GameCamera), float(FlexKit::pi));
		FlexKit::SetPositionW(FlexKit::GetCameraNode(GameCamera), { 0, 2, -5 });
	}


	void Handle(const Event& evt)
	{
		CurrentFrameEvents.push_back(evt);
	}

	void ProcessEvents()
	{
		for (auto& evt : CurrentFrameEvents)
		{
			if (evt.InputSource == Event::Keyboard)
			{
				switch ((PLAYER_EVENTS)evt.mData1.mINT[0])
				{
				case PLAYER_EVENTS::PLAYER_UP:
					InputState.UP =
						(evt.Action == Event::Pressed) ? true :
						((evt.Action == Event::Release) ? false : InputState.UP);
					break;
				case PLAYER_EVENTS::PLAYER_LEFT:
					InputState.LEFT =
						(evt.Action == Event::Pressed) ? true :
						((evt.Action == Event::Release) ? false : InputState.LEFT);
					break;
				case PLAYER_EVENTS::PLAYER_DOWN:
					InputState.DOWN =
						(evt.Action == Event::Pressed) ? true :
						((evt.Action == Event::Release) ? false : InputState.DOWN);
					break;
				case PLAYER_EVENTS::PLAYER_RIGHT:
					InputState.RIGHT =
						(evt.Action == Event::Pressed) ? true :
						((evt.Action == Event::Release) ? false : InputState.RIGHT);
					break;
				case PLAYER_EVENTS::PLAYER_ROTATE_LEFT:
					if (evt.Action == Event::Pressed)
					{
						GameCamera.Rotate_CounterClockwise();
						Game.SetPlayerDirection(Player, GameCamera.FacingDirection);
					}
					break;
				case PLAYER_EVENTS::PLAYER_ROTATE_RIGHT:
					if (evt.Action == Event::Pressed) {
						GameCamera.Rotate_Clockwise();
						Game.SetPlayerDirection(Player, GameCamera.FacingDirection);
					}
					break;
				case PLAYER_EVENTS::PLAYER_ACTION1:
				{
					if (Game.Players[Player].State != GridPlayer::PS_Idle)
						return;

					if ((evt.Action == Event::Release))
					{
						int2 Offset = { 0, 0 };

						switch (Game.Players[Player].FacingDirection)
						{
						case PF_UP:
							Offset += int2{ 0, -1 };
							break;
						case PF_DOWN:
							Offset += int2{ 0,  1 };
							break;
						case PF_LEFT:
							Offset += int2{ -1,  0 };
							break;
						case PF_RIGHT:
							Offset += int2{ 1,  0 };
							break;
						default:
							FK_ASSERT(0, "!!!!!");
						}

						int2 GridPOS = Game.Players[Player].XY + Offset;

						size_t ID = chrono::high_resolution_clock::now().time_since_epoch().count();

						auto State = Game.GetCellState(GridPOS);
						if (State == EState::Bomb)
						{
							Game.MoveBomb(GridPOS, Offset);
						}
						else
						{
							Game.CreateBomb(EBombType::Regular, GridPOS, ID, Player);
							Game.MarkCell(GridPOS, EState::Bomb);
						}
						break;
					}
				}	break;
				default:
					break;
				}


				if (evt.Action == Event::Release)
				{
					switch ((PLAYER_EVENTS)evt.mData1.mINT[0])
					{
					case PLAYER_EVENTS::PLAYER_UP:
					case PLAYER_EVENTS::PLAYER_DOWN:
					case PLAYER_EVENTS::PLAYER_LEFT:
					case PLAYER_EVENTS::PLAYER_RIGHT:
						InputState.PreferredDirection = PF_UNKNOWN;
						break;
					default:
						break;
					}
				}


				if (evt.Action == Event::Pressed)
				{
					switch ((PLAYER_EVENTS)evt.mData1.mINT[0])
					{
					case PLAYER_EVENTS::PLAYER_UP:
						InputState.PreferredDirection = PLAYER_FACING_DIRECTION::PF_UP;			break;
					case PLAYER_EVENTS::PLAYER_LEFT:
						InputState.PreferredDirection = PLAYER_FACING_DIRECTION::PF_LEFT;		break;
					case PLAYER_EVENTS::PLAYER_DOWN:
						InputState.PreferredDirection = PLAYER_FACING_DIRECTION::PF_DOWN;		break;
					case PLAYER_EVENTS::PLAYER_RIGHT:
						InputState.PreferredDirection = PLAYER_FACING_DIRECTION::PF_RIGHT;		break;
					default:
						break;
					}
				}
			}
		}
	}


	void SetPlayerCameraAspectRatio(float AR)
	{
		FlexKit::SetCameraAspectRatio(GameCamera, AR);
	}


	void SetActive(Player_Handle P)
	{
		Player = P;
		Enabled = true;
	}


	void MovePlayer(FlexKit::int2 XY)
	{
		Game.MovePlayer(Player, XY);
	}


	void MoveUp()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{ 0, -1 });
		Game.Players[Player].FacingDirection = PF_UP;
	}


	void MoveDown()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{ 0,  1 });
		Game.Players[Player].FacingDirection = PF_DOWN;
	}


	void MoveLeft()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{ -1,  0 });
		Game.Players[Player].FacingDirection = PF_LEFT;
	}


	void MoveRight()
	{
		auto POS = Game.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{ 1,  0 });
		Game.Players[Player].FacingDirection = PF_RIGHT;
	}


	bool Enabled = false;


	void Update(const double dt)
	{
		ProcessEvents();

		int Direction = -1;

		if (InputState.PreferredDirection == PF_UNKNOWN)
		{
			if (InputState.UP)
			{
				Direction = PF_UP;
			}
			else if (InputState.DOWN)
			{
				Direction = PF_DOWN;
			}
			else if (InputState.LEFT)
			{
				Direction = PF_LEFT;
			}
			else if (InputState.RIGHT)
			{
				Direction = PF_RIGHT;
			}
		}
		else
		{
			Direction = InputState.PreferredDirection;
		}

		if (Direction >= 0)
		{
			switch ((Direction + GameCamera.FacingDirection) % PF_COUNT)
			{
			case PLAYER_FACING_DIRECTION::PF_UP:
				MoveUp();
				break;
			case PLAYER_FACING_DIRECTION::PF_LEFT:
				MoveLeft();
				break;
			case PLAYER_FACING_DIRECTION::PF_DOWN:
				MoveDown();
				break;
			case PLAYER_FACING_DIRECTION::PF_RIGHT:
				MoveRight();
				break;
			}
		}

		FrameCache.push_back(CurrentFrameEvents);
		CurrentFrameEvents.clear();

		const float3 SceneScale = float3{ Game.Scale.x / Game.WH[0], 1, Game.Scale.y / Game.WH[1] };

		GameCamera.Update(dt, Game.GetPlayerPosition(Player) * SceneScale);
	}


	struct
	{
		bool UP;
		bool DOWN;
		bool LEFT;
		bool RIGHT;

		int	PreferredDirection;// if -1 then a preferred direction is ignored

		operator bool() { return UP | DOWN | LEFT | RIGHT; }
	}InputState;

	GameCameraController						GameCamera;

	typedef static_vector<Event, 12>			KeyEventList;
	KeyEventList								CurrentFrameEvents;
	FlexKit::CircularBuffer<KeyEventList, 120>	FrameCache;

	Player_Handle	Player;
	Game&			Game;
};



/************************************************************************************************/


class DebugCameraController : 
	public FlexKit::OrbitCameraBehavior
{
public:
	void Initiate()
	{
	}

	bool EventHandler(Event evt)
	{
		if (evt.InputSource == Event::Keyboard)
		{
			if (evt.Action == Event::Pressed)
			{
				switch (evt.mData1.mINT[0])
				{
				case PLAYER_EVENTS::DEBUG_PLAYER_UP:
					MoveForward		= true;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_LEFT:
					MoveLeft		= true;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_DOWN:
					MoveBackward	= true;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_RIGHT:
					MoveRight		= true;
					break;
				default:
					break;
				};
			}

			if (evt.Action == Event::Release)
			{
				switch (evt.mData1.mINT[0])
				{
				case PLAYER_EVENTS::DEBUG_PLAYER_UP:
					MoveForward		= false;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_DOWN:
					MoveBackward	= false;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_LEFT:
					MoveLeft		= false;
					break;
				case PLAYER_EVENTS::DEBUG_PLAYER_RIGHT:
					MoveRight		= false;
					break;
				default:
					break;
				}
			}
		}
		return false;
	}


	void Update(float dt)
	{
		FlexKit::float3 Direction = { 0, 0, 0 };

		if (MoveForward)
			Direction += float3( 0,  0, -1);
		if (MoveBackward)
			Direction += float3( 0,  0,  1);
		if (MoveLeft)
			Direction += float3(-1, 0, 0);
		if (MoveRight)
			Direction += float3( 1, 0, 0);

		Direction = GetOrientation() * Direction;

		TranslateWorld(Direction * dt * MovingSpeed);
	}

	float		MovingSpeed		= 30.0f;
	bool		MoveForward		= false;
	bool		MoveBackward	= false;
	bool		MoveLeft		= false;
	bool		MoveRight		= false;
};


/************************************************************************************************/


class PlayerPuppet:
		public FlexKit::DrawableBehavior,
		public FlexKit::SceneNodeBehavior
{
public: 
	PlayerPuppet(FlexKit::GraphicScene* ParentScene, FlexKit::EntityHandle Handle) :
		FlexKit::DrawableBehavior	{ParentScene, Handle},
		FlexKit::SceneNodeBehavior	{GetNode()}
	{}


	PlayerPuppet(const PlayerPuppet&) = delete;


	PlayerPuppet(PlayerPuppet && rhs)
	{
		Node		 = rhs.Node;
		Entity		 = rhs.Entity;
		ParentScene	 = rhs.ParentScene;
		
		rhs.Entity		= FlexKit::InvalidHandle_t;
		rhs.Node		= FlexKit::InvalidHandle_t;
		rhs.ParentScene = nullptr;
	}


	void Update(float dt)
	{
		Yaw(dt * FlexKit::pi);
	}
};


/************************************************************************************************/


PlayerPuppet CreatePlayerPuppet(FlexKit::GraphicScene* ParentScene)
{
	return 	PlayerPuppet(
		ParentScene,
		ParentScene->CreateDrawableAndSetMesh("Flower"));
}


/************************************************************************************************/


enum ModelHandles
{
	MH_Floor,
	MH_BrokenFloor,
	MH_Obstacle,
	MH_Player,
	MH_RegularBomb,
};


/************************************************************************************************/


typedef FlexKit::Vector<Event> EventList;

class FrameSnapshot
{
public:
	FrameSnapshot(Game* IN = nullptr, EventList* IN_FrameEvents = nullptr, size_t IN_FrameID = (size_t)-1, iAllocator* IN_Memory = nullptr);
	~FrameSnapshot();

	FrameSnapshot(const FrameSnapshot&)					= delete;
	FrameSnapshot& operator = (const FrameSnapshot&)	= delete;

	FrameSnapshot& operator = (FrameSnapshot&& rhs)
	{
		FrameCopy	= std::move(rhs.FrameCopy);
		FrameEvents = std::move(rhs.FrameEvents);

		return *this;
	}


	FrameSnapshot	(FrameSnapshot&& rhs) :
		FrameCopy	{ std::move(rhs.FrameCopy)		},
		FrameEvents { std::move(rhs.FrameEvents)	}
	{}

	void Restore	(Game* out);
	
	EventList		FrameEvents;
	Game			FrameCopy;
	size_t			FrameID;

	iAllocator* Memory;
};


/************************************************************************************************/


void DrawGame(
	double							dt,
	float							AspectRatio,
	Game&							game,
	FlexKit::FrameGraph&			FrameGraph,
	FlexKit::ConstantBufferHandle	ConstantBuffer,
	FlexKit::VertexBufferHandle		VertexBuffer,
	FlexKit::TextureHandle			RenderTarget,
	FlexKit::TextureHandle			DepthBuffer,
	FlexKit::CameraHandle			Camera,
	FlexKit::TriMeshHandle			PlayerModel,
	FlexKit::iAllocator*			TempMem);


/************************************************************************************************/
#endif