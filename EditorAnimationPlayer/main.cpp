#include "pch.h"
#include <Application.h>
#include <boost/interprocess/shared_memory_object.hpp>

using namespace boost::interprocess;

int main()
{
	try
	{
		shared_memory_object shm_obj
		(	open_or_create			//open or create
			, "shared_memory"		//name
			, read_only				//read-only mode
		);

		shm_obj.truncate(4096 * MEGABYTE);

		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		auto app = std::make_unique<FlexKit::FKApplication>(allocator);

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
