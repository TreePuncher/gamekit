cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vkBackend)

target_link_libraries(vkBackend PRIVATE flex)

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
    "public/vkBackend.hpp"
    "src/vkRenderSystem.hpp"
    "src/vkDirectContext.hpp"
    "src/vkCopyContext.hpp"
)

target_include_directories(flex
    PUBLIC
    "include"
    "public"
    )

install(TARGETS vkBackend DESTINATION lib)
