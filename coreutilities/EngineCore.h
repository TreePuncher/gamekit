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
#include "..\coreutilities\GraphicsComponents.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\ProfilingUtilities.h"
#include "..\coreutilities\Resources.h"
#include "..\coreutilities\ThreadUtilities.h"

#include "..\graphicsutilities\ForwardRendering.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\MeshUtils.h"

#include "..\PhysicsUtilities\physicsutilities.h"


/************************************************************************************************/


namespace FlexKit
{
	struct sKeyState
	{
		sKeyState()
		{
			W = false;
			A = false;
			S = false;
			D = false;
			Q = false;
			E = false;
			P = false;
			R = false;
			F = false;
			SpaceBar = false;
			FrameID = 0;
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
		FlexKit::int2	dPos					= { 0, 0 };
		FlexKit::float2	Normalized_dPos			= { 0, 0 };
		FlexKit::float2	Position				= { 0, 0 };
		FlexKit::float2	NormalizedPos			= { 0, 0 };
		FlexKit::float2	NormalizedScreenCord	= { 0, 0 };



		bool LMB_Pressed = false;
		bool RMB_Pressed = false;
		bool Enabled = false;

		double LMB_Duration = 0.0;
		double RMB_Duration = 0.0;
	};


	/************************************************************************************************/


	struct EngineMemory;

	static const size_t MAX_CLIENTS = 10;
	static const size_t SERVER_PORT = 60000;

	static const size_t PRE_ALLOC_SIZE = GIGABYTE * 2;
	static const size_t LEVELBUFFERSIZE = MEGABYTE * 64;
	static const size_t NODEBUFFERSIZE = MEGABYTE * 64;
	static const size_t TEMPBUFFERSIZE = MEGABYTE * 128;
	static const size_t BLOCKALLOCSIZE = MEGABYTE * 768;


	/************************************************************************************************/


	struct EngineMemory
	{
		// Allocators
		BlockAllocator	BlockAllocator;
		StackAllocator	TempAllocator;
		StackAllocator	LevelAllocator;

		EngineMemory_DEBUG	Debug;


		auto GetBlockMemory() -> decltype(BlockAllocator)&	 { return BlockAllocator; }
		auto GetLevelMemory() -> decltype(LevelAllocator)&	 { return LevelAllocator; }
		auto GetTempMemory()  -> decltype(TempAllocator)&	 { return TempAllocator;  }


		// Memory Pools
		byte	NodeMem[NODEBUFFERSIZE];
		byte	BlockMem[BLOCKALLOCSIZE];
		byte	LevelMem[LEVELBUFFERSIZE];
		byte	TempMem[TEMPBUFFERSIZE];
	};


	/************************************************************************************************/


	class EngineCore
	{
	public:
		EngineCore(EngineMemory* memory) :
			Memory			(memory),
			CmdArguments	(memory->BlockAllocator),
			Time			(memory->BlockAllocator),
			Threads			(4, memory->BlockAllocator),// TODO: Get System Thread Count.
			RenderSystem	(memory->BlockAllocator, &Threads),
			FrameLock		(true)
		{
			InitiateSceneNodeBuffer(memory->NodeMem, sizeof(EngineMemory::NodeMem));
		}


		~EngineCore()
		{
			if (!Memory)
				return;

			Release();

			Memory = nullptr;
		}


		bool Initate(EngineMemory* Memory, uint2 WH);
		void Release();

		EngineCore				(const EngineCore&) = delete;
		EngineCore& operator =	(const EngineCore&) = delete;

		bool			FrameLock;
		bool			End;

		RenderSystem			RenderSystem;

		RenderWindow			Window;
		Time					Time;

		PhysicsSystem			Physics;

		Vector<const char*>		CmdArguments;

		ThreadManager			Threads;
		EngineMemory*			Memory;

		BlockAllocator& GetBlockMemory() { return  Memory->BlockAllocator; }
		StackAllocator& GetLevelMemory() { return  Memory->LevelAllocator; }
		StackAllocator& GetTempMemory()  { return  Memory->TempAllocator; }

		EngineMemory_DEBUG* GetDebugMemory() { return &Memory->Debug; }
	};


	EngineMemory*	CreateEngineMemory();
	EngineMemory*	CreateEngineMemory(bool&);
	
	void ReleaseCore			(EngineCore*		Game);
	bool InitiateCoreSystems	(uint2 WH,			EngineCore*& Game);

	void UpdateCoreComponents	(EngineCore* Core, double dt);


	/************************************************************************************************/


	float GetWindowAspectRatio	(EngineCore* Engine);
	uint2 GetWindowWH			(EngineCore* Engine);

	float2	GetPixelSize	(EngineCore*);

	void UpdateMouseInput	(MouseInputState* State,	RenderWindow* Window);
	void PushCmdArg			(EngineCore* Engine,		const char* arg);



}	/************************************************************************************************/