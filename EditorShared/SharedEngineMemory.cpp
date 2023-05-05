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

	auto sharedMemory = new(buffer) SharedEngineMemory();
	sharedMemory->mapped = std::move(region);
	strncpy(sharedMemory->blockTag, "Hello I am shared memory!\n", 32);

	FlexKit::BlockAllocator_desc BAdesc;
	BAdesc.SmallBlock	= BLOCKALLOCSIZE / 4;
	BAdesc.MediumBlock	= BLOCKALLOCSIZE / 4;
	BAdesc.LargeBlock	= BLOCKALLOCSIZE / 2;
	BAdesc._ptr			= buffer + (64 - ((size_t)buffer % 64));

	sharedMemory->blockAllocator.Init(BAdesc);
	sharedMemory->components	= &sharedMemory->blockAllocator.allocate<SharedComponents>(sharedMemory->blockAllocator);
	sharedMemory->inputQueue	= Queue<MessageBlob>{ sharedMemory->blockAllocator };
	sharedMemory->outputQueue	= Queue<MessageBlob>{ sharedMemory->blockAllocator };

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

	lightComponent			{ allocator },
	shadowMaps				{ allocator },

	triggers				{ allocator, allocator },

	ikTargetComponent		{ allocator },
	ikComponent				{ allocator }
{

}
