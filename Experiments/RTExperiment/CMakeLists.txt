#RTExperiment

project(RTExperiment)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES "*.cpp")

add_executable(
    RTExperiment
    ${CPP_FILES}
)

target_include_directories(
    RTExperiment
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET RTExperiment PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(RTExperiment PRIVATE flex flex_optional)
add_dependencies(RTExperiment ASSETS)

Flex_LinkAssets(RTExperiment)
