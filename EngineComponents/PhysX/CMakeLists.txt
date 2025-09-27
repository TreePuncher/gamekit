cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)

add_library(PhysxComponet
    "src/PhysicsDebugVis.cpp"
    "src/physicsutilities.cpp"
)

target_sources(
    PhysxComponet
    PUBLIC
    FILE_SET physxcomponent_headers
    TYPE HEADERS
    FILES
    "include/PhysicsUtilities.hpp"
    "include/PhysicsDebugVis.hpp"
)

target_include_directories(PhysxComponet
    PUBLIC
    "include"
    PRIVATE
    "src"
)

target_link_libraries(PhysxComponet PRIVATE unofficial::omniverse-physx-sdk::sdk flex)

install(TARGETS PhysxComponet FILE_SET physxcomponent_headers)
install(TARGETS PhysxComponet DESTINATION lib)
