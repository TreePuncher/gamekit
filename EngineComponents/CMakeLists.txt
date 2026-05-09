cmake_minimum_required(VERSION 3.25)

add_subdirectory(BackendShared)
if(WIN32)
add_subdirectory(DX12)
endif(WIN32)

message("${CMAKE_SYSTEM_NAME}")
if(${CMAKE_SYSTEM_NAME} EQUAL "android")
else()
    message("Enabling Vulkan")
    add_subdirectory(vk)
endif()

if(DEFINED ANDROID_PLATFORM)
    add_subdirectory(android)
else()
    add_subdirectory(PhysX)
endif()

add_subdirectory(HighQualityRendering)

