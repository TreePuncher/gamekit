#project root
cmake_minimum_required(VERSION 3.23)
project(FlexEngine LANGUAGES CXX VERSION 0.0.1)

add_subdirectory(AnimationTest)
add_subdirectory(HairRendering)
add_subdirectory(MergePathSortTest)
add_subdirectory(MultiThread_UnitTests) # Needs moving to a new unit test framework
add_subdirectory(PhysicsTest)
