#ifndef PIPELINESTATE_H
#define PIPELINESTATE_H


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

#include "buildsettings.h"
#include "containers.h"
#include "ThreadUtilities.h"
#include "Handle.h"
#include "ResourceHandles.h"
#include <atomic>
#include <condition_variable>

struct ID3D12PipelineStateObject;

using std::atomic_bool;
using std::condition_variable;
using std::mutex;

struct ID3D12Device;
struct ID3D12PipelineState;


namespace FlexKit
{
	class RenderSystem;
	class RootSignature;
	class PipelineStateTable;


	struct LoadPipelineStateRes
	{
		ID3D12PipelineState*	pipelineState;
		const RootSignature*	rootSignature;
	};

	using LOADSTATE_FN = FlexKit::TypeErasedCallable<LoadPipelineStateRes (RenderSystem*, iAllocator&), 32>;

	/************************************************************************************************/

	class PipelineStateObject
	{
	public:
		enum class PSO_States
		{
			LoadInProgress,
			LoadQueued,
			ReLoadQueued,
			Loaded,
			Failed,
			Stale,
			Unloaded,
			Undefined
		};

		bool changeState(const PipelineStateObject::PSO_States newState);
		void Release(iAllocator* allocator);

		void WaitForLoad(iAllocator& temp);

		ID3D12PipelineState*				PSO				= nullptr;
		PSOHandle							id				= InvalidHandle;
		bool								stale			= false;
		std::atomic<PSO_States>				state			= PSO_States::Unloaded;
		PipelineStateObject*				next			= nullptr;
		const RootSignature*				rootSignature	= nullptr;
		LOADSTATE_FN						loader;
		std::condition_variable				CV;
	};


	/************************************************************************************************/


	class LoadTask : public iWork
	{
	public:
		LoadTask(iAllocator* IN_allocator, PipelineStateTable* IN_PST, PipelineStateObject* IN_PSO);

		void Run(iAllocator& allocator)		override;
		void Release()	override;


		RenderSystem*			RS;
		PipelineStateObject*	PSO;
		iAllocator*				allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI PipelineStateTable
	{
	public:
		PipelineStateTable(iAllocator* allocator, RenderSystem* RS, ThreadManager* Threads);
		
		void ReleasePSOs();

		void							RegisterPSOLoader	(PSOHandle, LOADSTATE_FN);
		bool							QueuePSOLoad		(PSOHandle, iAllocator*);

		ID3D12PipelineState*			GetPSO			(PSOHandle, iAllocator& temp);
		RootSignature const * const 	GetPSORootSig	(PSOHandle) const;
		PipelineStateObject*			GetPSOObject	(PSOHandle) const;


	private:
		PipelineStateObject*	_GetStateObject			(PSOHandle);
		PipelineStateObject*	_GetNearestStateObject	(PSOHandle);

		PipelineStateObject*		_GetStateObject			(PSOHandle) const;
		PipelineStateObject const*	_GetNearestStateObject	(PSOHandle) const;

		bool					_AddStateObject			(PipelineStateObject*	PSO);

		static_vector<PipelineStateObject*,	128>			States;

		ThreadManager*										WorkQueue;
		ID3D12Device*										Device;
		RenderSystem*										RS;
		iAllocator*											allocator;

		friend LoadTask;
	};


}	/************************************************************************************************/
#endif
