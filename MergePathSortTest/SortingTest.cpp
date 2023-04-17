#include "SortingTest.h"
#include <imgui.h>
#include <fmt\format.h>
#include <implot.h>

/************************************************************************************************/


constexpr FlexKit::PSOHandle InitiateBuffer		= FlexKit::PSOHandle{ GetCRCGUID(InitiateBuffer) };
constexpr FlexKit::PSOHandle LocalSort			= FlexKit::PSOHandle{ GetCRCGUID(LocalSort) };
constexpr FlexKit::PSOHandle CreateMergePath	= FlexKit::PSOHandle{ GetCRCGUID(CreateMergePath) };
constexpr FlexKit::PSOHandle GlobalMerge		= FlexKit::PSOHandle{ GetCRCGUID(GlobalMerge) };


/************************************************************************************************/


SortTest::SortTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState				{ IN_framework },
	constantBuffer				{ IN_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	vertexBuffer				{ IN_framework.GetRenderSystem().CreateVertexBuffer(16 * MEGABYTE, false) },
	cameras						{ IN_framework.core.GetBlockMemory() },
	runOnceQueue				{ IN_framework.core.GetBlockMemory() },
	sortingRootSignature		{ IN_framework.core.GetBlockMemory() },
	timingQueries				{ IN_framework.core.RenderSystem.CreateTimeStampQuery(512) },
	readBackBuffer				{ IN_framework.core.RenderSystem.CreateReadBackBuffer(512) },
	debugUI						{ IN_framework.core.RenderSystem, IN_framework.core.GetBlockMemory() },
	gpuAllocator				{ IN_framework.core.RenderSystem, 512 * MEGABYTE, 64 * KILOBYTE, FlexKit::DeviceHeapFlags::UAVBuffer, IN_framework.core.GetBlockMemory() }
{
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());

	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Sorting Test - WIP");

	sortingRootSignature.AllowIA = true;
	sortingRootSignature.SetParameterAsUINT(0, 16, 0, 0);
	sortingRootSignature.SetParameterAsSRV(1, 0);
	sortingRootSignature.SetParameterAsSRV(2, 1);
	sortingRootSignature.SetParameterAsUAV(3, 0, 0);

	auto res2 = sortingRootSignature.Build(framework.GetRenderSystem(), framework.core.GetTempMemory());
	FK_ASSERT(res2, "Failed to create root signature!");

	framework.GetRenderSystem().RegisterPSOLoader(InitiateBuffer, {
		.rootSignature = &sortingRootSignature,
		.loadState = [&](auto) { return CreateInitiateDataPSO(); } });

	framework.GetRenderSystem().RegisterPSOLoader(LocalSort, {
		.rootSignature = &sortingRootSignature,
		.loadState = [&](auto) { return CreateLocalSortPSO(); } });

	framework.GetRenderSystem().RegisterPSOLoader(CreateMergePath, {
		.rootSignature = &sortingRootSignature,
		.loadState = [&](auto) { return CreateMergePathPSO(); } });

	framework.GetRenderSystem().RegisterPSOLoader(GlobalMerge, {
		.rootSignature = &sortingRootSignature,
		.loadState = [&](auto) { return CreateGlobalMergePSO(); } });
}

/************************************************************************************************/


SortTest::~SortTest()
{
	sortingRootSignature.Release();
	framework.GetRenderSystem().ReleaseCB(constantBuffer);
}


/************************************************************************************************/


FlexKit::UpdateTask* SortTest::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	FlexKit::UpdateInput();
	debugUI.Update(renderWindow, core, dispatcher, dT);
	ImGui::NewFrame();
	ImGui::Begin("Hello");

	double meanTime = 0.0f;
	double graph_x[256];
	double graph_y[256];
	memset(graph_x, 0.0f, sizeof(graph_x));

	for (size_t I = 0; I < samples.size(); I++)
	{
		meanTime += samples[I];
		graph_y[I] = samples[I];
		graph_x[I] = I * 4.0 / 144.0;
	}

	meanTime /= samples.size();

	auto str = fmt::format("Mean Sort Time: {}ms\n", meanTime / 1000.0);
	ImGui::Text(str.c_str());

	if (ImPlot::BeginPlot("Timing History"))
	{
		ImPlot::PlotLine("", graph_x, graph_y, (int)samples.size());
		ImPlot::EndPlot();
	}

	if (ImPlot::BeginPlot("Timing Histogram"))
	{
		ImPlot::PlotHistogram("", graph_y, (int)samples.size());
		ImPlot::EndPlot();
	}

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();

	return nullptr;
}


