#ShapeKeys

project(ShapeKeys)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES "*.cpp")

add_executable(
    ShapeKeys
    ${CPP_FILES}
)

target_include_directories(
    ShapeKeys
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET ShapeKeys PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(ShapeKeys PRIVATE flex flex_optional)

Flex_CopyAssets(ShapeKeys)
