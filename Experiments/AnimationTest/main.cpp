#include "pch.h"
#include <Application.hpp>
#include "AnimationTest.hpp"
#include <vkBackend.hpp>
#include <vkSurface.hpp>


#define USEVK 1


#if !USEVK
#include <Win32Graphics.hpp>
#include <dxBackend.hpp>
#endif

using namespace FlexKit;

struct TestState : FrameworkState
{
	TestState(GameFramework& IN_framework) : FrameworkState(IN_framework)
	{
#if WIN32
#if USEVK
		renderWindow = CreateWin32VKSurface(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
#else
		renderWindow = CreateWin32RenderWindow(GetRenderSystem(), DefaultWindowDesc({ 800, 600 }));
#endif
#else
		renderWindow = CreateWaylandSurface(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
#endif


		GetRenderSystem().RegisterPSOLoader(GetTypeGUID(Trangle),
			[](IRenderSystem& renderSystem, iAllocator& allocator)
			{
				PipelineBuilder builder(renderSystem, allocator);
				builder.AddInputLayout({
					.inputs = {
						{
							.name = "POSITION",
							.index = 0,
							.format = DeviceFormat::R32G32B32_FLOAT,
							.inputSlotClass = EInputClassification::PerVertex,
						}},
					.count = 1
					});
				builder.AddVertexShader("VMain", "assets/shaders/TestShader.hlsl");
				builder.AddPixelShader("PMain", "assets/shaders/TestShader.hlsl");
				builder.AddRasterizerState();
				builder.AddRenderTargetState({
						.targetCount = 1,
						.targetFormats = { DeviceFormat::R8G8B8A8_UNORM },
					});

				return builder.Build(renderSystem, allocator);
			});

		GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

		vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
		cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);
	}

	~TestState()
	{
		GetRenderSystem().ReleaseVB(vBuffer);
	}

	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT)
	{
		t += dT;

#ifdef WIN32
#if USEVK
	    vkWin32UpdateInput();
#else
		Win32UpdateInput();
#endif
#else
		ProcessEvents(*renderWindow);
#endif
	    return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
	{
		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);

		struct
		{
			float xyz[3];
		} triangle[3] = {
			{.xyz = { -1.0f, -1.0f, 0.0f }},
			{.xyz = { 0.0f, 1.0f, 0.0f }},
			{.xyz = { 1.0f, -1.0f, 0.0f }},
		};

		GetRenderSystem().VertexBufferPush(vBuffer, triangle, sizeof(triangle));


#if USEVK
		ClearBackBuffer(frameGraph, renderTarget, { 0, 0, 0, 1 });
#else
		ClearBackBuffer(frameGraph, renderTarget, { 0, 0, 0, 1 });
#endif
		struct DrawTrangle
		{
			FrameResourceHandle renderTarget;
		};

		frameGraph.AddConstantBuffer(cBuffer);

		frameGraph.AddNode<>(
			DrawTrangle{},
			[&](FrameGraphNodeBuilder& builder, auto& data)
			{
				data.renderTarget = builder.RenderTarget(renderTarget);
			},
			[=](const auto& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
			{
				float fTime = (float)t;

				
				auto cb = resources.ReserveCB(512);
				
				struct
				{
					float time;
				} constants0{
					.time = fTime,
				};

			    struct
				{
					float4 xyz;
					float4 uvw;
				} constants1{
					.xyz = float4{ 0.0f, 1.0f, 0.0f, 0.0f },
					.uvw = float4{ 1.0f, 0.0f, 0.0f, 0.0f },
				};

				const auto cb0Set = ConstantBufferDataSet{ constants0, cb };
				//const auto cb1Set = ConstantBufferDataSet{ constants1, cb };

				const IPipelineInterface* pipelineInterface = resources.GetPipelineState(GetTypeGUID(Trangle), threadLocalAllocator)->GetInterface();
				DescriptorSet descriptorSet{ ctx, pipelineInterface->GetDescHeap(0), threadLocalAllocator};
				descriptorSet.SetCBV(ctx, 0, cb0Set);

				ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), threadLocalAllocator);
				ctx.SetVertexBuffers(static_vector<VertexBufferEntry, 1>{ VertexBufferEntry
					                    {
						                    .VertexBuffer	= vBuffer,
		                                    .Stride			= 36,
		                                    .Offset			= 0, 
				                        } });

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				//ctx.SetGraphicsConstantValue(0, 8, &constants0);
				//ctx.SetGraphicsConstantBufferView(0, cb1Set);
				ctx.SetGraphicsDescriptorTable(0, descriptorSet);

				//ctx.SetGraphicsConstantBufferView(0, cBuffer, 0);

				ctx.Draw(3);
			});

		PresentBackBuffer(frameGraph, *renderWindow);
	    return nullptr;
	}


	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
	{
		bool res = renderWindow->Present();
		core.RenderSystem->ResetVertexBuffer(vBuffer);
	}

	double					t				= 0.0;
	IRenderWindow*			renderWindow	= nullptr;
	VertexBufferHandle		vBuffer			= InvalidHandle;
	ConstantBufferHandle	cBuffer			= InvalidHandle;
};

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, FlexKit::CoreOptions{
			.GPUdebugMode		= true,
			.GPUValidation		= true,
			.GPUSyncQueues		= true,
#if USEVK
			.CreateRenderSystem = CreateVK,
#else
			.CreateRenderSystem = CreateDX,
#endif
		});

		app->PushState<TestState>();
		app->GetCore().FPSLimit		= 144;
		app->GetCore().FrameLock	= true;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (std::runtime_error runtimeError)
	{
		FK_LOG_ERROR("Exception Caught!\n%s", runtimeError.what());
	}
	catch (...)
	{
		FK_LOG_ERROR("Exception Caught!");
		return -1;
	}
	return 0;
}


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
