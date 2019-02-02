#include "MultiplayerGameState.h"
#include "Packets.h"


using namespace FlexKit;


/************************************************************************************************/


MultiplayerGame::MultiplayerGame(
			FlexKit::GameFramework* IN_framework, 
			NetworkState*			IN_network,
			size_t					IN_playerCount, 
			BaseState* base) :
		FrameworkState	{IN_framework},
		
		frameID			{0},
		base			{base},
		network			{IN_network}
{
	AddResourceFile("CharacterBase.gameres");
	characterModel = FlexKit::LoadTriMeshIntoTable(framework->core->RenderSystem, "PlayerMesh");
}


/************************************************************************************************/


MultiplayerGame::~MultiplayerGame()
{
}


/************************************************************************************************/


bool MultiplayerGame::Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool MultiplayerGame::PreDrawUpdate(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool MultiplayerGame::Draw(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph)
{
	return true;
}


/************************************************************************************************/


bool MultiplayerGame::PostDrawUpdate(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	return true;
}


/**********************************************************************

Copyright (c) 2019 Robert May

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