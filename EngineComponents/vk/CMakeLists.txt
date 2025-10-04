cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Directx-dxc REQUIRED)

add_library(vkBackend)

#find_path(dxc_dir dxcapi.h)
#target_include_directories(vkBackend PRIVATE dxc_dir)

target_link_libraries(vkBackend PRIVATE flex)
target_link_libraries(vkBackend PRIVATE vk-bootstrap::vk-bootstrap)
target_link_libraries(vkBackend PRIVATE Vulkan::Vulkan)

target_sources(
        vkBackend PRIVATE
        vkBackend.cpp
        "src/vkCopyContext.cpp"
        "src/vkDescriptorHeap.cpp"
        "src/vkDirectContext.cpp"
        "src/vkPipelineLayout.cpp"
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
    "src/vkRenderSystem.hpp"
    "src/vkResourceTable.hpp"
)

if(WIN32)
target_compile_definitions(vkBackend PRIVATE VK_USE_PLATFORM_WIN32_KHR=1)
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
