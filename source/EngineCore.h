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

#include "MeshUtils.h"
#include "physicsutilities.h"


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

	class RenderSystem;
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
			TempAllocator.Init((byte*)_aligned_malloc(TEMPBUFFERSIZE, 0x10), TEMPBUFFERSIZE);
		}

		BlockAllocator	    BlockAllocator;
		StackAllocator	    TempAllocator;
		ThreadSafeAllocator TempAllocatorMT;

		auto GetBlockMemory() -> auto&	    { return BlockAllocator;    }
		auto GetTempMemory()  -> auto&	    { return TempAllocator;     }
		auto GetTempMemoryMT()  -> auto&	{ return TempAllocatorMT;   }
	};


	/************************************************************************************************/


	struct CoreOptions
	{
		uint32_t	threadCount		= 4;
		bool		GPUdebugMode	= false;
		bool		GPUValidation	= false;
		bool		GPUSyncQueues	= false;
	};

	class EngineCore
	{
	public:
		EngineCore(EngineMemory* memory, const CoreOptions& options = {});
		~EngineCore();


		bool Initiate(EngineMemory* Memory, const bool debugmode = false, const bool gpuValidation = false, const bool syncQueues = false);
		void Release();

		EngineCore				(const EngineCore&) = delete;
		EngineCore& operator =	(const EngineCore&) = delete;

		bool					FrameLock	= true;
		bool					vSync		= false;
		size_t					FPSLimit	= 120;
		bool					End			= false;
		ThreadManager			Threads;

		RenderSystem&			RenderSystem;

		Time					Time;

		Vector<const char*>		CmdArguments;

		EngineMemory*			Memory;

		BlockAllocator&			GetBlockMemory()	{ return  Memory->BlockAllocator; }
		StackAllocator&			GetTempMemory()		{ return  Memory->GetTempMemory(); }
		ThreadSafeAllocator&	GetTempMemoryMT()	{ return  Memory->GetTempMemoryMT(); }
	};


	EngineMemory*	CreateEngineMemory();
	EngineMemory*	CreateEngineMemory(bool&);


	void ReleaseEngineMemory(EngineMemory* Memory);
	void PushCmdArg			(EngineCore* Engine,		const char* arg);


}	/************************************************************************************************/
