#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace boost::interprocess;

namespace FlexKit
{
	struct EngineMemory;
}

struct SharedEngineMemory
{
	FlexKit::EngineMemory*	allocation;
	mapped_region			mapped;
};

SharedEngineMemory	AllocateSharedEngineMemory(shared_memory_object& obj);
void				ReleaseSharedEngineMemory(SharedEngineMemory&);
