#include <print>
#include <Application.hpp>
#include <RTExperiment.hpp>

int main()
{
	using namespace FlexKit;

	auto memory = CreateEngineMemory();
	FKApplication app{ memory, FlexKit::CoreOptions{
		.threadCount	= 3,
		.GPUdebugMode	= true,
		.GPUValidation	= true,
		.GPUSyncQueues	= true,
	} };

	app.PushState<Experiments::RTExperiment>();
	app.Run();
	app.Release();

	ReleaseEngineMemory(memory);

	return 0;
}
