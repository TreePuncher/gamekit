#define  _SILENCE_CXX20_CISO646_REMOVED_WARNING

#include "buildsettings.h"
#include <Application.h>
#include <ModifiableShape.h>
#include <CameraUtilities.h>
#include <MathUtils.h>
#include <Win32Graphics.h>
#include <fmt\printf.h>

#include <memory>
#include "SortingTest.h"

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, FlexKit::Max(std::thread::hardware_concurrency() / 2, 1u) - 1);

		app->PushState<SortTest>();

		app->GetCore().FPSLimit		= 90;
		app->GetCore().FrameLock	= true;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}
