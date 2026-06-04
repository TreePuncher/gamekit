#ifndef PIPELINESTATE_H
#define PIPELINESTATE_H


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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

#include "BuildSettings.hpp"
#include "Containers.hpp"
#include "ThreadUtilities.hpp"
#include "Handle.hpp"
#include "ResourceHandles.hpp"
#include <atomic>
#include <condition_variable>
#include <RenderSystemInterface.hpp>

struct ID3D12PipelineStateObject;

using std::atomic_bool;
using std::condition_variable;
using std::mutex;

struct ID3D12Device;
struct ID3D12PipelineState;


namespace dx_Internal
{
	using namespace FlexKit;

	class dxRenderSystem;
	class RootSignature;
	class dxPipelineStateTable;


	/************************************************************************************************/


	class dxPipelineState : public IPipelineState
	{
	public:
		ID3D12PipelineState*		state			= nullptr;
		const IPipelineInterface*	rootSignature	= nullptr;

		DevicePipelineState_ptr		GetDevicePipeState() const final;
		const IPipelineInterface*	GetInterface() const noexcept final;
	};

	class dxPipelineStateObject
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

		bool changeState(const dxPipelineStateObject::PSO_States newState);
		void Release(iAllocator* allocator);

		void WaitForLoad(iAllocator& temp);

		dxPipelineState						PSO;
		PSOHandle							id					= InvalidHandle;
		bool								stale				= false;
		std::atomic<PSO_States>				state				= PSO_States::Unloaded;
		dxPipelineStateObject*				next				= nullptr;
		const IPipelineInterface*			pipelineInterface	= nullptr;
		LOADSTATE_FN						loader;
		std::condition_variable				CV;
	};


	/************************************************************************************************/


	class dxShaderLoadTask : public iWork
	{
	public:
		dxShaderLoadTask(iAllocator* IN_allocator, dxPipelineStateTable* IN_PST, dxPipelineStateObject* IN_PSO);

		void Run(iAllocator& allocator)		override;
		void Release()	override;


		IRenderSystem*			RS;
		dxPipelineStateObject*	PSO;
		iAllocator*				allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI dxPipelineStateTable
	{
	public:
		dxPipelineStateTable(iAllocator* allocator, IRenderSystem* RS, ThreadManager* Threads);
		
		void ReleasePSOs();

		void							RegisterPSOLoader	(PSOHandle, LOADSTATE_FN);
		bool							QueuePSOLoad		(PSOHandle, iAllocator*);

		dxPipelineState*					GetPSO			(PSOHandle, iAllocator& temp);
		IPipelineInterface const * const 	GetPSORootSig	(PSOHandle) const;
		dxPipelineStateObject*				GetPSOObject	(PSOHandle) const;


	private:
		dxPipelineStateObject*			_GetStateObject			(PSOHandle);
		dxPipelineStateObject*			_GetNearestStateObject	(PSOHandle);

		dxPipelineStateObject*			_GetStateObject			(PSOHandle) const;
		dxPipelineStateObject const*	_GetNearestStateObject	(PSOHandle) const;

		bool							_AddStateObject			(dxPipelineStateObject*	PSO);

		static_vector<dxPipelineStateObject*,	128>		States;

		ThreadManager*										WorkQueue;
		IRenderSystem*										RS;
		iAllocator*											allocator;

		friend dxShaderLoadTask;
	};


}	/************************************************************************************************/
#endif
