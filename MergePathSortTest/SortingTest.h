#pragma once

#include <Application.h>
#include <Win32Graphics.h>
#include <filesystem>
#include <expected>
#include <DebugUI.h>

class SortTest : public FlexKit::FrameworkState
{
public:
	SortTest(FlexKit::GameFramework& IN_framework);
	~SortTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	ID3D12PipelineState* CreateInitiateDataPSO();
	ID3D12PipelineState* CreateLocalSortPSO();
	ID3D12PipelineState* CreateMergePathPSO();
	ID3D12PipelineState* CreateGlobalMergePSO();

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	FlexKit::MemoryPoolAllocator	gpuAllocator;
	FlexKit::CameraComponent		cameras;
	FlexKit::SceneNodeComponent		sceneNodes;

	bool									pause		= false;
	bool									debugVis	= false;
	bool									sampleTime	= true;
	FlexKit::CircularBuffer<double, 256>	samples;

	FlexKit::RootSignature			sortingRootSignature;
	FlexKit::Win32RenderWindow		renderWindow;

	FlexKit::ImGUIIntegrator		debugUI;

	FlexKit::ReadBackResourceHandle	readBackBuffer;
	FlexKit::QueryHandle			timingQueries;
	FlexKit::ConstantBufferHandle	constantBuffer;
	FlexKit::VertexBufferHandle		vertexBuffer;
	FlexKit::RunOnceQueue<void (FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;
};


/**********************************************************************

Copyright (c) 2022 Robert May

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

