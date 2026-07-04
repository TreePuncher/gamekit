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


#include "dxPipelineState.hpp"
#include "dxRenderSystem.hpp"
#include <RenderSystemInterface.hpp>


namespace dx_Internal
{
	/************************************************************************************************/


	DevicePipelineState_ptr dxPipelineState::GetDevicePipeState() const
	{
		return state;
	}

	const IPipelineInterface* dxPipelineState::GetInterface() const noexcept
	{
		return rootSignature;
	}

	/************************************************************************************************/

	bool dxPipelineStateObject::changeState(const dxPipelineStateObject::PSO_States newState)
	{
		auto currentState = state.load(std::memory_order_acquire);

		if (currentState == dxPipelineStateObject::PSO_States::Unloaded ||
			currentState == dxPipelineStateObject::PSO_States::Loaded)
		{
			if (state.compare_exchange_strong(currentState, newState, std::memory_order_release))
			{
				return true;
			}
		}

		return false;
	}


	/************************************************************************************************/


	void dxPipelineStateObject::Release(iAllocator* allocator)
	{
		if (PSO.state)
			PSO.state->Release();

		PSO.state = nullptr;

		if (auto _ptr = next; _ptr)
			_ptr->Release(allocator);

		if (pipelineInterface)
		{
			const_cast<IPipelineInterface*>(pipelineInterface)->Release();
			pipelineInterface = nullptr;
		}

		allocator->free(this);
	}


	/************************************************************************************************/


	void dxPipelineStateObject::WaitForLoad(iAllocator& temp)
	{
		switch (state)
		{
		case PSO_States::LoadQueued:
		case PSO_States::LoadInProgress: 
		{
			std::mutex			M;
			std::unique_lock	UL(M);

			const_cast<condition_variable&>(CV).wait(UL, [&]{
				return
					!(state == PSO_States::LoadInProgress); });
		}	break;

		case PSO_States::ReLoadQueued:
		case PSO_States::Loaded:
		{
			return;
		}	break;
		case PSO_States::Failed:
		{
			FK_LOG_ERROR("TRYING TO LOAD A UNLOADABLE PSO!");
			// TODO: Handle Load Failures
		}	return;
		case PSO_States::Unloaded:
		{
			while (true)
			{
				state		= PSO_States::LoadInProgress;
				auto res	= loader(IRenderSystem::GetInstance(), temp);
				stale		= false;

				if (!res.pipelineState) {
					state = PSO_States::Failed;
					FK_LOG_ERROR("PSO Load FAILED!");
					return;
				}

				if (stale && loader != loader)
				{
					FK_LOG_2("Stale PSO LOADED!");
					continue;
				}

				FK_LOG_2("Finished PSO Load");

				state				= PSO_States::Loaded;
				PSO.state			= res.pipelineState.As<ID3D12PipelineState>();
				PSO.rootSignature	= res.pipelineInterface;
				pipelineInterface	= static_cast<const IPipelineInterface*>(res.pipelineInterface);
				CV.notify_all();
				return;
			}
		}

		default: 
			return;
		}
	}


	/************************************************************************************************/


	dxPipelineStateTable::dxPipelineStateTable(iAllocator* IN_allocator, IRenderSystem* IN_RS, ThreadManager* IN_Threads) :
		allocator	{ IN_allocator	 },
		RS			{ IN_RS			 },
		WorkQueue	{ IN_Threads	 }
	{
		States.SetFull();
	}


	/************************************************************************************************/


	void dxPipelineStateTable::ReleasePSOs()
	{
		FK_LOG_9("Releasing Pipeline State Objects");

		for (auto& s : States)
		{
			if (s)
				s->Release(allocator);
		}
	}


	/************************************************************************************************/


	bool dxPipelineStateTable::QueuePSOLoad(PSOHandle handle, iAllocator* queueAllocator)
	{
		dxPipelineStateObject* PSO = _GetStateObject(handle);
		if (!PSO)
			return false;

		if (PSO->state == dxPipelineStateObject::PSO_States::LoadInProgress ||
			PSO->state == dxPipelineStateObject::PSO_States::LoadQueued ||
			PSO->state == dxPipelineStateObject::PSO_States::ReLoadQueued )
				return false;

		auto& NewTask = queueAllocator->allocate_aligned<dxShaderLoadTask>(queueAllocator, this, PSO);

		while (true)
		{
			auto state = PSO->state.load(std::memory_order_acquire);

			if(	(	state == dxPipelineStateObject::PSO_States::Unloaded || 
					state == dxPipelineStateObject::PSO_States::Loaded ) &&
				(	state != dxPipelineStateObject::PSO_States::LoadInProgress &&
					state != dxPipelineStateObject::PSO_States::LoadQueued))
			{
				auto prevPSO								= PSO->PSO;
				dxPipelineStateObject::PSO_States newState	= 
					(prevPSO.state == nullptr) ?
						dxPipelineStateObject::PSO_States::LoadQueued : 
						dxPipelineStateObject::PSO_States::ReLoadQueued;


				if (PSO->changeState(newState))
					WorkQueue->AddWork(&NewTask);
				else
					continue;

				return true;
			}

			else
			{
				queueAllocator->release(NewTask); // Failed to schedule task, release task
				return false;
			}
		}


		// un-reachable
		FK_ASSERT(false, "!!!!!!!!! The unreachable has been reached !!!!!!!!!");
		exit(-1);

		return false;
	}


