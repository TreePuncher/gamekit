#imguiExmple

project(rmluiexample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    rmluiexample
    main.cpp
)

set_property(TARGET rmluiexample PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(rmluiexample PRIVATE flex dxBackend)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    #file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
    #file(CREATE_LINK ${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC)
endif()

add_dependencies(rmluiexample ASSETS)
Flex_LinkAssets(rmluiexample)
