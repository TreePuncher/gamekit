cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)

add_library(PhysxComponent
    "src/PhysicsDebugVis.cpp"
    "src/physicsutilities.cpp"
)

target_sources(
    PhysxComponent
    PUBLIC
    FILE_SET physxcomponent_headers
    TYPE HEADERS
    FILES
    "include/PhysicsUtilities.hpp"
    "include/PhysicsDebugVis.hpp"
)

target_include_directories(PhysxComponent
    PUBLIC
    "include"
    PRIVATE
    "src"
)

target_link_libraries(PhysxComponent PUBLIC unofficial::omniverse-physx-sdk::sdk flex)

install(TARGETS PhysxComponent FILE_SET physxcomponent_headers)
install(TARGETS PhysxComponent DESTINATION lib)
