#project root
cmake_minimum_required(VERSION 3.23)
project(FlexEngine LANGUAGES CXX VERSION 0.0.1)

if(DEFINED ANDROID_PLATFORM)
    add_subdirectory(Android)
    add_subdirectory(AndroidIO)
else()
endif()

add_subdirectory(AnimationTest)
add_subdirectory(HairRendering)
add_subdirectory(MergePathSortTest)
add_subdirectory(MultiThread_UnitTests) # Needs moving to a new unit test framework
add_subdirectory(PhysicsTest)
add_subdirectory(PrefetchExperiment)
add_subdirectory(ProceduralGeneration)
add_subdirectory(ShapeKeyTest)
add_subdirectory(SignalsAndSlotsTest)
add_subdirectory(Tessellation)
add_subdirectory(TextureStreamingTest)
#add_subdirectory(ReflectionExperiment)
add_subdirectory(BasicRaytracer)
