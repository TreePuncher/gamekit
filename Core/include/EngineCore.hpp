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

#include "BuildSettings.hpp"

#include "Assets.hpp"
#include "Containers.hpp"
#include "MemoryUtilities.hpp"
#include "ThreadUtilities.hpp"
#include "TimeUtilities.hpp"
#include "Input.hpp"


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
		EngineMemory(BlockAllocator_desc& desc, size_t tempAllocatorSize = TEMPBUFFERSIZE) :
			blockAllocator	{},
			tempAllocator	{},
			tempAllocatorMT	{ tempAllocator }
		{
			blockAllocator.Init(desc);
			tempAllocator.Init((std::byte*)_aligned_malloc(TEMPBUFFERSIZE, 0x10), tempAllocatorSize);
		}

		BlockAllocator		blockAllocator;
		ThreadSafeAllocator	blockAllocatorMT{ blockAllocator };

		StackAllocator		tempAllocator;
		ThreadSafeAllocator	tempAllocatorMT;

		auto GetBlockMemory() -> auto&	{ return blockAllocator;	}
		auto GetTempMemory() -> auto&	{ return tempAllocator;		}
		auto GetTempMemoryMT() -> auto&	{ return tempAllocatorMT;	}
	};


	/************************************************************************************************/


	typedef IRenderSystem* (*fnCreateRenderSystem)(const RenderSystemOptions&);

	struct CoreOptions
	{
		uint32_t	threadCount		= FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1;
		bool		GPUdebugMode	= false;
		bool		GPUValidation	= false;
		bool		GPUSyncQueues	= false;

		fnCreateRenderSystem	CreateRenderSystem = nullptr;
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

		EngineMemory*			Memory;
	    ThreadManager			Threads;
		IRenderSystem*			RenderSystem;

		Time					time;

		Vector<const char*>		CmdArguments;

		IRenderWindow*			activeWindow = nullptr;

		BlockAllocator&			GetBlockMemory()	{ return  Memory->blockAllocator; }
		ThreadSafeAllocator&	GetMTBlockMemory()	{ return  Memory->blockAllocatorMT; }
		StackAllocator&			GetTempMemory()		{ return  Memory->GetTempMemory(); }
		ThreadSafeAllocator&	GetTempMemoryMT()	{ return  Memory->GetTempMemoryMT(); }
	};


	EngineMemory*	CreateEngineMemory();
	EngineMemory*	CreateEngineMemory(bool&);


	void ReleaseEngineMemory(EngineMemory* Memory);
	void PushCmdArg			(EngineCore* Engine,		const char* arg);


}	/************************************************************************************************/
