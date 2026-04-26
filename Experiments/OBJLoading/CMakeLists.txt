#OBJLoading

project(OBJLoading)

find_package(zlib-ng CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    OBJLoading
    ObjLoading.cpp
    main.cpp
)

set_property(TARGET OBJLoading PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(OBJLoading PRIVATE flex vkBackend)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
    file(CREATE_LINK ${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC)
endif()

#add_dependencies(OBJLoading ASSETS)

#Flex_LinkAssets(OBJLoading)
