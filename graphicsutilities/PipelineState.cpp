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

	PipelineStateTable::PipelineStateTable(iAllocator* Memory_IN, RenderSystem* RS_IN) :
		LoadsInProgress(Memory_IN),
		Device(RS_IN->pDevice),
		Memory(Memory_IN),
		RS(RS_IN)
	{
		States.SetFull();
	}


	/************************************************************************************************/


	void PipelineStateTable::ReleasePSOs()
	{
		for (auto s : States)
			SAFERELEASE(s);

		LoadsInProgress.Release();
	}


	/************************************************************************************************/


	bool PipelineStateTable::ReloadLoadPSO( EPIPELINESTATES State )
	{
		return false;
	}


	/************************************************************************************************/


	void PipelineStateTable::LoadPSOs()
	{
		while (LoadsInProgress.size()) {

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
	}


	/************************************************************************************************/


	void PipelineStateTable::QueuePSOLoad(EPIPELINESTATES State)
	{
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
	}


	/************************************************************************************************/
	
	
	ID3D12PipelineState*	PipelineStateTable::GetPSO( EPIPELINESTATES State )
	{
		ID3D12PipelineState*	PSO_Out = nullptr;
		PSO_Out = States[(int)State];

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

		return PSO_Out;
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
}
