#AnimationTest

project(PhysicsTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    PhysicsTest
    main.cpp
    pch.cpp
    PhysicsTest.cpp
    Player.cpp
)

target_include_directories(
    PhysicsTest
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

target_link_libraries(PhysicsTest PRIVATE flex flex_optional)

Flex_LinkAssets(PhysicsTest)
