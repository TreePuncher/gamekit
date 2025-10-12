#include "pch.h"
#include <Application.hpp>
#include "AnimationTest.hpp"
#include <vkBackend.hpp>
#include <vkSurface.hpp>


using namespace FlexKit;

struct TestState : FrameworkState
{
	TestState(GameFramework& IN_framework) : FrameworkState(IN_framework)
	{
#if WIN32
		renderWindow = CreateWin32VKSurface(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
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
	}


	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT)
	{
#ifdef WIN32
		vkWin32UpdateInput();
#else
		ProcessEvents(*renderWindow);
#endif
	    return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
	{
		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);
		ClearBackBuffer(frameGraph, renderTarget, { 0, 1, 0, 1 });

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
			[](const auto& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& allocator)
			{
				ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), allocator);
				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				ctx.Draw(3);
			});

		PresentBackBuffer(frameGraph, *renderWindow);
	    return nullptr;
	}


	void PostDrawUpdate(FlexKit::EngineCore&, double dT) override
	{
		bool res = renderWindow->Present();
	}

	FlexKit::IRenderWindow* renderWindow = nullptr;
};

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, FlexKit::CoreOptions{ .CreateRenderSystem = FlexKit::CreateVK });

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
