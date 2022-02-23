#pragma once

#include "type.h"

namespace FlexKit
{
    constexpr uint32_t AnimatorComponentID           = GetTypeGUID(Animator);
    constexpr uint32_t BindPointComponentID          = GetTypeGUID(BindPoint);
    constexpr uint32_t BrushComponentID              = GetTypeGUID(Brush);
    constexpr uint32_t CameraComponentID             = GetTypeGUID(CameraComponentID);
    constexpr uint32_t FABRIKComponentID             = GetTypeGUID(FABRIK);
    constexpr uint32_t FABRIKTargetComponentID       = GetTypeGUID(FABRIKTarget);
    constexpr uint32_t MaterialComponentID           = GetTypeGUID(Material);
    constexpr uint32_t PointLightShadowMapID         = GetTypeGUID(PointLighShadowCaster);
    constexpr uint32_t PointLightComponentID         = GetTypeGUID(PointLight);
    constexpr uint32_t SceneVisibilityComponentID    = GetTypeGUID(SceneVisibility);
    constexpr uint32_t SkeletonComponentID           = GetTypeGUID(Skeleton);
    constexpr uint32_t StringComponentID             = GetTypeGUID(StringID);
    constexpr uint32_t TransformComponentID          = GetTypeGUID(TransformComponent);

    constexpr uint32_t PassHandleID = GetTypeGUID(PassHandle);
}
