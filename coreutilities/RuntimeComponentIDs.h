#pragma once

#include "type.h"

namespace FlexKit
{
    constexpr ComponentID AnimatorComponentID           = GetTypeGUID(Animator);
    constexpr ComponentID BindPointComponentID          = GetTypeGUID(BindPoint);
    constexpr ComponentID BrushComponentID              = GetTypeGUID(Brush);
    constexpr ComponentID CameraComponentID             = GetTypeGUID(CameraComponentID);
    constexpr ComponentID FABRIKComponentID             = GetTypeGUID(FABRIK);
    constexpr ComponentID FABRIKTargetComponentID       = GetTypeGUID(FABRIKTarget);
    constexpr ComponentID MaterialComponentID           = GetTypeGUID(Material);
    constexpr ComponentID PointLightShadowMapID         = GetTypeGUID(PointLighShadowCaster);
    constexpr ComponentID PointLightComponentID         = GetTypeGUID(PointLight);
    constexpr ComponentID SceneVisibilityComponentID    = GetTypeGUID(SceneVisibility);
    constexpr ComponentID SkeletonComponentID           = GetTypeGUID(Skeleton);
    constexpr ComponentID TransformComponentID          = GetTypeGUID(TransformComponent);

    constexpr uint32_t PassHandleID = GetTypeGUID(PassHandle);
}