/************************************************************************************************/



constexpr uint32_t p			= 8;
constexpr uint32_t blockCount	= 2048;
constexpr uint32_t bufferSize	= 1024 * blockCount;


FlexKit::UpdateTask* SortTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	FlexKit::ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 0, 0, 0, 0 });

	auto reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());
	auto reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());

	struct RenderStrands
	{
		FlexKit::FrameResourceHandle			sourceBuffer;
		FlexKit::FrameResourceHandle			destinationBuffer;
		FlexKit::FrameResourceHandle			mergePathBuffer;
		FlexKit::FrameResourceHandle			timingResults;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	frameGraph.AddMemoryPool(&gpuAllocator);
	frameGraph.AddOutput(renderWindow.GetBackBuffer());

	for(size_t J = 0; J < 1; J++)
	{
		frameGraph.AddNode(
			RenderStrands{
				.reserveCB = reserveCB
			},
			[&](FlexKit::FrameGraphNodeBuilder& builder, RenderStrands& data)
			{
				data.sourceBuffer		= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(bufferSize * 4), FlexKit::DeviceAccessState::DASUAV);
				data.destinationBuffer	= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(bufferSize * 4), FlexKit::DeviceAccessState::DASUAV);
				data.mergePathBuffer	= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(bufferSize / 32 * p), FlexKit::DeviceAccessState::DASUAV);
				data.timingResults		= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(512), FlexKit::DeviceAccessState::DASUAV);

				builder.SetDebugName(data.sourceBuffer, "sourceBuffer");
				builder.SetDebugName(data.destinationBuffer, "destinationBuffer");
				builder.SetDebugName(data.mergePathBuffer, "mergePathBuffer");
				builder.ReadBack(readBackBuffer);
			},
			[=, &sortingRootSignature = sortingRootSignature, backBuffer = renderWindow.GetBackBuffer(), this](RenderStrands& data, const FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, FlexKit::iAllocator& threadLocalAllocator)
			{
				ctx.SetComputeRootSignature(sortingRootSignature);
				ctx.SetComputeConstantValue(0, 1, &bufferSize, 0);
				ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.sourceBuffer, ctx));
				ctx.Dispatch(resources.GetPipelineState(InitiateBuffer), { bufferSize / 1024, 1, 1 });
				ctx.AddUAVBarrier(resources.GetResource(data.sourceBuffer));

				ctx.BeginEvent_DEBUG("Merge Sort");
				ctx.TimeStamp(timingQueries, 2 * J + 0);

				ctx.Dispatch(resources.GetPipelineState(LocalSort), { blockCount, 1, 1 });

				struct
				{
					uint32_t p;
					uint32_t blockSize;
					uint32_t blockCount;
				} constants = {
					.p			= p,
					.blockSize	= bufferSize / blockCount,
					.blockCount = blockCount
				};

				auto input				= data.sourceBuffer;
				auto output				= data.destinationBuffer;
				const uint32_t mergeX	= blockCount;
				uint32_t passX			= blockCount;

				const auto passes = (uint32_t)std::ceilf(std::log2(blockCount));

				ctx.BeginEvent_DEBUG("Merges");
				for(size_t i = 0; i < passes; i++)
				{
					ctx.BeginEvent_DEBUG("Merge Pass");

					ctx.SetComputeConstantValue(0, 3, &constants, 0);
					ctx.SetComputeShaderResourceView(1, resources.NonPixelShaderResource(input, ctx), 0  * 4);
					ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.mergePathBuffer, ctx));
					ctx.Dispatch(resources.GetPipelineState(CreateMergePath), { blockCount * p / 16, 1, 1 });

					ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.mergePathBuffer, ctx));
					ctx.SetComputeUnorderedAccessView(3, resources.UAV(output, ctx));

					ctx.Dispatch(resources.GetPipelineState(GlobalMerge), { mergeX, 1, 1 });

					constants.p				*= 2;
					constants.blockSize		*= 2;
					constants.blockCount	/= 2;
					passX *= 2;

					std::swap(input, output);
					ctx.EndEvent_DEBUG();
				}

				ctx.TimeStamp(timingQueries, 2 * J + 1);

				if (sampleTime)
				{
					sampleTime = false;

					ctx.ResolveQuery(timingQueries, (2 * J), (2 + 2 * J), resources.ResolveDst(readBackBuffer, ctx), 2 * J * 8);

					ctx.QueueReadBack(readBackBuffer,
						[&renderSystem = resources.renderSystem(), this](FlexKit::ReadBackResourceHandle readBack)
						{
							auto [buffer, size] = renderSystem.OpenReadBackBuffer(readBack);

							size_t timing[2];
							UINT64 freq;
							memcpy(timing, buffer, sizeof(timing));
							auto HR = renderSystem.GraphicsQueue->GetTimestampFrequency(&freq);
							renderSystem.CloseReadBackBuffer(readBack);

							samples.push_back((timing[1] - timing[0]) / double(freq));

							sampleTime = true;
						});

				}
				ctx.EndEvent_DEBUG();
				ctx.EndEvent_DEBUG();
			});
	}

	debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

	PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void SortTest::PostDrawUpdate(FlexKit::EngineCore& core, double dT)
{
	renderWindow.Present(0, 0);

	core.RenderSystem.ResetConstantBuffer(constantBuffer);
}

 
/************************************************************************************************/


