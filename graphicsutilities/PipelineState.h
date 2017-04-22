#ifndef PIPELINESTATE_H
#define PIPELINESTATE_H


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

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"

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
	struct RenderSystem;


	enum EPIPELINESTATES
	{
		TERRAIN_DRAW_PSO = 0,
		TERRAIN_DRAW_PSO_DEBUG,
		TERRAIN_DRAW_WIRE_PSO, // Debug State
		TERRAIN_CULL_PSO,
		TILEDSHADING_SHADE,
		TILEDSHADING_LIGHTPREPASS,
		TILEDSHADING_COPYOUT,
		OCCLUSION_CULLING,
		EPSO_COUNT,
		EPSO_ERROR
	};

	typedef ID3D12PipelineState* LOADSTATE_FN(RenderSystem* RS);

	struct LoadState
	{
		LoadState() : State(EPSO_ERROR)
		{
			FK_ASSERT(false);
		}

		LoadState(const LoadState& rhs) : State( rhs.State )
		{

		}

		LoadState(EPIPELINESTATES s) : State(s)
		{

		}

		bool operator == (EPIPELINESTATES rhs)
		{
			return State == rhs;
		}

		const EPIPELINESTATES	State;
		mutex					Mutex;
		condition_variable		CV;
		bool					Finished;
	};


	struct PipelineStateTable
	{
		static_vector<ID3D12PipelineState*, EPSO_COUNT>		States;
		static_vector<LOADSTATE_FN*, EPSO_COUNT>			StateLoaders;
		SL_list<LoadState>									LoadsInProgress;
		ID3D12Device*										Device;
	};


	/************************************************************************************************/


	FLEXKITAPI PipelineStateTable*		CreatePSOTable			( RenderSystem* RS );
	FLEXKITAPI void						ReleasePipelineStates	( RenderSystem* RS );

	FLEXKITAPI bool						LoadPipelineStates	( RenderSystem* RS );


	FLEXKITAPI ID3D12PipelineState*		GetPSO				( RenderSystem* RS, EPIPELINESTATES State );
	FLEXKITAPI void 					QueuePSOLoad		( RenderSystem* RS, EPIPELINESTATES State ); // Forces a PSO load

	FLEXKITAPI bool						ReloadLoadPSO		( RenderSystem* RS, PipelineStateTable* States, EPIPELINESTATES State );
	FLEXKITAPI void						RegisterPSOLoader	( RenderSystem* RS, PipelineStateTable* States, EPIPELINESTATES State, LOADSTATE_FN Loader );


	/************************************************************************************************/

}
#endif