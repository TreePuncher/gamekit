#pragma once
#include <ResourceHandles.hpp>

namespace FlexKit
{
	struct ITextureManager
	{
		virtual void LoadLowestLevel(ResourceHandle, CopyContextHandle) = 0;
		virtual void BindAsset(GUID_t, ResourceHandle) = 0;
	};

	struct NullTextureManager_t : ITextureManager
	{
		void LoadLowestLevel(ResourceHandle, CopyContextHandle) final {}
		void BindAsset(GUID_t, ResourceHandle) final {}
	}	inline NullTextureManager;
}
