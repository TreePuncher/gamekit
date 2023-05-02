#pragma once
#include "ResourceHandles.h"
#include "containers.h"

namespace FlexKit
{
	class Scene;

	using PostLoadAction = TypeErasedCallable<void(Scene&)>;

	struct SceneLoadingContext
	{
		Scene&					scene;
		LayerHandle				layer;
		Vector<NodeHandle>		nodes;
		Vector<PostLoadAction>	pendingActions;
	};
}
