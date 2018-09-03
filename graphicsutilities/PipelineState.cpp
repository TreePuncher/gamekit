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

#include "PipelineState.h"
#include "graphics.h"

namespace FlexKit
{

	PipelineStateTable::PipelineStateTable(iAllocator* Memory_IN, RenderSystem* RS_IN, ThreadManager* IN_Threads) :
		Device		{ RS_IN->pDevice },
		Memory		{ Memory_IN		 },
		RS			{ RS_IN			 },
		WorkQueue	{ IN_Threads	 }
	{
		States.SetFull();
	}


	/************************************************************************************************/


	void PipelineStateTable::ReleasePSOs()
	{
		FK_LOG_INFO("Releasing Pipeline State Objects");

		for (auto& s : States)
			SAFERELEASE(s.PSO);
	}


	/************************************************************************************************/


	bool PipelineStateTable::ReloadLoadPSO( EPIPELINESTATES State )
	{
		return false;
	}


	/************************************************************************************************/


	void PipelineStateTable::LoadPSOs()
	{
#if 0
		while (LoadsInProgress.size()) 
		{
#if 1
			std::chrono::system_clock Clock;
			auto Before = Clock.now();
			FINALLY
				auto After = Clock.now();
			auto Duration = chrono::duration_cast<chrono::milliseconds>(After - Before);
			FK_LOG_INFO("Shader Load Time: %d milliseconds", Duration.count());
			FINALLYOVER
#endif

			auto& LoadRequest = LoadsInProgress.first();
			std::lock_guard<mutex> lk(LoadRequest.Mutex);

			auto Res = StateLoaders[LoadRequest.State](RS);
			if (Res) {
				//SAFERELEASE(States->States[LoadRequest.State]); // Not Safe to do, just leak and disable during release mode
				States[LoadRequest.State] = Res;
			}
#if _DEBUG
			FK_LOG_INFO("Finished PSO Load");
#endif

			LoadRequest.CV.notify_all();

			if(LoadsInProgress.size())
				LoadsInProgress.pop_front();
		}
#endif


		int x = 0;

	}


	/************************************************************************************************/


	void PipelineStateTable::QueuePSOLoad(EPIPELINESTATES State, iAllocator* Allocator)
	{
#if 0
		if (!LoadsInProgress.size()) {

			auto Load = LoadsInProgress.push_back({ State });

			std::thread Thread([this, State]
			{
				LoadPSOs();
			});

			Thread.detach();
		}
		else
		{
			LoadsInProgress.push_back({ State });
		}
#endif
		if (States[(int)State].CurretState == PipelineStateObject::States::Unloaded)
		{
			States[(int)State].CurretState = PipelineStateObject::States::LoadQueued;
			LoadTask*	NewTask = &Allocator->allocate_aligned<LoadTask>(Allocator, this, State);

			WorkQueue->AddWork(NewTask, Allocator);
		}
	}


	/************************************************************************************************/
	
	
	ID3D12PipelineState*	PipelineStateTable::GetPSO( EPIPELINESTATES State )
	{
		auto PSO_State = States[(int)State].CurretState;

#if 0
		while (PSO_Out == nullptr)
		{
			if (LoadsInProgress.size())
			{
				LoadsInProgress.For_Each([&](auto& e)
				{
					if (e == State) {
						{
							std::unique_lock<mutex> lk(e.Mutex);
							e.CV.wait(lk, [&] {return States[(size_t)State] != nullptr; });
						}
						PSO_Out = States[(size_t)State];
						return false;
					}

					return true;
				});
			}
			else
				PSO_Out = States[(size_t)State];

			if (!PSO_Out)
				QueuePSOLoad(State);
		}
#endif

		while (true)
		{
			switch (PSO_State)
			{
			case PipelineStateObject::States::LoadInProgress: {
				std::mutex			M;
				std::unique_lock	UL(M);

				States[(int)State].CV.wait(UL, [&]{
					return
						!(States[(int)State].CurretState == PipelineStateObject::States::LoadInProgress); });
			}	break;

			case PipelineStateObject::States::Loaded:
				return States[(int)State].PSO;
				break;
			case PipelineStateObject::States::Failed:
			{
				// TODO: Handle Load Failures
			}	return nullptr;
			case PipelineStateObject::States::Unloaded:
			{
				// TODO: Load Here
				return nullptr;
			}

			default: 
				return nullptr;
			}
		}

		return nullptr;
	}


	/************************************************************************************************/


	bool GetPSOReadyState( RenderSystem* RS, PipelineStateTable* States, EPIPELINESTATES State )
	{
		return false;
	}


	/************************************************************************************************/


	void PipelineStateTable::RegisterPSOLoader( EPIPELINESTATES State, LOADSTATE_FN Loader )
	{
		StateLoaders[State] = Loader;
	}


	/************************************************************************************************/


	void LoadTask::Run()
	{
		std::chrono::system_clock Clock;
		auto Before = Clock.now();

		FINALLY
			auto After = Clock.now();
			auto Duration = chrono::duration_cast<chrono::milliseconds>(After - Before);
		FK_LOG_INFO("Shader Load Time: %d milliseconds", Duration.count());
		FINALLYOVER

		PST->States[State].CurretState = PipelineStateObject::States::LoadInProgress;

		auto Loader = PST->StateLoaders[State];
		auto res	= Loader(PST->RS);


		if (!res) {
			PST->States[State].CurretState = PipelineStateObject::States::Failed;
			FK_LOG_ERROR("PSO Load FAILED!");
			return;
		}


		FK_LOG_INFO("Finished PSO Load");

		PST->States[State].CurretState = PipelineStateObject::States::Loaded;
		PST->States[State].CV.notify_all();
	}


}	/************************************************************************************************/

