#MultiThread_UnitTests

project(MultiThread_UnitTests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MSUnitTestFramework REQUIRED)
find_program(VSTest_EXECUTABLE NAMES vstest.console.exe REQUIRED)

add_library(
    MultiThread_UnitTests
    stdafx.cpp
    MT_UnitTests.cpp
)

target_include_directories(MultiThread_UnitTests PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(MultiThread_UnitTests PRIVATE flex)
target_link_libraries(MultiThread_UnitTests PRIVATE MSUnitTestFramework::MSUnitTestFramework)
add_test(NAME mytest COMMAND "${VSTest_EXECUTABLE}" "$<TARGET_FILE:MultiThread_UnitTests>")

file(CREATE_LINK ${FLEX_ROOT}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets SYMBOLIC)
