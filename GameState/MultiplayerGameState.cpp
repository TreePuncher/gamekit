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
		
		frameEvents		{IN_framework->core->GetBlockMemory()},
		frameID			{0},
		base			{base},
		localHandler	{localGame, IN_framework->core->GetBlockMemory()},
		localGame		{IN_framework->core->GetBlockMemory()},
		network			{IN_network},
		handler			{IN_framework->core->GetBlockMemory()}
{
	AddResourceFile("CharacterBase.gameres");
	characterModel = FlexKit::LoadTriMeshIntoTable(framework->core->RenderSystem, "PlayerMesh");

	network->PushHandler(&handler);
}


/************************************************************************************************/


MultiplayerGame::~MultiplayerGame()
{
	network->PopHandler();
}


/************************************************************************************************/


bool MultiplayerGame::Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
{
	frameCache.push_back(
		FrameSnapshot(
			&localGame, 
			&frameEvents, 
			frameID, 
			framework->core->GetBlockMemory()));

	localGame.Update(dT, framework->core->GetTempMemory());

	frameID++;

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
	auto currentRenderTarget = GetCurrentBackBuffer(&framework->core->Window);
	CameraHandle ActiveCamera = (CameraHandle)localHandler.GameCamera;

	ClearVertexBuffer	(frameGraph, base->vertexBuffer);
	ClearVertexBuffer	(frameGraph, base->textBuffer);
	ClearBackBuffer		(frameGraph, { 0.0f, 0.0f, 0.0f, 0.0f });


	DrawGame(
		dT,
		GetWindowAspectRatio(framework->core),
		localGame,
		frameGraph,
		base->constantBuffer,
		base->vertexBuffer,
		GetCurrentBackBuffer(&framework->core->Window),
		base->depthBuffer,
		ActiveCamera,
		characterModel,
		framework->core->GetTempMemory());


	PresentBackBuffer(frameGraph, &framework->core->Window);

	return true;
}


/************************************************************************************************/


bool MultiplayerGame::PostDrawUpdate(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	return true;
}


/************************************************************************************************/