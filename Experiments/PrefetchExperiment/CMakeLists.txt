#Prefetch Experiment
project(Reflection)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)

add_executable(PrefetchExperiment main.cpp)

target_link_libraries(PrefetchExperiment PRIVATE flex)


