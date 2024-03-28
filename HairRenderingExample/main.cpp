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


#ifdef _DEBUG
		const bool debugGPU = true;
#else
		const bool debugGPU = false;
#endif
		FlexKit::CoreOptions options{
			.GPUdebugMode = true,
			.GPUValidation = true,
			.GPUSyncQueues = true,
		};

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, options);
		
		auto& state = app->PushState<HairRenderingTest>();

		app->GetCore().FPSLimit		= 144;
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
