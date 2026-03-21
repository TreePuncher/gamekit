#AnimationTest

project(AnimationTest)

find_package(zlib-ng CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    AnimationTest
    AnimationTest.cpp
    main.cpp
    pch.cpp
    Player.cpp
)

set_property(TARGET AnimationTest PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(AnimationTest PRIVATE flex flex_optional vkBackend PhysxComponent HighQualityRenderingComponent TextureStreaming zlib-ng::zlib)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
    file(CREATE_LINK ${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC)
endif()

#add_dependencies(AnimationTest ASSETS)

#Flex_LinkAssets(AnimationTest)
