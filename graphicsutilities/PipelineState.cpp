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

#include "PipelineState.h"
#include "graphics.h"

namespace FlexKit
{
    /************************************************************************************************/


    bool PipelineStateObject::changeState(const PipelineStateObject::PSO_States newState)
    {
        auto currentState = state.load(std::memory_order_acquire);

        if (currentState == PipelineStateObject::PSO_States::Unloaded ||
            currentState == PipelineStateObject::PSO_States::Loaded)
        {
            if (state.compare_exchange_strong(currentState, newState, std::memory_order_release))
            {
                return true;
            }
        }

        return false;
    }


    /************************************************************************************************/


    void PipelineStateObject::Release(iAllocator* allocator)
    {
        if(PSO)
            PSO->Release();

        PSO = nullptr;

        if (auto _ptr = next; _ptr)
            _ptr->Release(allocator);

        allocator->free(this);
    }


    /************************************************************************************************/


	PipelineStateTable::PipelineStateTable(iAllocator* IN_allocator, RenderSystem* IN_RS, ThreadManager* IN_Threads) :
		Device		{ IN_RS->pDevice },
		allocator	{ IN_allocator	 },
		RS			{ IN_RS			 },
		WorkQueue	{ IN_Threads	 }
	{
		States.SetFull();
	}


	/************************************************************************************************/


	void PipelineStateTable::ReleasePSOs()
	{
		FK_LOG_INFO("Releasing Pipeline State Objects");

		for (auto& s : States)
		{
			if (s)
				s->Release(allocator);
		}
	}


	/************************************************************************************************/


	bool PipelineStateTable::QueuePSOLoad(PSOHandle handle, iAllocator* queueAllocator)
	{
		PipelineStateObject* PSO = _GetStateObject(handle);
		if (!PSO)
			return false;

		if (PSO->state == PipelineStateObject::PSO_States::LoadInProgress ||
			PSO->state == PipelineStateObject::PSO_States::LoadQueued ||
			PSO->state == PipelineStateObject::PSO_States::ReLoadQueued )
				return false;

		auto& NewTask = queueAllocator->allocate_aligned<LoadTask>(queueAllocator, this, PSO);

		while (true)
		{
			auto state = PSO->state.load(std::memory_order_acquire);

			if(	(	state == PipelineStateObject::PSO_States::Unloaded || 
					state == PipelineStateObject::PSO_States::Loaded ) &&
				(	state != PipelineStateObject::PSO_States::LoadInProgress &&
					state != PipelineStateObject::PSO_States::LoadQueued))
			{
				auto prevPSO								= PSO->PSO;
				PipelineStateObject::PSO_States newState	= 
					(prevPSO == nullptr) ?
						PipelineStateObject::PSO_States::LoadQueued : 
						PipelineStateObject::PSO_States::ReLoadQueued;


				if (PSO->changeState(newState))
					WorkQueue->AddWork(&NewTask, queueAllocator);
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
	
	
	ID3D12PipelineState*	PipelineStateTable::GetPSO(PSOHandle handle)
	{
		while (true)
		{
			PipelineStateObject* PSO = _GetStateObject(handle);

			if (!PSO)
				return nullptr;

			switch (PSO->state)
			{
			case PipelineStateObject::PSO_States::LoadInProgress: 
			{
				std::mutex			M;
				std::unique_lock	UL(M);

				PSO->CV.wait(UL, [&]{
					return
						!(PSO->state == PipelineStateObject::PSO_States::LoadInProgress); });
			}	break;

			case PipelineStateObject::PSO_States::ReLoadQueued:
			case PipelineStateObject::PSO_States::Loaded:
			{
				return PSO->PSO;
			}	break;
			case PipelineStateObject::PSO_States::Failed:
			{
				FK_LOG_ERROR("TRYING TO LOAD A UNLOADABLE PSO!");
				// TODO: Handle Load Failures
			}	return nullptr;
			case PipelineStateObject::PSO_States::Unloaded:
			{
				while (true)
				{
					PSO->state = PipelineStateObject::PSO_States::LoadInProgress;

					auto loader = PSO->loader;
					auto res	= loader(RS);

					PSO->stale = false;

					if (!res) {
						PSO->state = PipelineStateObject::PSO_States::Failed;
						FK_LOG_ERROR("PSO Load FAILED!");
						return nullptr;
					}

					if (PSO->stale && PSO->loader != loader)
					{
						FK_LOG_INFO("Stale PSO LOADED!");
						continue;
					}

					FK_LOG_INFO("Finished PSO Load");

					PSO->state = PipelineStateObject::PSO_States::Loaded;
					PSO->PSO = res;
					PSO->CV.notify_all();

					return res;
				}
			}

			default: 
				return nullptr;
			}
		}

		return nullptr;
	}


	/************************************************************************************************/


	RootSignature const * const PipelineStateTable::GetPSORootSig(PSOHandle handle) const
	{
		auto PSO = _GetStateObject(handle);
		return PSO->rootSignature;
	}

	/************************************************************************************************/


	bool GetPSOReadyState( RenderSystem* RS, PipelineStateTable* States, PSOHandle State )
	{
		FK_ASSERT(false, "GETPSOREADYSTATE");

		return false;
	}


	/************************************************************************************************/


	void PipelineStateTable::RegisterPSOLoader( PSOHandle handle, PipelineStateDescription PSODesc)
	{
		PipelineStateObject* PSO = _GetNearestStateObject(handle);

		if (!PSO)
		{
			FK_LOG_INFO("Adding State Node!");
			// add new node
			PSO			= &allocator->allocate<PipelineStateObject>();
			PSO->id		= handle;
			PSO->state	= PipelineStateObject::PSO_States::Unloaded;
			PSO->loader			= PSODesc.loadState;
			PSO->rootSignature	= PSODesc.rootSignature;
			_AddStateObject(PSO);
			return;
		}

		if (PSO->id == InvalidHandle_t)
		{
			// First node in chain
			PSO->id				= handle;
			PSO->state			= PipelineStateObject::PSO_States::Unloaded;
			PSO->loader			= PSODesc.loadState;
			PSO->rootSignature	= PSODesc.rootSignature;
			return;
		}

		if (PSO->id == handle)
		{
			// node exists
			PSO->stale			= true;
			PSO->loader			= PSODesc.loadState;
			PSO->rootSignature	= PSODesc.rootSignature;
			return;
		}

		if (!PSO->next)
		{
			FK_LOG_INFO("Adding State Node!");
			// add new node
			PSO					= &allocator->allocate<PipelineStateObject>();
			PSO->id				= handle;
			PSO->state			= PipelineStateObject::PSO_States::Unloaded;
			PSO->loader			= PSODesc.loadState;
			PSO->rootSignature	= PSODesc.rootSignature;
			_AddStateObject(PSO);
			return;
		}
	}


	/************************************************************************************************/


	PipelineStateObject* PipelineStateTable::_GetStateObject(PSOHandle handle)
	{
		PipelineStateObject* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle; PSO = PSO->next);

		return PSO;
	}


	/************************************************************************************************/


	PipelineStateObject* PipelineStateTable::_GetNearestStateObject(PSOHandle handle)
	{
		PipelineStateObject* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle && PSO->next != nullptr; PSO = PSO->next);

		return PSO;
	}


	/************************************************************************************************/
	// Less then Ideal

	PipelineStateObject const*	PipelineStateTable::_GetStateObject(PSOHandle handle) const
	{
		PipelineStateObject const* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle; PSO = PSO->next);

		return PSO;
	}


