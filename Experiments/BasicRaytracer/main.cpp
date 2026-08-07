#include <Application.hpp>
#include <RTExperiment.hpp>
#include <dxBackend.hpp>

using namespace FlexKit;

int main()
{
	try
	{
		auto memoryPools = CreateEngineMemory();

		auto app = std::make_unique<FKApplication>(
			&memoryPools,
			CoreOptions{
				.GPUdebugMode		= false,
				.GPUValidation		= false,
				.GPUSyncQueues		= false,
				.CreateRenderSystem	= CreateDX,
			}, FrameworkOptions{
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
		auto msg = runtimeError.what();
		auto formatted = std::format("{}", msg);
		FK_LOG_ERROR("Exception Caught!\n%s", formatted.c_str());
	}
	catch (...)
	{
		FK_LOG_ERROR("Exception Caught!");
		return -1;
	}
	return 0;
}
