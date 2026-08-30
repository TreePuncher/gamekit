cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(SIMDE_INCLUDE_DIRS "simde")
target_include_directories(flex PUBLIC ${SIMDE_INCLUDE_DIRS})

add_library(MathUtilities)

target_sources(
    MathUtilities PRIVATE
    "src/MathUtilities.cpp")

target_sources(
    MathUtilities PUBLIC
    FILE_SET math_headers
    TYPE HEADERS
    FILES
    "include/MathUtilities.hpp"
)

target_include_directories(MathUtilities
    PUBLIC
    "include"
     ${SIMDE_INCLUDE_DIRS}
     ${FLEX_ROOT}/core/include
    )

install(TARGETS MathUtilities DESTINATION lib)

