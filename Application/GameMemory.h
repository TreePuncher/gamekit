/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#pragma once

#include "..\buildsettings.h"

#include "..\coreutilities\timeutilities.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\ProfilingUtilities.h"
#include "..\coreutilities\Resources.h"

#include "..\coreutilities\GraphicsComponents.h"

#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\ForwardRendering.h"
#include "..\graphicsutilities\TiledRender.h"
#include "..\graphicsutilities\SSReflections.h"
#include "..\graphicsutilities\MeshUtils.h"

#include "..\PhysicsUtilities\physicsutilities.h"

/************************************************************************************************/


using namespace FlexKit;


/************************************************************************************************/


struct sKeyState
{
	sKeyState()
	{
		W        = false;
		A        = false;
		S        = false;
		D        = false;
		Q        = false;
		E        = false;
		P        = false;
		R        = false;
		F        = false;
		SpaceBar = false;
		FrameID  = 0;
	}

	char B[4];
	bool W;
	bool A;
	bool S;
	bool D;
	bool Q;
	bool E;
	bool P;
	bool R;
	bool F;
	bool SpaceBar;
	size_t FrameID;
};


/************************************************************************************************/


struct MouseInputState
{
	int2	dPos			= { 0, 0 };
	float2	Position		= { 0, 0 };
	float2	NormalizedPos	= { 0, 0 };

	bool LMB_Pressed = false;
	bool RMB_Pressed = false;
	bool Enabled	 = false;

	double LMB_Duration = 0.0;
	double RMB_Duration = 0.0;
};


/************************************************************************************************/


struct EngineMemory;

static const size_t MAX_CLIENTS = 10;
static const size_t SERVER_PORT = 60000;

static const size_t PRE_ALLOC_SIZE  = GIGABYTE * 2;
static const size_t LEVELBUFFERSIZE = MEGABYTE * 64;
static const size_t NODEBUFFERSIZE  = MEGABYTE * 64;
static const size_t TEMPBUFFERSIZE  = MEGABYTE * 128;
static const size_t BLOCKALLOCSIZE	= MEGABYTE * 768;


/************************************************************************************************/


struct EngineMemory
{
	BlockAllocator	BlockAllocator;
	StackAllocator	TempAllocator;
	StackAllocator	LevelAllocator;

	bool			FrameLock;
	bool			End;

	RenderSystem	RenderSystem;
	OcclusionCuller	Culler;

	RenderWindow	Window;
	DepthBuffer		DepthBuffer;
	Time			Time;
	Resources		Assets;
	GeometryTable	Geometry;

	PhysicsSystem	Physics;

	TiledDeferredRender	TiledRender;
	ForwardRender		ForwardRender;
	SSReflectionBuffers	Reflections;

	PVS			PVS_;

	static_vector<Event>	NetworkEvents;
					
	// Component System
	SceneNodeComponentSystem	Nodes;

	Vector<const char*>		CmdArguments;

	EngineMemory_DEBUG	Debug;

	// Memory Pools
	byte						NodeMem	[NODEBUFFERSIZE];
	byte						BlockMem[BLOCKALLOCSIZE];
	byte						LevelMem[LEVELBUFFERSIZE];
	byte						TempMem	[TEMPBUFFERSIZE];
};


/************************************************************************************************/


typedef void (*MouseEventHandler)		(EngineMemory* Engine, const FlexKit::Event& in);
typedef void*(*InitiateGameStateFN)		(EngineMemory* Engine);
typedef bool (*InitiateEngineFN)		(EngineMemory* Engine);
typedef void (*OnHotReloadFN)			(EngineMemory*, double dt, void* _ptr);

typedef void (*UpdateFN)				(EngineMemory* Engine, void* _ptr, double dt);
typedef void (*FixedUpdateFN)			(EngineMemory*, double dt, void* _ptr);

typedef void (*UpdateAnimationsFN)		(EngineMemory* RS,		iAllocator* TempMemory, double dt,	void* _ptr);
typedef	void (*UpdatePreDrawFN)			(EngineMemory* Engine,	iAllocator* TempMemory, double dt,	void* _ptr); 
typedef	void (*DrawFN)					(EngineMemory* RS,		iAllocator* TempMemory,				void* _ptr);
typedef void (*PostDrawFN)				(EngineMemory* Engine,	iAllocator* TempMemory, double dt,	void* _ptr);
typedef void (*CleanUpFN)				(EngineMemory* Engine,	void* _ptr);

typedef void (*PostPhysicsUpdateFN)	(void*);
typedef void (*PrePhysicsUpdateFN)	(void*);



/************************************************************************************************/


struct CodeTable
{
	InitiateGameStateFN	Init;
	InitiateEngineFN	InitEngine;
	UpdateFN			Update;
	FixedUpdateFN		UpdateFixed;
	UpdateAnimationsFN	UpdateAnimations;
	UpdatePreDrawFN		UpdatePreDraw;
	DrawFN				Draw;
	PostDrawFN			PostDraw;
	CleanUpFN			Cleanup;
};

typedef void (*GetStateTableFN)(CodeTable* out);


/************************************************************************************************/