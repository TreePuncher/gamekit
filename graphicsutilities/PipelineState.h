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
#include "..\coreutilities\ThreadUtilities.h"

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
	class PipelineStateTable;


	enum EPIPELINESTATES
	{
		TERRAIN_DRAW_PSO = 0,
		TERRAIN_DRAW_PSO_DEBUG,
		TERRAIN_DRAW_WIRE_PSO, // Debug State
		TERRAIN_CULL_PSO,
		FORWARDDRAW,
		FORWARDDRAWINSTANCED,
		FORWARDDRAW_OCCLUDE,
		TILEDSHADING_SHADE,
		TILEDSHADING_LIGHTPREPASS,
		TILEDSHADING_COPYOUT,
		OCCLUSION_CULLING,
		SSREFLECTIONS,
		DRAW_PSO,
		DRAW_LINE_PSO,
		DRAW_LINE3D_PSO,
		DRAW_SPRITE_TEXT_PSO,
		EPSO_COUNT,
		EPSO_ERROR
	};

	typedef ID3D12PipelineState* LOADSTATE_FN(RenderSystem* RS);

	class LoadState
	{
	public:
		LoadState() noexcept : 
			State		{EPSO_ERROR},
			Finished	{false}
		{
			FK_ASSERT(false);
		}


		LoadState(const LoadState& rhs) noexcept :
			State		{rhs.State},
			Finished	{false}{}



		LoadState(EPIPELINESTATES PSOID) noexcept :
			State		{PSOID},
			Finished	{false} {}


		bool operator == (EPIPELINESTATES rhs) noexcept
		{
			return State == rhs;
		}

		const EPIPELINESTATES	State;
		mutex					Mutex;
		condition_variable		CV;
		bool					Finished;
	};


	/************************************************************************************************/


	class PipelineStateObject
	{
	public:
		ID3D12PipelineState* PSO	= nullptr;
		enum class States
		{
			LoadInProgress,
			LoadQueued,
			Loaded,
			Failed,
			Unloaded
		}CurretState = States::Unloaded;

		std::condition_variable		CV;
	};


	/************************************************************************************************/


	class LoadTask : public iWork
	{
	public:
		LoadTask(iAllocator* Memory, PipelineStateTable* IN_PST, EPIPELINESTATES IN_State) :
			iWork	{ Memory	},
			PST		{ IN_PST	},
			State	{ IN_State	}
		{}

		void Run() override;

		PipelineStateTable*			PST;
		EPIPELINESTATES				State;
	};


	/************************************************************************************************/


	class FLEXKITAPI PipelineStateTable
	{
	public:
		PipelineStateTable(iAllocator* Memory, RenderSystem* RS, ThreadManager* Threads);
		
		void ReleasePSOs();
		void LoadPSOs();

		void					QueuePSOLoad	( EPIPELINESTATES State, iAllocator* Allocator );
		bool					ReloadLoadPSO	( EPIPELINESTATES State );
		ID3D12PipelineState*	GetPSO			( EPIPELINESTATES State );

		void RegisterPSOLoader( EPIPELINESTATES State, LOADSTATE_FN Loader );

	private:
		static_vector<PipelineStateObject,	EPSO_COUNT>		States;
		static_vector<LOADSTATE_FN*,		EPSO_COUNT>		StateLoaders;

		ThreadManager*										WorkQueue;
		ID3D12Device*										Device;
		RenderSystem*										RS;
		iAllocator*											Memory;

		friend LoadTask;
	};


}	/************************************************************************************************/
#endif