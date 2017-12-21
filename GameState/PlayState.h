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

#include "Gameplay.h"

/*
TODO's
*/


using FlexKit::GameFramework;


class PlayState : public FrameworkState
{
public:
	// Game Element Controllers
	// GameplayComponentSystem		Model;

	PlayState(EngineCore* Engine, GameFramework* Framework);
	~PlayState();

	bool Update			(EngineCore* Engine, double dT) final;
	bool DebugDraw		(EngineCore* Engine, double dT) final;
	bool PreDrawUpdate	(EngineCore* Engine, double dT) final;
	bool Draw			(EngineCore* Engine, double dT, FrameGraph& Graph) final;

	bool EventHandler	(Event evt)	final;

	InputComponentSystem		Input;
	OrbitCameraSystem			OrbitCameras;
	GraphicScene				Scene;

	ThirdPersonCameraComponentSystem	TPC;
	PhysicsComponentSystem				Physics;
	DrawableComponentSystem				Drawables;
	LightComponentSystem				Lights;

	VertexBufferHandle			VertexBuffer;
	int							Test;

	// GameObjects
	GameObject<> CubeObjects[1024];
	GameObject<> FloorObject;
	GameObject<> TestObject;
	GameObject<> Player;
};


PlayState* CreatePlayState(EngineCore* Engine, GameFramework* Framework);


#endif