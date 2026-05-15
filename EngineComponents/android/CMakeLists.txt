cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)

add_library(androidglue
    STATIC
    src/android_main.cpp
	src/common/system_utils.cpp
    src/game-activity/GameActivity.cpp
    src/game-activity/GameActivityEvents.cpp
    src/game-activity/native_app_glue/android_native_app_glue.cpp
    src/game-text-input/gametextinput.cpp
    src/AndroidRenderWindow.cpp
    src/AndroidIOHelpers.cpp
)

target_sources(
    androidglue PUBLIC
    FILE_SET androidglue_headers
    TYPE HEADERS
    FILES
	"include/common/gamesdk_common.h"
	"include/system_utils.h"
	"include/game-activity/native_app_glue/android_native_app_glue.h"
    "include/game-activity/GameActivity.h"
    "include/game-activity/GameActivityEvents.h"
    "include/game-activity/GameActivityLog.h"

    "include/game-text-input/gametextinput.h"
    "include/GameActivity.h"
    "include/GameActivityEvents.h"
    "include/GameActivityLog.h"
    "include/AndroidIOHelpers.hpp"
)

target_link_libraries(androidglue PRIVATE android log vkBackend flex vkHeaders)
target_link_libraries(androidglue PRIVATE Vulkan::Vulkan)
target_link_libraries(androidglue PRIVATE vulkan)
target_link_libraries(androidglue PRIVATE vk-bootstrap::vk-bootstrap)


target_include_directories(androidglue
    PUBLIC
    "include"
    "include"
    )


install(TARGETS androidglue DESTINATION lib)