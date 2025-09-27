cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(TextureStreaming TextureStreamingUtilities.cpp)

target_sources(TextureStreaming PUBLIC TextureStreamingUtilities.hpp)

target_link_libraries(TextureStreaming PUBLIC flex HighQualityRenderingComponent)
