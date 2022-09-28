#pragma once
#include "ResourceHandles.h"
#include "containers.h"

namespace FlexKit
{
    class Scene;

    struct SceneLoadingContext
    {
        Scene&              scene;
        LayerHandle         layer;
        Vector<NodeHandle>  nodes;
    };
}
