#include "pch.h"
#include <Application.h>
#include "TextureStreamingTest.h"

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

#ifdef _DEBUG
		constexpr bool enableDebug = false;
#else
		constexpr bool enableDebug = true;
#endif

		FlexKit::CoreOptions options{
			.threadCount	= FlexKit::Max(std::thread::hardware_concurrency(), 1u) - 1,
			//.threadCount	= 0,
			.GPUdebugMode	= enableDebug,
			.GPUValidation	= false,
			.GPUSyncQueues	= false,
		};

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, options);
		
		app->PushState<TextureStreamingTest>();
		app->GetCore().FPSLimit		= 90;
		app->GetCore().FrameLock	= false;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (...)
	{
		return -1;
	}
	return 0;
}
