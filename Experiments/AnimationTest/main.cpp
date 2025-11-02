#include "pch.h"
#include <Application.hpp>
#include "AnimationTest.hpp"
#include <vkBackend.hpp>
#include <vkSurface.hpp>
#include <dxBackend.hpp>
#include <Win32Graphics.hpp>


#define USEVK 1

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
				builder.AddPixelShader("PMain",		"assets/shaders/TestShader.hlsl");
				builder.AddVertexShader("VMain",	"assets/shaders/TestShader.hlsl");
				builder.AddRasterizerState();
				builder.AddRenderTargetState({	
					    .targetCount	= 1,
					    .targetFormats	= { DeviceFormat::R8G8B8A8_UNORM },
					});

				return builder.Build(renderSystem, allocator);
			});

		GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

		pushBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
	}

	~TestState()
	{
		GetRenderSystem().ReleaseVB(pushBuffer);
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

		GetRenderSystem().VertexBufferPush(pushBuffer, triangle, sizeof(triangle));


#if USEVK
		ClearBackBuffer(frameGraph, renderTarget, { 0, 0, 0, 1 });
#else
		ClearBackBuffer(frameGraph, renderTarget, { 0, 0, 0, 1 });
#endif
		struct DrawTrangle
		{
			FrameResourceHandle renderTarget;
		};

		frameGraph.AddNode<>(
			DrawTrangle{},
			[&](FrameGraphNodeBuilder& builder, auto& data)
			{
				data.renderTarget = builder.RenderTarget(renderTarget);
			},
			[=](const auto& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& allocator)
			{
				float fTime = (float)t;

				ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), allocator);
				ctx.SetVertexBuffers(static_vector<VertexBufferEntry, 1>{ VertexBufferEntry
					                    {
						                    .VertexBuffer	= pushBuffer,
		                                    .Stride			= 36,
		                                    .Offset			= 0, 
				                        } });

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				ctx.SetGraphicsConstantValue(0, 1, &fTime);
				ctx.Draw(3);
			});

		PresentBackBuffer(frameGraph, *renderWindow);
	    return nullptr;
	}


	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
	{
		bool res = renderWindow->Present();
		core.RenderSystem->ResetVertexBuffer(pushBuffer);
	}

	double				t				= 0.0;
	IRenderWindow*		renderWindow	= nullptr;
	VertexBufferHandle	pushBuffer;
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
