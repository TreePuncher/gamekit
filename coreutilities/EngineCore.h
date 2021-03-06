/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#include "buildsettings.h"

#include "Assets.h"
#include "containers.h"
#include "GraphicsComponents.h"
#include "memoryutilities.h"
#include "ProfilingUtilities.h"
#include "ThreadUtilities.h"
#include "timeutilities.h"
#include "Input.h"

#include "FrameGraph.h"
#include "graphics.h"
#include "MeshUtils.h"

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


	struct EngineMemory;

	static const size_t MAX_CLIENTS = 10;
	static const size_t SERVER_PORT = 60000;

	static const size_t TEMPBUFFERSIZE = MEGABYTE * 256;
	static const size_t BLOCKALLOCSIZE = GIGABYTE * 2;

	static const size_t PRE_ALLOC_SIZE = BLOCKALLOCSIZE + TEMPBUFFERSIZE;

	/************************************************************************************************/


	struct EngineMemory
	{
        EngineMemory() :
            BlockAllocator{},
            TempAllocator{},
            TempAllocatorMT{ TempAllocator }
        {
            BlockAllocator_desc BAdesc;
            BAdesc.SmallBlock   = BLOCKALLOCSIZE / 4;
            BAdesc.MediumBlock  = BLOCKALLOCSIZE / 4;
            BAdesc.LargeBlock   = BLOCKALLOCSIZE / 2;

            BlockAllocator.Init(BAdesc);
            TempAllocator.Init(TempMem, TEMPBUFFERSIZE);
        }

		BlockAllocator	    BlockAllocator;
		StackAllocator	    TempAllocator;
        ThreadSafeAllocator TempAllocatorMT;

		auto GetBlockMemory() -> decltype(BlockAllocator)&	    { return BlockAllocator; }
		auto GetTempMemory()  -> decltype(TempAllocator)&	    { return TempAllocator; }
		auto GetTempMemoryMT()  -> decltype(TempAllocatorMT)&	{ return TempAllocatorMT; }


		// Memory Pools
		byte*	BlockMem = (byte*)_aligned_malloc(BLOCKALLOCSIZE, 16);
		byte*	TempMem  = (byte*)_aligned_malloc(TEMPBUFFERSIZE, 16);
	};


	/************************************************************************************************/


	class EngineCore
	{
	public:
		EngineCore(EngineMemory* memory, size_t threadCount) :
			Memory			{ memory										},
			CmdArguments	{ memory->BlockAllocator						},
			Time			{ memory->BlockAllocator						},
			Threads			{ threadCount, memory->BlockAllocator	        },
			RenderSystem	{ memory->BlockAllocator, &Threads				}
		{
			InitiateSceneNodeBuffer(memory->BlockAllocator);

            if (!Initiate(memory))
                throw std::runtime_error{"Failed to initiate core"};
		}


		~EngineCore()
		{
			if (!Memory)
				return;

			Release();

            ReleaseGeometryTable();
            SceneNodeTable.Release();
            RenderSystem.Release();
            profiler.Release();

			Memory = nullptr;
		}


        bool Initiate(EngineMemory* Memory);
		void Release();

		EngineCore				(const EngineCore&) = delete;
		EngineCore& operator =	(const EngineCore&) = delete;

		bool					FrameLock   = true;
        size_t                  FPSLimit    = 120;
		bool					End         = false;
        ThreadManager			Threads;

		RenderSystem			RenderSystem;

		Time					Time;

		Vector<const char*>		CmdArguments;

		EngineMemory*			Memory;

		BlockAllocator&         GetBlockMemory()    { return  Memory->BlockAllocator; }
        StackAllocator&         GetTempMemory()     { return  Memory->GetTempMemory(); }
		ThreadSafeAllocator&    GetTempMemoryMT()   { return  Memory->GetTempMemoryMT(); }
	};


	EngineMemory*	CreateEngineMemory();
	EngineMemory*	CreateEngineMemory(bool&);


    void ReleaseEngineMemory(EngineMemory* Memory);


	/************************************************************************************************/


    void EnableMouseInput   ();
    void DisableMouseInput  ();
	void UpdateMouseInput	(MouseInputState* State,	RenderWindow* Window);
	void PushCmdArg			(EngineCore* Engine,		const char* arg);



}	/************************************************************************************************/
