#include "SortingTest.h"


/************************************************************************************************/


constexpr FlexKit::PSOHandle InitiateBuffer = FlexKit::PSOHandle{ GetCRCGUID(InitiateBuffer) };
constexpr FlexKit::PSOHandle LocalSort		= FlexKit::PSOHandle{ GetCRCGUID(LocalSort) };
constexpr FlexKit::PSOHandle GlobalSort		= FlexKit::PSOHandle{ GetCRCGUID(GlobalSort) };


/************************************************************************************************/


SortTest::SortTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState				{ IN_framework },
	constantBuffer				{ IN_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	cameras						{ IN_framework.core.GetBlockMemory() },
	runOnceQueue				{ IN_framework.core.GetBlockMemory() },
	sortingRootSignature		{ IN_framework.core.GetBlockMemory() },
	gpuAllocator				{ IN_framework.core.RenderSystem, 64 * MEGABYTE, 64 * KILOBYTE, FlexKit::DeviceHeapFlags::UAVBuffer, IN_framework.core.GetBlockMemory() }
{
	auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 });

	if (res.second)
		renderWindow = res.first;

	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Hair Rendering - WIP");

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

	framework.GetRenderSystem().RegisterPSOLoader(GlobalSort, {
		.rootSignature = &sortingRootSignature,
		.loadState = [&](auto) { return CreateGlobalSortPSO(); } });
}

/************************************************************************************************/


SortTest::~SortTest()
{
	sortingRootSignature.Release();
	framework.GetRenderSystem().ReleaseCB(constantBuffer);
}


/************************************************************************************************/


FlexKit::UpdateTask* SortTest::Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT)
{
	FlexKit::UpdateInput();

	return nullptr;
}


/************************************************************************************************/


FlexKit::UpdateTask* SortTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph)
{
	FlexKit::ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 1, 0, 1, 0 });

	auto reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());

	struct RenderStrands
	{
		FlexKit::FrameResourceHandle			sourceBuffer;
		FlexKit::FrameResourceHandle			destinationBuffer;
		FlexKit::FrameResourceHandle			mergePathBuffer;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	frameGraph.AddMemoryPool(&gpuAllocator);

	frameGraph.AddNode(
		RenderStrands{
			.reserveCB = reserveCB
		},
		[&](FlexKit::FrameGraphNodeBuilder& builder, RenderStrands& data)
		{
			data.sourceBuffer		= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(4 * MEGABYTE), FlexKit::DeviceResourceState::DRS_UAV);
			data.destinationBuffer	= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(4 * MEGABYTE), FlexKit::DeviceResourceState::DRS_UAV);
			data.mergePathBuffer	= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(MEGABYTE), FlexKit::DeviceResourceState::DRS_UAV);

			builder.SetDebugName(data.sourceBuffer, "sourceBuffer");
			builder.SetDebugName(data.destinationBuffer, "destinationBuffer");
			builder.SetDebugName(data.mergePathBuffer, "mergePathBuffer");
		},
		[=, &sortingRootSignature = sortingRootSignature, backBuffer = renderWindow.GetBackBuffer(), this](RenderStrands& data, const FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, FlexKit::iAllocator& threadLocalAllocator)
		{
			const uint32_t bufferSize = 1024 * 8;

			ctx.SetComputeRootSignature(sortingRootSignature);
			ctx.SetComputeConstantValue(0, 1, &bufferSize, 0);
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.sourceBuffer, ctx));
			ctx.Dispatch(resources.GetPipelineState(InitiateBuffer), { bufferSize / 1024, 1, 1 });
			ctx.AddUAVBarrier(resources.GetResource(data.sourceBuffer));

			ctx.Dispatch(resources.GetPipelineState(LocalSort), { bufferSize / 1024, 1, 1 });
			ctx.AddUAVBarrier(resources.GetResource(data.sourceBuffer));

			const uint32_t p = 8;
			struct
			{
				uint32_t p;
				uint32_t blockSize;
				uint32_t blockCount;
			} constants = {
				.p = p,
				.blockSize = bufferSize / 2,
			};

			ctx.DiscardResource(resources.GetResource(data.mergePathBuffer));

			ctx.SetComputeConstantValue(0, 3, &constants, 0);
			ctx.SetComputeShaderResourceView(1, resources.NonPixelShaderResource(data.sourceBuffer, ctx), 0  * 4);
			ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.sourceBuffer, ctx), (bufferSize / 2) * 4);
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.mergePathBuffer, ctx));
			ctx.Dispatch(resources.GetPipelineState(GlobalSort), { 1, 1, 1 });
			ctx.AddUAVBarrier(resources.GetResource(data.mergePathBuffer));
		});


	PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void SortTest::PostDrawUpdate(FlexKit::EngineCore&, double dT)
{
	renderWindow.Present(1, 0);
}


/************************************************************************************************/


bool SortTest::EventHandler(FlexKit::Event evt)
{
	return false;
}


/************************************************************************************************/


ID3D12PipelineState* SortTest::CreateInitiateDataPSO()
{
	auto& renderSystem = framework.GetRenderSystem();
	FlexKit::Shader CShader = renderSystem.LoadShader("Initiate", "cs_6_2", R"(assets\shaders\Sorting\InitiateBuffer.hlsl)");

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
	auto HR = renderSystem.pDevice9->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

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
	auto HR = renderSystem.pDevice9->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

	return PSO;
}


/************************************************************************************************/


ID3D12PipelineState* SortTest::CreateGlobalSortPSO()
{
	auto& renderSystem = framework.GetRenderSystem();
	FlexKit::Shader CShader = renderSystem.LoadShader("GlobalMergePathSort", "cs_6_2", R"(assets\shaders\Sorting\MergePath.hlsl)");

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
	auto HR = renderSystem.pDevice9->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&PSO));

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

