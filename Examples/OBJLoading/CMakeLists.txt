#OBJLoading

project(OBJLoadingExample)

find_package(zlib-ng CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    OBJLoadingExample
    main.cpp
)

set_property(TARGET OBJLoadingExample PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(OBJLoadingExample PRIVATE exampleframework objloader)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
endif()

file(COPY "${PROJECT_SOURCE_DIR}/shaders" DESTINATION ${PROJECT_BINARY_DIR}/assets)
file(COPY "${FLEX_ROOT}/shaders/imgui" DESTINATION ${PROJECT_BINARY_DIR}/assets/shaders)
file(COPY_FILE "${PROJECT_SOURCE_DIR}/test.obj" "${PROJECT_BINARY_DIR}/test.obj" ONLY_IF_DIFFERENT)

