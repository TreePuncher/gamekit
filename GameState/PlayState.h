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
	// Prevent Slicing
	iGridTask(const iGridTask& rhs)			= delete;
	iGridTask& operator = (iGridTask&& rhs) = delete;
	iGridTask& operator = (iGridTask& rhs)	= delete;

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


class LocalPlayerHandler :
	public FlexKit::iEventReceiver,
	public FlexKit::iUpdatable
{
public:
	LocalPlayerHandler(GameGrid& grid) :
		Grid{ grid } {}

	void Handle(const Event& evt) override
	{
	}

	void MovePlayer(FlexKit::int2 XY)
	{
		//if (Grid.IsCellClear(XY))
		//	Grid.Players[Player].XY = XY;

		Grid.MovePlayer(Player, XY);
	}

	void MoveUp()
	{
		auto POS = Grid.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{  0, -1 });
	}

	void MoveDown()
	{
		auto POS = Grid.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{  0,  1 });
	}

	void MoveLeft()
	{
		auto POS = Grid.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{ -1,  0 });
	}

	void MoveRight()
	{
		auto POS = Grid.Players[Player].XY;
		MovePlayer(POS + FlexKit::int2{  1,  0 });
	}

	bool Enabled = false;

	Player_Handle	Player;
	GameGrid&		Grid;
};


/************************************************************************************************/


class MovePlayerTask : 
	public iGridTask
{
public:
	MovePlayerTask(
		FlexKit::int2	a, 
		FlexKit::int2	b,
		Player_Handle	player,
		GameGrid*		grid
			) :
			A			{a},
			B			{b},
			Player		{player},
			complete	{false},
			T			{0.0f},
			Grid		{grid}
	{
		std::cout << "Moving Player\n";
		Grid->Players[Player].State = GridPlayer::PS_Moving;
	}

	MovePlayerTask& operator = (MovePlayerTask& rhs) = delete;

	void Update(const double dt) override;

	bool Complete() { return complete; }

	float			T;

	FlexKit::int2	A, B;

	GameGrid*		Grid;
	Player_Handle	Player;
	bool			complete;
};


/************************************************************************************************/


class InputMap
{
public:
	bool Map(const Event& evt_in, Event& evt_out)
	{

	}

	void MapKeyToEvent(KEYCODES KC, size_t EventID)
	{

	}

private:

	static_vector<Event, KC_COUNT> EventMap;
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

	InputComponentSystem		Input;
	OrbitCameraSystem			OrbitCameras;
	GraphicScene				Scene;

	ThirdPersonCameraComponentSystem	TPC;
	PhysicsComponentSystem				Physics;
	DrawableComponentSystem				Drawables;
	LightComponentSystem				Lights;

	WorldRender							Render;

	TextureHandle				DepthBuffer;
	ConstantBufferHandle		ConstantBuffer;
	VertexBufferHandle			VertexBuffer;

	GameGrid			Grid;
	LocalPlayerHandler	Player1_Handler;
	LocalPlayerHandler	Player2_Handler;
};


/************************************************************************************************/

#endif