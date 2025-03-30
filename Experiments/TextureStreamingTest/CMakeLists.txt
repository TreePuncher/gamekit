#TextureStreamingTest

project(TextureStreamingTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES "*.cpp")

add_executable(
    TextureStreamingTest
    ${CPP_FILES}
)

target_include_directories(
    TextureStreamingTest
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET TextureStreamingTest PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(TextureStreamingTest PRIVATE flex flex_optional)
add_dependencies(TextureStreamingTest ASSETS)

Flex_LinkAssets(TextureStreamingTest)
