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
		0.25f,
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
