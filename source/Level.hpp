#pragma once

#include "scene.h"
#include "physicsutilities.h"

namespace FlexKit
{
	class EngineCore;
	class PhysXComponent;

	struct Level
	{
		Scene		scene;
		LayerHandle	layer;
	};

	void	InitLevelTable	(iAllocator&);
	bool	LoadLevel		(uint64_t ID, EngineCore& core);
	Level*	GetLevel		(uint64_t ID) noexcept;

	bool		CreateLevel		(uint64_t, EngineCore& core);
	uint64_t	GetActiveLevelID() noexcept;
	Level*		GetActiveLevel	() noexcept;
	void		SetActiveLevel	(uint64_t ID) noexcept;

	void ReleaseLevel		(uint64_t ID) noexcept;
	void ReleaseAllLevels	() noexcept;
}
