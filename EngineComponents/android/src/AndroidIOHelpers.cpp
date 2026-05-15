#include "AndroidIOHelpers.hpp"
#include "native_app_glue/android_native_app_glue.h"
#include "game-activity/GameActivity.h"

std::filesystem::path GetGameAssetPath(struct android_app* pApp)
{
    using std::filesystem::path;

    return path{ path{ pApp->activity->internalDataPath }.parent_path().string() + "/gameAssets"};
}