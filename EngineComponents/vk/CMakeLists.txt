cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_library(vkBackend)

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
    "include/vkWin32Surface.hpp"

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

target_include_directories(vkBackend
    PUBLIC
    "include"
    PRIVATE
    "src"
)

install(TARGETS vkBackend DESTINATION lib)
