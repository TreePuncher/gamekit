#include "SoundComponents.h"
#include "Components.h"

#ifdef ENABLESOUND
#include <fmod.hpp>
#endif

SoundSystem::SoundSystem(FlexKit::ThreadManager& IN_Threads, FlexKit::iAllocator* allocator) :
	threads{ IN_Threads }
{
#ifdef ENABLESOUND
	auto result = FMOD::System_Create(&system);
	auto initres = system->init(32, FMOD_INIT_NORMAL, nullptr);

	auto& Work = FlexKit::CreateWorkItem(
		[this](FlexKit::iAllocator& threadLocalAllocator)
		{
			FMOD::Sound* newSound = nullptr;
			auto res = system->createSound("test.flac", FMOD_DEFAULT, 0, &newSound);
			if (res == FMOD_RESULT::FMOD_OK)
				system->playSound(newSound, nullptr, false, &channel);

		}, allocator);

	Work._debugID = "FMOD: Play Sound";
	threads.AddWork(Work);
#endif
}

SoundSystem::~SoundSystem()
{
#ifdef ENABLESOUND
	for(auto s : sounds)
		s->release();

	sounds.clear();
	system->release();
	system = nullptr;
#endif
}

SoundHandle SoundSystem::LoadSoundFromDisk(const char* str)
{
	return FlexKit::InvalidHandle;
}

void SoundSystem::Update(FlexKit::iAllocator* memory)
{
#ifdef ENABLESOUND
	auto& Work = FlexKit::CreateWorkItem(
		[this](FlexKit::iAllocator&) {
			auto result = system->update();
		},
		memory);

	Work._debugID = "FMOD: Update Sound";
	threads.AddWork(Work);
#endif
}


FlexKit::UpdateTask* QueueSoundUpdate(FlexKit::UpdateDispatcher& Dispatcher, SoundSystem* Sounds, FlexKit::iAllocator* allocator)
{
#ifdef ENABLESOUND
	struct SoundUpdateData
	{
		SoundSystem* Sounds;
	};

	SoundSystem* Sounds_ptr = nullptr;
	auto& SoundUpdate = Dispatcher.Add<SoundUpdateData>(
		[&](auto& Builder, SoundUpdateData& Data)
		{
			Data.Sounds = Sounds;
			Builder.SetDebugString("UpdateSound");
		},
		[allocator](auto& Data, FlexKit::iAllocator& threadAllocator)
		{
			ProfileFunction();

			FK_LOG_9("Sound Update");
			Data.Sounds->Update(allocator);
		});

	return &SoundUpdate;
#else
	return nullptr;
#endif
}
