#include <print>
#include <Application.hpp>
#include <RTExperiment.hpp>
#include <dxBackend.hpp>

int main()
{
	try
	{
		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator,
			FlexKit::CoreOptions{
				.GPUdebugMode		= true,
				.GPUValidation		= true,
				.GPUSyncQueues		= true,
				.CreateRenderSystem	= CreateDX,
			}, FlexKit::FrameworkOptions{
				.integrateIMGUI		= false
			});

		app->PushState<RTExperimentState>();
		app->GetCore().FPSLimit = 144;
		app->GetCore().FrameLock = true;
		app->GetCore().vSync = true;
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
