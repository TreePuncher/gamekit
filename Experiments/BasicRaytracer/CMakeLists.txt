#RTExperiment

project(BasicRaytracer)

if(NOT ANDROID)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    file (GLOB CPP_FILES "*.cpp")

    add_executable(
        BasicRaytracer
        ${CPP_FILES}
    )

    target_sources(
    BasicRaytracer PUBLIC
    FILE_SET rtHeaders
    TYPE HEADERS
    FILES
    "RTExperiment.hpp"
    "ShaderBindingTable.hpp"
    "PersistentGPUAllocator.hpp"
    "OBJLoader.hpp"
    )

    target_include_directories(
        BasicRaytracer
        PUBLIC
        flex_optional
        ${PROJECT_SOURCE_DIR})


#    set_property(TARGET BasicRaytracer PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    target_link_libraries(BasicRaytracer PRIVATE flex dxBackend dxHeaders)
    add_dependencies(BasicRaytracer ASSETS)

    if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
        file(CREATE_LINK ${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC)
    endif()

    Flex_LinkAssets(BasicRaytracer)
endif()
