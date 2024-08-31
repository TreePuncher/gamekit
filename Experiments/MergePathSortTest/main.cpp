#define  _SILENCE_CXX20_CISO646_REMOVED_WARNING

#include <BuildSettings.hpp>
#include <Application.hpp>
#include <ModifiableShape.hpp>
#include <CameraUtilities.hpp>
#include <MathUtilities.hpp>
#include <Win32Graphics.hpp>
#include <fmt\printf.h>

#include <memory>
#include "SortingTest.h"

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator);

		app->PushState<SortTest>();

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
