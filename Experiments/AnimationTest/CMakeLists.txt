#AnimationTest

project(AnimationTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    AnimationTest
    AnimationTest.cpp
    main.cpp
    pch.cpp
    Player.cpp
)

target_include_directories(
    AnimationTest
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET AnimationTest PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(AnimationTest PRIVATE flex flex_optional)

Flex_CopyAssets(AnimationTest)
