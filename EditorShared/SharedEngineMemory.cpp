#include "SharedEngineMemory.hpp"
#include <memoryUtilities.h>
#include <EngineCore.h>

using namespace FlexKit;


SharedEngineMemory* InitiateSharedMemory(shared_memory_object& obj)
{
	try
	{
		obj.truncate(FlexKit::BLOCKALLOCSIZE + 64 + FlexKit::AlignedSize(sizeof(SharedEngineMemory), 64));
	}
	catch (...)
	{
		FK_LOG_ERROR("Failed to create shared memory allocator!");
		return nullptr;
	}

	mapped_region region{ obj, read_write };

	auto buffer = (FlexKit::byte*)region.get_address();

	auto sharedMemory = new(buffer) SharedEngineMemory{};
	sharedMemory->mapped = std::move(region);
	strncpy(sharedMemory->blockTag, "Hello I am shared memory!\n", 32);

	FlexKit::BlockAllocator_desc BAdesc;
	BAdesc.SmallBlock	= BLOCKALLOCSIZE / 4;
	BAdesc.MediumBlock	= BLOCKALLOCSIZE / 4;
	BAdesc.LargeBlock	= BLOCKALLOCSIZE / 2;
	BAdesc._ptr			= buffer + (64 - ((size_t)buffer % 64)) + sizeof(SharedEngineMemory);

	sharedMemory->sharedAllocator.Init(BAdesc);
	sharedMemory->components	= &sharedMemory->blockAllocator.allocate<SharedComponents>(sharedMemory->blockAllocator);
	sharedMemory->playerQueue	= InterProcessQueue<InterProcessMessage>{ sharedMemory->blockAllocator };
	sharedMemory->editorQueue	= InterProcessQueue<InterProcessMessage>{ sharedMemory->blockAllocator };
	sharedMemory->responders	= FlexKit::Vector<std::unique_ptr<ResponseInterface>>{ sharedMemory->blockAllocator };

	return sharedMemory;
}


SharedEngineMemory* GetSharedMemory(shared_memory_object& obj, size_t offset)
{
	static mapped_region region{ obj, read_write, 0, 0, (void*)offset };
	auto shared = reinterpret_cast<SharedEngineMemory*>(region.get_address());
	shared->components->Register();

	return shared;
}


void SharedComponents::Register()
{
	FlexKit::BrushComponent::ManualRegistration(&brushComponent);
	FlexKit::SceneNodeComponent::ManualRegistration(&sceneNodes);
	FlexKit::StringIDComponent::ManualRegistration(&stringIDComponent);
	FlexKit::CameraComponent::ManualRegistration(&cameraComponent);
	FlexKit::SceneVisibilityComponent::ManualRegistration(&visibilityComponent);
	FlexKit::SkeletonComponent::ManualRegistration(&skeletonComponent);
	FlexKit::AnimatorComponent::ManualRegistration(&animatorComponent);
	FlexKit::LightComponent::ManualRegistration(&lightComponent);
	FlexKit::ShadowMapComponent::ManualRegistration(&shadowMaps);
	FlexKit::FABRIKTargetComponent::ManualRegistration(&ikTargetComponent);
	FlexKit::FABRIKComponent::ManualRegistration(&ikComponent);
	FlexKit::TriggerComponent::ManualRegistration(&triggers);
}


void ReleaseSharedEngineMemory(SharedEngineMemory& memory)
{
}


SharedComponents::SharedComponents(
	FlexKit::iAllocator& allocator) :

	stringIDComponent		{ allocator },
	cameraComponent			{ allocator },
	visibilityComponent		{ allocator },
	skeletonComponent		{ allocator },
	animatorComponent		{ allocator },
	brushComponent			{ allocator },

	lightComponent			{ allocator },
	shadowMaps				{ allocator },

	triggers				{ allocator, allocator },

	ikTargetComponent		{ allocator },
	ikComponent				{ allocator }
{

}


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
