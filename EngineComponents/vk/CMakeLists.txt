cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Directx-dxc REQUIRED)
find_package(spirv_cross_core REQUIRED)
find_package(spirv_cross_util REQUIRED)

add_library(vkBackend)

target_link_libraries(vkBackend PRIVATE flex)
target_link_libraries(vkBackend PRIVATE vk-bootstrap::vk-bootstrap)
target_link_libraries(vkBackend PRIVATE Vulkan::Vulkan)
target_link_libraries(vkBackend PRIVATE spirv-cross-core)
target_link_libraries(vkBackend PRIVATE spirv-cross-util)

target_compile_definitions(vkBackend PUBLIC SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)

target_sources(
        vkBackend PRIVATE
        vkBackend.cpp
        "src/vkCopyContext.cpp"
        "src/vkDescriptorHeap.cpp"
        "src/vkDirectContext.cpp"
        "src/vkPipelineLayout.cpp"
        "src/vkPipelineBuilder.cpp"
        "src/vkPipelineState.cpp"
        "src/vkRendersystem.cpp"
        "src/vkResourceTable.cpp"
        "src/vkWaylandSurface.cpp"
)

if(WIN32)
    target_sources(
            vkBackend PRIVATE
            "src/vkWin32Surface.cpp"
    )
endif(WIN32)

target_sources(
    vkBackend PUBLIC
    FILE_SET vkbackend_headers
    TYPE HEADERS
    FILES
    "include/vkBackend.hpp"
    "include/vkSurface.hpp"

    "src/vkCopyContext.hpp"
    "src/vkDescriptorHeap.hpp"
    "src/vkDirectContext.hpp"
    "src/vkPipelineLayout.hpp"
    "src/vkPipelineBuilder.hpp"
    "src/vkPipelineState.hpp"
    "src/vkRenderSystem.hpp"
    "src/vkResourceTable.hpp"
)

if(WIN32)
target_compile_definitions(vkBackend PRIVATE VK_USE_PLATFORM_WIN32_KHR=1)
target_link_libraries(vkBackend PRIVATE Microsoft::DXIL)

#find_library(spirvReflect spirv-reflect-static.lib)
#find_library(spirv_cross_core spirv-cross-core.lib)
#find_library(spirv_cross_cpp spirv-cross-cpp.lib)
#find_library(spirv_cross_reflect unofficial::spirv-reflect)

target_link_libraries(vkBackend PRIVATE ${spirvReflect})
#target_link_libraries(vkBackend PRIVATE ${spirv_cross_cpp})
#target_link_libraries(vkBackend PRIVATE ${spirv_cross_reflect})

endif(WIN32)

if(UNIX)
    target_sources(
            vkBackend PUBLIC
            "src/xdg-shell-protocol.cpp"
    )

    find_library (dxc_lib_dir libdxcompiler.so)
    find_library (dxil_lib_dir libdxil.so)

    target_link_libraries(vkBackend PRIVATE ${dxc_lib_dir})
    target_link_libraries(vkBackend PRIVATE ${dxil_lib_dir})
    target_link_libraries(vkBackend PRIVATE libwayland-client.so)
    target_compile_definitions(vkBackend PRIVATE VK_USE_PLATFORM_WAYLAND_KHR=1)
    target_compile_definitions(vkBackend PRIVATE VK_USE_PLATFORM_XCB_KHR=1)
    target_compile_definitions(vkBackend PRIVATE VK_USE_PLATFORM_XLIB_KHR=1)

endif(UNIX)

target_include_directories(vkBackend
    PUBLIC
    "include"
    PRIVATE
    "src"
)

install(TARGETS vkBackend DESTINATION lib)
