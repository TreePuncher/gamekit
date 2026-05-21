cmake_minimum_required(VERSION 3.23)

project(FlexEngine  LANGUAGES CXX VERSION 0.1.1)
find_package(fmt REQUIRED)
find_package(scn REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SCN_REGEX_BACKEND "Boost")

add_library(shaderpreprocessor "src/ShaderPreprocessor.cpp")

target_sources(
    shaderpreprocessor PUBLIC
    FILE_SET shaderpreprocessor_headers
    TYPE HEADERS
    FILES
    "includes/ShaderPreprocessor.hpp"
)

target_include_directories(
    shaderpreprocessor PUBLIC
    "includes"
)

target_link_libraries(shaderpreprocessor PRIVATE flex fmt::fmt-header-only  scn::scn)
