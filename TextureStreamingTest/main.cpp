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
		constexpr bool enableDebug = true;
#else
		constexpr bool enableDebug = false;
#endif

		FlexKit::CoreOptions options{
			.threadCount	= FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1,
			.GPUdebugMode	= enableDebug,
			.GPUValidation	= enableDebug,
			.GPUSyncQueues	= enableDebug,
		};

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, options);
		
		app->PushState<TextureStreamingTest>();
		app->GetCore().FPSLimit		= 90;
		app->GetCore().FrameLock	= false;
		app->GetCore().vSync		= false;
		app->Run();
	}
	catch (...)
	{
		return -1;
	}
	return 0;
}
