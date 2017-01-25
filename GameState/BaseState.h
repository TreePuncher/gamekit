#ifndef BASESTATE_H
#define BASESTATE_H

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

#ifdef COMPILE_GAMESTATE
#define GAMESTATEAPI __declspec(dllexport)
#else
#define GAMESTATEAPI __declspec(dllimport)
#endif


#include "..\Application\GameMemory.h"
#include "..\graphicsutilities\Graphics.h"
#include "..\graphicsutilities\TerrainRendering.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\Resources.h"

#include "Console.h"

#include <iostream>

using namespace FlexKit;

struct MouseHandler
{
};

struct SubState;

typedef bool (*FN_UPDATE_SUBSTATE)  (SubState* StateMemory, EngineMemory*, double DT); // Return True to allow Cascading updates down State Stack
typedef void (*FN_RELEASE_SUBSTATE) (SubState* StateMemory);

typedef void(*FN_EVENT_HANDLER) (SubState* StateMemory, Event);


struct SubStateVTable
{
	FN_UPDATE_SUBSTATE  Update			= nullptr;
	FN_UPDATE_SUBSTATE  PreDrawUpdate	= nullptr;
	FN_UPDATE_SUBSTATE  PostDrawUpdate	= nullptr;
	FN_EVENT_HANDLER	EventHandler	= nullptr;
	FN_RELEASE_SUBSTATE Release			= nullptr;
};

struct BaseState
{
	struct {
		FontAsset*			Font;
		Texture2D			Terrain;
	}DefaultAssets;

	MouseInputState		MouseState;

	Landscape			Landscape;
	SceneNodes*			Nodes;
	PScene				PScene;
	GraphicScene		GScene;

	Camera				DefaultCamera;

	Camera*				ActiveCamera;
	RenderWindow*		ActiveWindow;
	float4				ClearColor;

	Console				Console;
	GUIRender			GUIRender;
	EngineMemory*		Engine;

	static_vector<MouseHandler>		MouseHandlers;
	static_vector<SubStateVTable*>	SubStates;

	double				PhysicsUpdateTimer;
	bool				Quit;

	size_t				TerrainSplits;
};


struct SubState
{
	SubStateVTable	VTable;
	BaseState*		Base;
};


/************************************************************************************************/


void			DrawMouseCursor	 (EngineMemory* Engine, BaseState* State);

void			PushSubState	 (BaseState* _ptr, SubState* SS);
void			PopSubState		 (BaseState* _ptr);

void			UpdateBaseState	 (EngineMemory* Engine, BaseState* _ptr, double dT);
void			PreDrawBaseState (EngineMemory* Engine, BaseState* _ptr, double dT);
void			ReleaseBaseState (EngineMemory* Engine, BaseState* _ptr );

SubStateVTable* GetStateVTable	 (SubState* _ptr);

#endif