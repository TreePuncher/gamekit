#ifndef PLAYSTATE_H
#define PLAYSTATE_H

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

#include "..\Application\GameFramework.h"
#include "..\Application\CameraUtilities.h"
#include "..\Application\GameMemory.h"
#include "..\Application\WorldRender.h"


#include "Gameplay.h"


/*
TODO's
*/


using FlexKit::GameFramework;


typedef size_t Player_Handle;
typedef size_t GridObject_Handle;


/************************************************************************************************/


class InputMap
{
public:
	InputMap(iAllocator* Memory) :
		EventMap{ Memory }{}


	bool Map(const Event& evt_in, Event& evt_out)
	{
		if (evt_in.InputSource != Event::Keyboard)
			return false;

		auto res = FlexKit::find(
			EventMap, 
			[&](auto &rhs) 
				{
					return evt_in.mData1.mKC[0] == rhs.EventKC;
				});

		if (res == EventMap.end())
			return false;

		evt_out					= evt_in;
		evt_out.mData1.mINT[0]	= (*res).EventID;

		return true;
	}

	void MapKeyToEvent(KEYCODES KC, int32_t EventID)
	{
		EventMap.push_back({ EventID, KC });
	}

	bool operator ()(const Event& evt_in, Event& evt_out)
	{
		return Map(evt_in, evt_out);
	}

private:

	struct TiedEvent
	{
		int32_t		EventID;
		KEYCODES	EventKC;
	};

	Vector<TiedEvent> EventMap;
};


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
		Tasks  { memory }
	{}


	Player_Handle		CreatePlayer();
	GridObject_Handle	CreateGridObject();

	bool MovePlayer		(Player_Handle Player, GridID_t GridID);
	bool IsCellClear	(GridID_t GridID);
	void Update			(const double dt, iAllocator* Memory);

	FlexKit::Vector<GridPlayer>		Players;
	FlexKit::Vector<GridObject>		Objects;
	FlexKit::Vector<iGridTask*>		Tasks;

	iAllocator* Memory;
};


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


class PlayState : public FrameworkState
{
public:
	PlayState(GameFramework* Framework);
	~PlayState();

	bool Update			(EngineCore* Engine, double dT) final;

	bool PreDrawUpdate	(EngineCore* Engine, double dT) final;
	bool Draw			(EngineCore* Engine, double dT, FrameGraph& Graph) final;
	bool DebugDraw		(EngineCore* Engine, double dT) final;

	bool EventHandler	(Event evt)	final;

	void BindPlayer1();
	void BindPlayer2();

	void ReleasePlayer1();
	void ReleasePlayer2();

	WorldRender					Render;

	TextureHandle				DepthBuffer;
	ConstantBufferHandle		ConstantBuffer;
	VertexBufferHandle			VertexBuffer;

	GameGrid			Grid;
	LocalPlayerHandler	Player1_Handler;
	LocalPlayerHandler	Player2_Handler;
};


/************************************************************************************************/

#endif