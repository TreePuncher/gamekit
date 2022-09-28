#pragma once
#pragma comment(lib, "fmod_vc.lib")

#include "containers.h"
#include "handle.h"


/************************************************************************************************/
// Predeclarations

namespace FlexKit
{
	class ThreadManager;
	class iAllocator;
	class UpdateTask;
	class UpdateDispatcher;
}

namespace FMOD
{
	class Channel;
	class Sound;
	class System;
}


using SoundHandle = FlexKit::Handle_t<16, GetTypeGUID(SoundHandle)>;


/************************************************************************************************/


class SoundSystem
{
public:
	SoundSystem(FlexKit::ThreadManager& IN_Threads, FlexKit::iAllocator* memory);

	~SoundSystem();

	SoundHandle LoadSoundFromDisk(const char* str);

	void Update(FlexKit::iAllocator* memory);

	FlexKit::Vector<FMOD::Sound*>	sounds;
	FlexKit::ThreadManager&			threads;

	FMOD::System*	system	= nullptr;
	FMOD::Channel*	channel = nullptr;
	unsigned int	version;
};


/************************************************************************************************/


FlexKit::UpdateTask* QueueSoundUpdate(FlexKit::UpdateDispatcher& Dispatcher, SoundSystem* Sounds, FlexKit::iAllocator* allocator);
