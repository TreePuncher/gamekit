cmake_minimum_required(VERSION 3.23)

project(FlexEngine  LANGUAGES CXX VERSION 0.1.1)
find_package(fmt REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AssetPacker src/AssetPacker.cpp)
target_link_libraries(AssetPacker PRIVATE flex)
