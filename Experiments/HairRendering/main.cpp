#include <Application.hpp>
#include <BuildSettings.hpp>
#include "HairRendering.hpp"
#include <filesystem>

int main(const int args, const char* args_v[])
{
	try
	{
		bool enableWorkGroups = true;

		for (int i = 0; i < args; i++)
		{
			const std::string_view arg{ args_v[i] };
			if (arg == "--enable-workgroups")
				enableWorkGroups = true;
		}

		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));


#ifdef _DEBUG
		const bool debugGPU = true;
#else
		const bool debugGPU = false;
#endif
		FlexKit::CoreOptions options{
			.GPUdebugMode	= debugGPU,
			.GPUValidation	= debugGPU,
			.GPUSyncQueues	= debugGPU,
		};

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, options);
		
		auto& state = app->PushState<HairRenderingTest>(enableWorkGroups);

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
