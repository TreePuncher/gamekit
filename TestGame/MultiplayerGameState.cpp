#include "MultiplayerGameState.h"
#include "Packets.h"


using namespace FlexKit;


/************************************************************************************************/


GameState::GameState(
			FlexKit::GameFramework* IN_framework, 
			BaseState&				IN_base) :
		FrameworkState	{IN_framework},
		
		frameID			{ 0										},
		base			{ IN_base								},
		scene			{ IN_framework->core->GetBlockMemory()	}
{
	LoadScene(framework->core, &scene, "TestScene");
}


/************************************************************************************************/


GameState::~GameState()
{
	iAllocator* allocator = base.framework->core->GetBlockMemory();
	auto entities = scene.sceneEntities;

	for (auto entity : entities)
	{
		auto entityGO = SceneVisibilityBehavior::GetComponent()[entity].entity;
		scene.RemoveEntity(*entityGO);

		entityGO->Release();
		allocator->free(entityGO);
	}

	scene.ClearScene();
}


/************************************************************************************************/


bool GameState::Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool GameState::PreDrawUpdate(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool GameState::Draw(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph)
{
	return true;
}


/************************************************************************************************/


bool GameState::PostDrawUpdate(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
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