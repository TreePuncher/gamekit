#imguiExmple

project(imguiexample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

add_executable(
    imguiexample
    main.cpp
)

set_property(TARGET imguiexample PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(imguiexample PRIVATE flex dxBackend imgui::imgui imguizmo::imguizmo implot::implot)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
    file(CREATE_LINK ${FLEX_ROOT}/shaders ${PROJECT_BINARY_DIR}/assets/shaders SYMBOLIC COPY_ON_ERROR)
endif()
        
add_dependencies(imguiexample ASSETS)
Flex_LinkAssets(imguiexample)
