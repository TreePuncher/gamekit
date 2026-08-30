cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

Add_Executable(DoubleMathTests DoubleTests.cpp)
target_link_libraries(DoubleMathTests MathUtilities)
