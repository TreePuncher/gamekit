/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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

#include "EngineCore.hpp"
#include "Transforms.hpp"
#include "RenderSystemInterface.hpp"
#include "TriMeshResource.hpp"
#include <print>

namespace FlexKit
{   /************************************************************************************************/


	bool CreateRenderWindow(EngineCore* Game, uint32_t height, uint32_t width, bool fullscreen = false);


	/************************************************************************************************/


	RenderSystemOptions GetRSOptions(EngineCore& core, const CoreOptions& options)
	{
		RenderSystemOptions out{
			.threads	= &core.Threads,
			.allocator	= core.GetBlockMemory(),
			.modes		= options.GPUdebugMode ? RenderSystemModes::Debug : RenderSystemModes::Normal, 
		};

		return out;
	}

	EngineCore::EngineCore(EngineMemory* memory, const CoreOptions& options) :
		Memory			{ memory										},
		CmdArguments	{ memory->blockAllocator						},
		time			{ memory->blockAllocator						},
		Threads			{ options.threadCount, memory->blockAllocator	},
		RenderSystem	{ options.CreateRenderSystem != nullptr ? options.CreateRenderSystem(GetRSOptions(*this, options)) : nullptr }
	{
		profiler.GetThreadProfiler();
		InitiateSceneNodeBuffer(memory->blockAllocator);

#ifdef _DEBUG
		const bool debugMode = true;
#else
		const bool debugMode = false;
#endif

		if (!Initiate(memory, options.GPUdebugMode, options.GPUValidation, options.GPUSyncQueues))
			throw std::runtime_error{"Failed to initiate core"};
	}


	/************************************************************************************************/


	EngineCore::~EngineCore()
	{
		if (!Memory)
			return;

		Release();

		ReleaseGeometryTable();
		SceneNodeTable.Release();
		RenderSystem->Release();
		profiler.Release();

		Memory = nullptr;
	}


	/************************************************************************************************/


	EngineMemory::EngineMemory(BlockAllocator_desc& desc, size_t tempAllocatorSize) :
		blockAllocator{},
		tempAllocator{},
		tempAllocatorMT{ tempAllocator }, 
	    internalMemory{ desc._ptr }
	{
		blockAllocator.Init(desc);
		tempAllocator.Init((std::byte*)_aligned_malloc(TEMPBUFFERSIZE, 0x10), tempAllocatorSize);
	}


	/************************************************************************************************/


	EngineMemory::~EngineMemory()
	{
		DEBUGBLOCK(PrintBlockStatus(&GetBlockMemory()));

		_aligned_free(internalMemory);// VirtualFree(Memory, 0, MEM_RELEASE);
	}


	EngineMemory CreateEngineMemory()
	{
		BlockAllocator_desc BAdesc;
		BAdesc.PoolSize		= PRE_ALLOC_SIZE;
		BAdesc.SmallBlock   = BLOCKALLOCSIZE / 4;
		BAdesc.MediumBlock  = BLOCKALLOCSIZE / 4;
		BAdesc.LargeBlock   = BLOCKALLOCSIZE / 2;

		std::print("Pool Size: {}\n", BAdesc.PoolSize);
		auto allocation		= _aligned_malloc(BAdesc.PoolSize, 64);//VirtualAlloc(nullptr, preallocationSize, MEM_COMMIT, PAGE_READWRITE);;
		FK_ASSERT(allocation != nullptr, "Memory Allocation Error!");
		BAdesc._ptr = (std::byte*)allocation;
	    
		return EngineMemory{ BAdesc };
	}


	/************************************************************************************************/


	void ReleaseEngineMemory(EngineMemory* Memory)
	{
		DEBUGBLOCK(PrintBlockStatus(&Memory->GetBlockMemory()));

		_aligned_free(Memory);// VirtualFree(Memory, 0, MEM_RELEASE);
	}


	/************************************************************************************************/


	bool EngineCore::Initiate(EngineMemory* Memory, const bool debugMode, const bool gpuValidation, const bool syncQueues)
	{
		Graphics_Desc	desc	= { 0 };
		desc.Memory				= GetBlockMemory();
		desc.TempMemory			= GetTempMemory();

		desc.DX_DebugMode					= debugMode;
		desc.DX_GPUvalidation				= debugMode & gpuValidation;
		desc.DX_SynchronizedQueueValidation = debugMode & syncQueues;

		if (!RenderSystem->Initiate(desc)) {
			FK_LOG_ERROR("Failed to initiate renderSystem");

			return false;
		}

		InitiateAssetTable(GetBlockMemory());

		return true;
	}


	/************************************************************************************************/


	void EngineCore::Release()
	{
		for (auto Arg : CmdArguments)
			GetBlockMemory().free((void*)Arg);

		CmdArguments.Release();
		RenderSystem->Release();
		SceneNodeTable.Release();
		ReleaseGeometryTable();

		Threads.Release();

		Memory = nullptr;
	}


	/************************************************************************************************/


	void PushCmdArg(EngineCore* Engine, const char* str)
	{
		Engine->CmdArguments.push_back(str);
	}


}	/************************************************************************************************/
