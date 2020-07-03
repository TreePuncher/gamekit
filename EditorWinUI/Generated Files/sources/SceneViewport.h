#pragma once
#include "SceneViewport.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.
static_assert(false, "Do not compile generated C++/WinRT source files directly");

namespace winrt::EditorWinUI::implementation
{
    struct SceneViewport : SceneViewportT<SceneViewport>
    {
        SceneViewport() = default;

    };
}
namespace winrt::EditorWinUI::factory_implementation
{
    struct SceneViewport : SceneViewportT<SceneViewport, implementation::SceneViewport>
    {
    };
}
