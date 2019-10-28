#ifndef PLAYSTATE_H
#define PLAYSTATE_H

#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\CameraUtilities.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\WorldRender.h"
#include "..\coreutilities\Components.h"

#include "ThirdPersonUtilities.h"

#include "Gameplay.h"
#include "BaseState.h"


class thirdPersonActor final
{
public:
	thirdPersonActor(iAllocator* IN_allocator) : allocator{ IN_allocator }
	{
		// Add behaviors
		gameObject.AddView<CameraView>();
		gameObject.AddView<SceneNodeView<>>();

		// Tie nodes together
		Apply(gameObject, []
			(	CameraView&			camera,
				SceneNodeView<>&	node)
			{
				node.Parent(camera.GetCameraNode());
			});
	}


	~thirdPersonActor()
	{
		auto node	= &GetNode();
		auto camera	= &GetCamera();
		gameObject.Release();
	}


	SceneNodeView<>&    	GetNode()	{ return GetView<SceneNodeView<>>(gameObject); }
	CameraView&			    GetCamera() { return GetView<CameraView>(gameObject);		}

	operator GameObject&		() { return gameObject; }
	operator SceneNodeView<>&   () { return GetView<SceneNodeView<>>(gameObject);	}
	operator CameraView&	    () { return GetView<CameraView>(gameObject);		}

private:
	GameObject	gameObject;
	iAllocator* allocator;
};


class PlayState : public FrameworkState
{
public:
	PlayState(
		GameFramework&	IN_Framework, 
		BaseState&		Base);
	

	~PlayState();


	void Update			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final override;
    void PreDrawUpdate	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final override;
    void Draw			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final override;
    void PostDrawUpdate	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final override;

    void DebugDraw		(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) final override;
    void EventHandler	(Event evt)	final;


	GraphicScene		scene;

	WorldRender&		render;
	BaseState&			base;

	InputMap		eventMap;
	InputMap		debugCameraInputMap;
	InputMap		debugEventsInputMap;
	size_t			frameID;
	TriMeshHandle	characterModel;
	TriMeshHandle	LightModel;

	OrbitCameraBehavior	    debugCamera;
};


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

#endif
