#RTExperiment

project(RTExperiment)

if(NOT ANDROID)
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


#    set_property(TARGET RTExperiment PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    target_link_libraries(RTExperiment PRIVATE flex dxBackend dxHeaders)
    add_dependencies(RTExperiment ASSETS)

    if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
        file(CREATE_LINK ${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC)
    endif()

    Flex_LinkAssets(RTExperiment)
endif()
