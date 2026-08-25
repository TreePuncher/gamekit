#OBJLoading

project(HighQualityRendering)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    HighQualityRenderingExample
    src/main.cpp
)

target_link_libraries(HighQualityRenderingExample PRIVATE exampleframework objloader HighQualityRenderingComponent TextureStreaming AngelScriptComponents)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/assets)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
endif()

file(COPY "${FLEX_ROOT}/shaders" DESTINATION ${PROJECT_BINARY_DIR}/assets)