	/************************************************************************************************/
	
	
	dxPipelineState* dxPipelineStateTable::GetPSO(PSOHandle handle, iAllocator& temp)
	{
		while (true)
		{
			dxPipelineStateObject* PSO = _GetStateObject(handle);

			if (!PSO)
				return nullptr;

			switch (PSO->state)
			{
			case dxPipelineStateObject::PSO_States::LoadQueued:
			case dxPipelineStateObject::PSO_States::LoadInProgress: 
			{
				std::mutex			M;
				std::unique_lock	UL(M);

				PSO->CV.wait(UL, [&]{
					return
						!(PSO->state == dxPipelineStateObject::PSO_States::LoadInProgress); });
			}	break;

			case dxPipelineStateObject::PSO_States::ReLoadQueued:
			case dxPipelineStateObject::PSO_States::Loaded:
			{
				return &PSO->PSO;
			}	break;
			case dxPipelineStateObject::PSO_States::Failed:
			{
				FK_LOG_ERROR("TRYING TO LOAD A UNLOADABLE PSO!");
				// TODO: Handle Load Failures
			}	return nullptr;
			case dxPipelineStateObject::PSO_States::Unloaded:
			{
				while (true)
				{
					PSO->state		= dxPipelineStateObject::PSO_States::LoadInProgress;
					auto& loader	= PSO->loader;
					auto res		= loader(*RS, temp);

					PSO->stale = false;

					if (!res.pipelineState) {
						PSO->state = dxPipelineStateObject::PSO_States::Failed;
						FK_LOG_ERROR("PSO Load FAILED!");
						return nullptr;
					}

					if (PSO->stale && PSO->loader != loader)
					{
						FK_LOG_2("Stale PSO LOADED!");
						continue;
					}

					FK_LOG_2("Finished PSO Load");

					PSO->state				= dxPipelineStateObject::PSO_States::Loaded;
					PSO->PSO.state			= res.pipelineState.As<ID3D12PipelineState>();
					PSO->PSO.rootSignature	= res.pipelineInterface;
					PSO->pipelineInterface	= static_cast<const IPipelineInterface*>(res.pipelineInterface);
					PSO->CV.notify_all();

					return &PSO->PSO;
				}
			}

			default: 
				return nullptr;
			}
		}

		return nullptr;
	}


	/************************************************************************************************/


	IPipelineInterface const * const dxPipelineStateTable::GetPSORootSig(PSOHandle handle) const
	{
		auto PSO = _GetStateObject(handle);
		return PSO->pipelineInterface;
	}


	/************************************************************************************************/


	dxPipelineStateObject* dxPipelineStateTable::GetPSOObject(PSOHandle handle) const
	{
		return _GetStateObject(handle);
	}


	/************************************************************************************************/


	bool GetPSOReadyState(IRenderSystem* RS, dxPipelineStateTable* States, PSOHandle State )
	{
		FK_ASSERT(false, "GETPSOREADYSTATE");

		return false;
	}


	/************************************************************************************************/


	void dxPipelineStateTable::RegisterPSOLoader(PSOHandle handle, LOADSTATE_FN fn)
	{
		dxPipelineStateObject* PSO = _GetNearestStateObject(handle);

		if (!PSO)
		{
			FK_LOG_2("Adding State Node!");
			// add new node
			PSO					= &allocator->allocate<dxPipelineStateObject>();
			PSO->id				= handle;
			PSO->state			= dxPipelineStateObject::PSO_States::Unloaded;
			PSO->loader			= std::move(fn);
			_AddStateObject(PSO);
			return;
		}

		if (PSO->id == InvalidHandle)
		{
			// First node in chain
			PSO->id				= handle;
			PSO->state			= dxPipelineStateObject::PSO_States::Unloaded;
			PSO->loader			= std::move(fn);
			return;
		}

		if (PSO->id == handle)
		{
			// node exists
			PSO->stale			= true;
			PSO->loader			= std::move(fn);
			return;
		}

		if (!PSO->next)
		{
			FK_LOG_2("Adding State Node!");
			// add new node
			PSO					= &allocator->allocate<dxPipelineStateObject>();
			PSO->id				= handle;
			PSO->state			= dxPipelineStateObject::PSO_States::Unloaded;
			PSO->loader			= std::move(fn);
			_AddStateObject(PSO);
			return;
		}
	}


