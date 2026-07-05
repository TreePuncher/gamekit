#imguiExmple

project(signalsexample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    signalsexample
    main.cpp
)

target_link_libraries(signalsexample PRIVATE exampleframework)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
    file(CREATE_LINK ${FLEX_ROOT}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC COPY_ON_ERROR)
endif()
        
add_dependencies(signalsexample ASSETS)
Flex_LinkAssets(signalsexample)
