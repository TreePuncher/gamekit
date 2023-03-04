#include "level.hpp"
#include "containers.h"
#include "GameFramework.h"
#include "Scene.h"
#include "SceneLoadingContext.h"
#include <algorithm>

namespace FlexKit
{	/************************************************************************************************/


	struct LoadedLevel
	{
		uint64_t		ID;
		Level			level;
		iAllocator*		allocator;
	};

	Vector<LoadedLevel*>	loadedLevels;
	LoadedLevel*			activeLevel;

	void	InitLevelTable(iAllocator& IN_allocator)
	{
		loadedLevels = Vector<LoadedLevel*>{ IN_allocator };
	}


	/************************************************************************************************/


	bool	LoadLevel(uint64_t ID, EngineCore& core)
	{
		if (bool res = CreateLevel(ID, core); !res)
			return false;

		auto res = std::ranges::find_if(loadedLevels,
			[&](auto level) -> bool
			{
				return level->ID == ID;
			});

		if (res == loadedLevels.end())
			return false;

		auto& loadedLevel = **res;

		if (!activeLevel)
			activeLevel = &loadedLevel;

		iAllocator& allocator = core.GetBlockMemory();

		// Load Test Scene
		SceneLoadingContext loadCtx{
			.scene = loadedLevel.level.scene,
			.layer = loadedLevel.level.layer,
			.nodes = Vector<FlexKit::NodeHandle>{ allocator  }
		};

		return LoadScene(core, loadCtx, ID);
	}


	/************************************************************************************************/


	Level*	GetLevel(uint64_t ID) noexcept
	{
		auto res = std::ranges::find_if(loadedLevels,
			[&](auto level) -> bool
			{
				return level->ID == ID;
			});

		if (res == loadedLevels.end())
			return nullptr;
		else
			return &(*res)->level;

		std::unreachable();
	}


	/************************************************************************************************/


	bool CreateLevel(uint64_t ID, EngineCore& core)
	{
		if (GetLevel(ID) != nullptr)
			return false;

		iAllocator& allocator	= core.GetBlockMemory();
		auto& physX				= PhysXComponent::GetComponent();
		auto& loadedLevel		= allocator.allocate<LoadedLevel>();

		loadedLevel.level.layer	= physX.CreateLayer(true);
		loadedLevel.allocator	= &allocator;
		loadedLevel.ID			= ID;
		loadedLevels.push_back(&loadedLevel);

		if (!activeLevel)
			activeLevel = &loadedLevel;

		return true;
	}


	/************************************************************************************************/


	uint64_t GetActiveLevelID() noexcept
	{
		return activeLevel->ID;
	}


	Level*	GetActiveLevel() noexcept
	{
		return activeLevel ? &activeLevel->level : nullptr;
	}


	/************************************************************************************************/


	void	SetActiveLevel(uint64_t ID) noexcept
	{
		auto res = std::ranges::find_if(loadedLevels,
			[&](auto level) -> bool
			{
				return level->ID == ID;
			});

		if (res == loadedLevels.end())
			return;
		else
			activeLevel = *res;
	}


	/************************************************************************************************/


	void ReleaseLevel(uint64_t ID) noexcept
	{
		auto res = std::ranges::find_if(loadedLevels,
			[&](auto level) -> bool
			{
				return level->ID == ID;
			});

		if (res == loadedLevels.end())
			return;

		auto& physX = PhysXComponent::GetComponent();

		LoadedLevel* loaded= *res;
		loaded->level.scene.ClearScene();
		physX.ReleaseScene(loaded->level.layer);

		loaded->allocator->release(*loaded);

		loadedLevels.remove_unstable(res);
	}


	/************************************************************************************************/


	void ReleaseAllLevels() noexcept
	{
		activeLevel = nullptr;

		auto& physX = PhysXComponent::GetComponent();
		for (auto& levelEntry : loadedLevels)
		{
			levelEntry->level.scene.ClearScene();
			physX.ReleaseScene(levelEntry->level.layer);

			levelEntry->allocator->release(*levelEntry);
		}

		loadedLevels.Release();
	}


}	/************************************************************************************************/
