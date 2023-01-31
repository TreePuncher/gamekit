#include "pch.h"
#include <Application.h>
#include "PhysicsTest.h"

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1);

		app->PushState<PhysicsTest>();
		app->GetCore().FPSLimit		= 90;
		app->GetCore().FrameLock	= false;
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
