#include "pch.h"
#include <Application.hpp>
#include "AnimationTest.hpp"
#include <vkBackend.hpp>
#include <vkWin32Surface.hpp>


int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, FlexKit::CoreOptions{ .CreateRenderSystem = FlexKit::CreateVK });

		auto window = CreateWin32VKSurface(app.get()->GetRenderSystem(), { 800, 600 }, FlexKit::DeviceFormat::R8G8B8A8_UNORM);

		//app->PushState<AnimationTest>();
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
