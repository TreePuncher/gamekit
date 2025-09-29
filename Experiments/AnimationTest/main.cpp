#include "pch.h"
#include <Application.hpp>
#include "AnimationTest.hpp"
#include <vkBackend.hpp>
#include <vkWin32Surface.hpp>


using namespace FlexKit;

struct TestState : FrameworkState
{
	TestState(GameFramework& IN_framework) : FrameworkState(IN_framework)
	{
		renderWindow = CreateWin32VKSurface(GetRenderSystem(), { 800, 600 }, DeviceFormat::R8G8B8A8_UNORM);
	}


	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT)
	{
		vkWin32UpdateInput();
	    return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
	{
		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);
		ClearBackBuffer(frameGraph, renderTarget, { 0, 1, 0, 1 });
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
