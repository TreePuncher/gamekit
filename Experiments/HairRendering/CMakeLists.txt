#HairRendering

project(FlexEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    HairRendering
    Main.cpp
    HairRendering.cpp
)

target_include_directories(HairRendering PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(HairRendering PRIVATE flex)

file(CREATE_LINK ${FLEX_ROOT}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets SYMBOLIC)

Flex_CopyAssets(HairRendering)
