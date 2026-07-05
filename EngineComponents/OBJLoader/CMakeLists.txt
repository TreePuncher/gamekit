cmake_minimum_required(VERSION 3.23)

project(objLoader LANGUAGES CXX VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(objloader "src/objloader.cpp")

target_sources(
    objloader PUBLIC
    FILE_SET objloader_headers
    TYPE HEADERS
    FILES
    "includes/objloader.hpp"
)

target_include_directories(
    objloader PUBLIC
    "includes"
)

target_link_libraries(objloader PRIVATE flex)
