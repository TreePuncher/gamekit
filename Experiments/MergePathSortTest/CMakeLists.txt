#MergePath

project(MergePathExperiment)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    MergePath
    extra.cpp
    main.cpp
    SortingTest.cpp
)

target_include_directories(MergePath PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(MergePath PRIVATE flex)

add_dependencies(MergePath ASSETS)

Flex_LinkAssets(MergePath)
