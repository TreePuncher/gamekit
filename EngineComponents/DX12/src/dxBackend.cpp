#include <src/dxRenderSystem.hpp>

namespace FlexKit
{
	struct IRenderSystem* CreateDX(const struct RenderSystemOptions& options)
	{
		return options.allocator->allocate_aligned<dx_Internal::dxRenderSystem>(options.allocator, options.threads);
	}
}
