#include <Application.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <SharedEngineMemory.hpp>

using namespace boost::interprocess;

int main()
{
	try
	{
		shared_memory_object shm_obj(
			open_or_create,
			"shared_memory",
			read_write);

		auto sharedMemory = AllocateSharedEngineMemory(shm_obj);
		EXITSCOPE(ReleaseSharedEngineMemory(sharedMemory));

		auto app = std::make_unique<FlexKit::FKApplication>(sharedMemory.allocation);
		
		app->GetCore().FPSLimit		= 90;
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
