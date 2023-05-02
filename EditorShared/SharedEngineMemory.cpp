#include "SharedEngineMemory.hpp"
#include <memoryUtilities.h>
#include <EngineCore.h>

using namespace FlexKit;

SharedEngineMemory AllocateSharedEngineMemory(shared_memory_object& obj)
{
	obj.truncate(FlexKit::BLOCKALLOCSIZE);

	SharedEngineMemory shared
	{
		.allocation { (FlexKit::EngineMemory*)_aligned_malloc(sizeof(FlexKit::EngineMemory*), 0x40)},
		.mapped		{obj, read_write }
	};

	auto temp = obj.get_mapping_handle();

	FlexKit::BlockAllocator_desc BAdesc;
	BAdesc.SmallBlock	= BLOCKALLOCSIZE / 4;
	BAdesc.MediumBlock	= BLOCKALLOCSIZE / 4;
	BAdesc.LargeBlock	= BLOCKALLOCSIZE / 2;
	BAdesc._ptr			= (FlexKit::byte*)shared.mapped.get_address();

	shared.allocation = new(shared.allocation) EngineMemory{ BAdesc };

	return shared;
}


void ReleaseSharedEngineMemory(SharedEngineMemory&)
{

}
