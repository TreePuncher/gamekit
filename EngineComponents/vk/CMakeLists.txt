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
    "src/vkRenderSystem.cpp"
    "src/vkDirectContext.cpp"
    "src/vkCopyContext.cpp"
)

target_sources(
    vkBackend PUBLIC
    FILE_SET vkbackend_headers
    TYPE HEADERS
    FILES
    "include/vkBackend.hpp"
    "src/vkRenderSystem.hpp"
    "src/vkDirectContext.hpp"
    "src/vkCopyContext.hpp"
)

target_include_directories(vkBackend
    PUBLIC
    "include"
    "include"
    )

install(TARGETS vkBackend DESTINATION lib)
