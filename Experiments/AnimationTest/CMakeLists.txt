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

set_property(TARGET AnimationTest PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(AnimationTest PRIVATE flex flex_optional vkBackend)
add_dependencies(AnimationTest ASSETS)

Flex_LinkAssets(AnimationTest)