bool SortTest::EventHandler(FlexKit::Event evt)
{
	if ((evt.InputSource == FlexKit::Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC) ||
		(evt.InputSource == FlexKit::Event::E_SystemEvent && evt.Action == FlexKit::Event::Exit))
	{
		framework.quit = true;
		return true;
	}
	else
		return debugUI.HandleInput(evt);
}


/************************************************************************************************/


ID3D12PipelineState* SortTest::CreateInitiateDataPSO()
{
	auto& renderSystem = framework.GetRenderSystem();
	FlexKit::Shader CShader = renderSystem.LoadShader("Initiate", "cs_6_2", R"(assets\shaders\Sorting\InitiateBuffer.hlsl)");

	struct
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature*					rootSig;
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
		D3D12_SHADER_BYTECODE					byteCode;
	} stream = {
		.rootSig = sortingRootSignature,
		.byteCode = CShader,
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
		.SizeInBytes = sizeof(stream),
		.pPipelineStateSubobjectStream = &stream
	};

	ID3D12PipelineState* PSO = nullptr;
	auto HR = renderSystem.pDevice10->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

	return PSO;
}


/************************************************************************************************/


ID3D12PipelineState* SortTest::CreateLocalSortPSO()
{
	auto& renderSystem = framework.GetRenderSystem();
	FlexKit::Shader CShader = renderSystem.LoadShader("LocalBitonicSort", "cs_6_2", R"(assets\shaders\Sorting\BitonicSort.hlsl)");

	struct
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature* rootSig;
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
		D3D12_SHADER_BYTECODE					byteCode;
	} stream = {
		.rootSig = sortingRootSignature,
		.byteCode = CShader,
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
		.SizeInBytes = sizeof(stream),
		.pPipelineStateSubobjectStream = &stream
	};

	ID3D12PipelineState* PSO = nullptr;
	auto HR = renderSystem.pDevice10->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

	return PSO;
}


/************************************************************************************************/


ID3D12PipelineState* SortTest::CreateMergePathPSO()
{
	auto& renderSystem = framework.GetRenderSystem();
	FlexKit::Shader CShader = renderSystem.LoadShader("CreateMergePath", "cs_6_6", R"(assets\shaders\Sorting\MergePath.hlsl)");

	struct
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature* rootSig;
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
		D3D12_SHADER_BYTECODE					byteCode;
	} stream = {
		.rootSig = sortingRootSignature,
		.byteCode = CShader,
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
		.SizeInBytes = sizeof(stream),
		.pPipelineStateSubobjectStream = &stream
	};

	ID3D12PipelineState* PSO = nullptr;
	auto HR = renderSystem.pDevice10->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

	return PSO;
}


/************************************************************************************************/


ID3D12PipelineState* SortTest::CreateGlobalMergePSO()
{
	auto& renderSystem = framework.GetRenderSystem();
	FlexKit::Shader CShader = renderSystem.LoadShader("GlobalMerge", "cs_6_6", R"(assets\shaders\Sorting\Merge.hlsl)", { .hlsl2021 = true });

	struct
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		ID3D12RootSignature*					rootSig;
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE		type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
		D3D12_SHADER_BYTECODE					byteCode;
	} stream = {
		.rootSig = sortingRootSignature,
		.byteCode = CShader,
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
		.SizeInBytes = sizeof(stream),
		.pPipelineStateSubobjectStream = &stream
	};

	ID3D12PipelineState* PSO = nullptr;
	auto HR = renderSystem.pDevice10->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

	return PSO;
}


/************************************************************************************************/


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