	/************************************************************************************************/


	dxPipelineStateObject* dxPipelineStateTable::_GetStateObject(PSOHandle handle)
	{
		dxPipelineStateObject* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle; PSO = PSO->next);

		return PSO;
	}


	/************************************************************************************************/


	dxPipelineStateObject* dxPipelineStateTable::_GetNearestStateObject(PSOHandle handle)
	{
		dxPipelineStateObject* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle && PSO->next != nullptr; PSO = PSO->next);

		return PSO;
	}


	/************************************************************************************************/
	// Less then Ideal

	dxPipelineStateObject* dxPipelineStateTable::_GetStateObject(PSOHandle handle) const
	{
		dxPipelineStateObject* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle; PSO = PSO->next);

		return PSO;
	}


	dxPipelineStateObject const*	dxPipelineStateTable::_GetNearestStateObject(PSOHandle handle) const
	{
		dxPipelineStateObject const* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle && PSO->next != nullptr; PSO = PSO->next);

		return PSO;
	}


	/************************************************************************************************/


	bool dxPipelineStateTable::_AddStateObject(dxPipelineStateObject* PSO)
	{
		const auto handle = PSO->id;

		dxPipelineStateObject** node = &States[handle.INDEX % States.size()];

		constexpr size_t tryCount = 4;

		for(size_t i = 0; i < tryCount; ++i)
		{
			for (; *node; node = &(*node)->next)
				std::atomic_thread_fence(std::memory_order_acquire);

			*node = PSO;

			std::atomic_thread_fence(std::memory_order_release);
			std::atomic_thread_fence(std::memory_order_acquire);

			if (*node == PSO)
				return true;
		}

		return false;
	}


	/************************************************************************************************/


	dxShaderLoadTask::dxShaderLoadTask(iAllocator* IN_allocator, dxPipelineStateTable* IN_PST, dxPipelineStateObject* IN_PSO) :
			allocator	{ IN_allocator	},
			iWork		{ IN_allocator	},
			RS			{ IN_PST->RS	},
			PSO			{ IN_PSO		}
	{
		_debugID = "Load PSO task";
	}


	/************************************************************************************************/


	void dxShaderLoadTask::Run(iAllocator& threadLocalAllocator)
	{
		ProfileFunction();

		std::chrono::system_clock Clock;
		auto Before = Clock.now();

		EXITSCOPE(
			auto After      = Clock.now();
			auto Duration   = std::chrono::duration_cast<std::chrono::milliseconds>(After - Before);
			FK_LOG_1("Shader Load Time: %d milliseconds", Duration.count());
		);
		
		const auto previousPSO		= PSO->PSO;
		const auto previousState	= PSO->state.load();

		if(previousState != dxPipelineStateObject::PSO_States::ReLoadQueued)
			PSO->state = dxPipelineStateObject::PSO_States::LoadInProgress;

		while (true)
		{
			auto& loader = PSO->loader;
			PSO->stale	= false;

			if (!loader) // No Loader Registered!
			{
				PSO->state = dxPipelineStateObject::PSO_States::Unloaded;
				PSO->CV.notify_all();
				FK_LOG_WARNING("Tried to Load PSO with no loader registered!");
				return;
			}

			auto res = loader(*RS, threadLocalAllocator);

			if (!res.pipelineState) {
				if (previousState != dxPipelineStateObject::PSO_States::ReLoadQueued)
					PSO->state = dxPipelineStateObject::PSO_States::Failed;

				FK_LOG_ERROR("PSO Load FAILED!");
				return;
			}

			PSO->PSO.state			= res.pipelineState.As<ID3D12PipelineState>();
			PSO->PSO.rootSignature	= static_cast<const IPipelineInterface*>(res.pipelineInterface);
			PSO->pipelineInterface	= static_cast<const IPipelineInterface*>(res.pipelineInterface);

			if (PSO->stale && loader != PSO->loader)
				continue;

			PSO->state	= dxPipelineStateObject::PSO_States::Loaded;
			PSO->CV.notify_all();

			// TODO: FIX THIS LEAK!
			if (previousPSO.state)
			{
				int x = 0;
				//	previousPSO->Release();
			}

			break;
		}

		FK_LOG_INFO("Finished PSO Load");
	}


	/************************************************************************************************/


	void dxShaderLoadTask::Release()
	{
		allocator->free(this);
	}


}	/************************************************************************************************/

