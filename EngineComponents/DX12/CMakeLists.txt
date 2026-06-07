cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dxBackend
	"src/DDSUtilities.cpp"
	"src/dxBackend.cpp"
	"src/dxPipelineBuilder.cpp"
	"src/dxContext.cpp"
    "src/dxDescriptorSet.cpp"
	"src/dxRenderSystem.cpp"
    "src/dxUpload.cpp"
	"src/dxPipelineState.cpp"
    "src/dxVertexBufferSet.cpp"
	"src/Win32Graphics.cpp"
)

target_sources(
    dxBackend PUBLIC
    FILE_SET dxbackend_headers
    TYPE HEADERS
    FILES
    "include/dxBackend.hpp"
    "include/Win32Graphics.hpp"

    "src/DDSUtilities.hpp"
	"src/dxPipelineBuilder.hpp"
    "src/dxContext.hpp"
    "src/dxDescriptorSet.hpp"
    "src/dxIndirectLayout.hpp"
    "src/dxFrameBufferedObjects.hpp"
    "src/dxRenderSystem.hpp"
    "src/dxUpload.hpp"
    "src/dxVertexBufferSet.hpp"
    "src/dxPipelineState.hpp"
)

target_link_libraries(dxBackend PRIVATE flex BackendShared shaderpreprocessor)

target_include_directories(dxBackend
    PUBLIC
    "include"
    "include"
    )

install(TARGETS dxBackend DESTINATION lib)

set(WINPIX_LOCATION "${FLEX_ROOT}/ThirdParty/pix/bin/x64/")
ADD_LIBRARY(WINPIX SHARED IMPORTED)
SET_PROPERTY(TARGET WINPIX PROPERTY IMPORTED_IMPLIB     ${WINPIX_LOCATION}/WinPixEventRuntime.lib)
SET_PROPERTY(TARGET WINPIX PROPERTY IMPORTED_LOCATION   ${WINPIX_LOCATION}/WinPixEventRuntime.dll)

target_include_directories(dxBackend PUBLIC ${FLEX_ROOT}/ThirdParty/pix/Include/WinPixEventRuntime)    


add_library(dxHeaders INTERFACE)
target_include_directories(dxHeaders INTERFACE "src")

#add_custom_command(TARGET dxBackend POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy ${FLEX_ROOT}/ThirdParty/Pix/bin/x64/WinPixEventRuntime.dll ${CMAKE_CURRENT_BINARY_DIR}
#    COMMAND_EXPAND_LISTS)
#
