#include "buildsettings.h"
#include <Application.h>
#include <ModifiableShape.h>
#include <CameraUtilities.h>
#include <MathUtils.h>
#include <Win32Graphics.h>
#include <fmt\printf.h>

#include "HairRenderingExample.hpp"

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator);
		
		auto& state = app->PushState<HairRenderingTest>();

		app->GetCore().FPSLimit		= 144;
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
