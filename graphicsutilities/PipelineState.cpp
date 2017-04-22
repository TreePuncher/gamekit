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

	/************************************************************************************************/


	PipelineStateTable* CreatePSOTable( RenderSystem* RS )
	{
		PipelineStateTable* out		= &RS->Memory->allocate_aligned<PipelineStateTable>();
		out->LoadsInProgress.Memory = RS->Memory;
		out->Device					= RS->pDevice;

		out->States.SetFull();

		return out;
	}


	/************************************************************************************************/


	void ReleasePipelineStates(RenderSystem* RS)
	{
		for (auto s : RS->States->States)
			SAFERELEASE(s);

		RS->Memory->free(RS->States);
	}


	/************************************************************************************************/


	bool LoadPipelineStates( RenderSystem* RS )
	{
		return false;
	}


	/************************************************************************************************/


	bool ReloadLoadPipelineState( RenderSystem* RS, PipelineStateTable* States, EPIPELINESTATES State )
	{
		return false;
	}


	/************************************************************************************************/


	void LoadPSOs( RenderSystem* RS )
	{
		PipelineStateTable*	States = RS->States;
		while (States->LoadsInProgress.size()) {

#if 1
			std::chrono::system_clock Clock;
			auto Before = Clock.now();
			FINALLY
				auto After = Clock.now();
			auto Duration = chrono::duration_cast<chrono::milliseconds>(After - Before);
			std::cout << "Shader Load Time: " << Duration.count() << "milliseconds\n";
			FINALLYOVER
#endif

			auto& LoadRequest = States->LoadsInProgress.first();
			std::lock_guard<mutex> lk(LoadRequest.Mutex);

			auto Res = States->StateLoaders[LoadRequest.State](RS);
			if (Res) {
				//SAFERELEASE(States->States[LoadRequest.State]); // Not Safe to do, just leak and disable during release mode
				States->States[LoadRequest.State] = Res;
			}
			std::cout << "Finished PSO Load\n";
			LoadRequest.CV.notify_all();

			States->LoadsInProgress.pop_front();
		}
	}


	/************************************************************************************************/


	void QueuePSOLoad(RenderSystem* RS, EPIPELINESTATES State)
	{
		PipelineStateTable*	States = RS->States;

		if (!RS->States->LoadsInProgress.size()) {

			auto Load = States->LoadsInProgress.push_back({ State });

			std::thread Thread([RS, State]
			{
				LoadPSOs(RS);
			});

			Thread.detach();
		}
		else
		{
			States->LoadsInProgress.push_back({ State });
		}
	}


	/************************************************************************************************/
	
	
	ID3D12PipelineState*	GetPSO( RenderSystem* RS, EPIPELINESTATES State )
	{
		ID3D12PipelineState*	PSO_Out = nullptr;
		PipelineStateTable*		States  = RS->States;
		PSO_Out = States->States[(int)State];

		while (PSO_Out == nullptr)
		{
			if (States->LoadsInProgress.size())
			{
				States->LoadsInProgress.For_Each([&](auto& e)
				{
					if (e == State) {
						{
							std::unique_lock<mutex> lk(e.Mutex);
							e.CV.wait(lk, [&] {return States->States[(size_t)State] != nullptr; });
						}
						PSO_Out = States->States[(size_t)State];
						return false;
					}

					return true;
				});
			}
			else
				PSO_Out = States->States[(size_t)State];
		}

		return PSO_Out;
	}


	/************************************************************************************************/


	bool GetPSOReadyState( RenderSystem* RS, PipelineStateTable* States, EPIPELINESTATES State )
	{
		return false;
	}


	/************************************************************************************************/


	void RegisterPSOLoader( RenderSystem* RS, PipelineStateTable* States, EPIPELINESTATES State, LOADSTATE_FN Loader )
	{
		States->StateLoaders[State] = Loader;
	}


	/************************************************************************************************/
}
