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


class PlayState : public FrameworkState
{
public:
	PlayState(
		GameFramework*	IN_Framework, 
		BaseState*		Base);
	

	~PlayState();


	bool Update			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool PreDrawUpdate	(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool Draw			(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;
	bool PostDrawUpdate	(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) final;

	bool DebugDraw		(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT) final;
	bool EventHandler	(Event evt)	final;


	GraphicScene		scene;

	WorldRender&		render;
	BaseState&			base;

	SoundSystem			sound;

	/*
	FlexKit::GuiSystem	ui;
	FlexKit::GUIGrid*	uiMainGrid;
	FlexKit::GUIGrid*	uiSubGrid_1;
	FlexKit::GUIGrid*	uiSubGrid_2;
	*/

	InputMap		eventMap;
	InputMap		debugCameraInputMap;
	InputMap		debugEventsInputMap;
	size_t			frameID;
	TriMeshHandle	characterModel;
	TriMeshHandle	LightModel;

	FlexKit::OrbitCameraBehavior	debugCamera;
	//FlexKit::ThirdPersonCameraRig	thirdPersonCamera;
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