	PipelineStateObject const*	PipelineStateTable::_GetNearestStateObject(PSOHandle handle) const
	{
		PipelineStateObject const* PSO = States[handle.INDEX % States.size()];
		for (; PSO && PSO->id != handle && PSO->next != nullptr; PSO = PSO->next);

		return PSO;
	}


	/************************************************************************************************/


	bool PipelineStateTable::_AddStateObject(PipelineStateObject* PSO)
	{
		const auto handle = PSO->id;


		PipelineStateObject** node = &States[handle.INDEX % States.size()];

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


	LoadTask::LoadTask(iAllocator* IN_allocator, PipelineStateTable* IN_PST, PipelineStateObject* IN_PSO) :
			allocator	{ IN_allocator	},
			iWork		{ IN_allocator	},
			RS			{ IN_PST->RS	},
			PSO			{ IN_PSO		}{}


	/************************************************************************************************/


	void LoadTask::Run()
	{
		std::chrono::system_clock Clock;
		auto Before = Clock.now();

		EXITSCOPE(
			auto After = Clock.now();
			auto Duration = chrono::duration_cast<chrono::milliseconds>(After - Before);
			FK_LOG_INFO("Shader Load Time: %d milliseconds", Duration.count());
		);
		
		const auto previousPSO = PSO->PSO;
        const auto previousState = PSO->state.load();

        if(previousState != PipelineStateObject::PSO_States::ReLoadQueued)
		    PSO->state = PipelineStateObject::PSO_States::LoadInProgress;

		while (true)
		{
			auto loader = PSO->loader;
			PSO->stale	= false;

			if (!loader) // No Loader Registered!
			{
				PSO->state = PipelineStateObject::PSO_States::Unloaded;
				PSO->CV.notify_all();
				FK_LOG_WARNING("Tried to Load PSO with no loader registered!");
				return;
			}

			auto res = loader(RS);

			if (!res) {
                if (previousState != PipelineStateObject::PSO_States::ReLoadQueued)
    				PSO->state = PipelineStateObject::PSO_States::Failed;

				FK_LOG_ERROR("PSO Load FAILED!");
				return;
			}

			PSO->PSO	= res;

			if (PSO->stale && loader != PSO->loader)
				continue;

			PSO->state	= PipelineStateObject::PSO_States::Loaded;
			PSO->CV.notify_all();

            // TODO: FIX THIS LEAK!
            if (previousPSO)
                int x = 0;
			//	previousPSO->Release();

			break;
		}

		FK_LOG_INFO("Finished PSO Load");
	}


	/************************************************************************************************/


	void LoadTask::Release()
	{
		allocator->free(this);
	}


}	/************************************************************************************************/

