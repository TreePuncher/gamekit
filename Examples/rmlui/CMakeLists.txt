#imguiExmple

project(rmluiexample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(RmlUi CONFIG REQUIRED)

add_executable(
    rmluiexample
    main.cpp
)

set_property(TARGET rmluiexample PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(rmluiexample PRIVATE exampleframework)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
endif()

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets/shaders)
    file(CREATE_LINK ${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC)
endif()

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets/fonts)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets/fonts)
endif()

file(COPY_FILE ${PROJECT_SOURCE_DIR}/CreatoDisplay-Black.otf ${PROJECT_BINARY_DIR}/assets/fonts/CreatoDisplay-Black.otf ONLY_IF_DIFFERENT)

add_dependencies(rmluiexample ASSETS)
Flex_LinkAssets(rmluiexample)
