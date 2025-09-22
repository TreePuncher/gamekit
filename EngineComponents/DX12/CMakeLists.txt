cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dxBackend
	"src/dxBackend.cpp"
	"src/dxContext.cpp"
    "src/dxDescriptorHeaps.cpp"
	"src/dxRenderSystem.cpp"
    "src/dxUpload.cpp"
	"src/PipelineState.cpp"
    "src/VertexBufferSet.cpp"
	"src/Win32Graphics.cpp"
)

target_sources(
    dxBackend PUBLIC
    FILE_SET dxbackend_headers
    TYPE HEADERS
    FILES
    "include/dxBackend.hpp"
    "include/Win32Graphics.hpp"

	"src/dxContext.hpp"
    "src/dxDescriptorHeaps.hpp"
    "src/dxIndirectLayout.hpp"
    "src/dxFrameBufferedObjects.hpp"
    "src/dxRenderSystem.hpp"
    "src/dxUpload.hpp"
    "src/VertexBufferSet.hpp"
    "src/PipelineState.hpp"
)

target_link_libraries(dxBackend PRIVATE flex)

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

#add_custom_command(TARGET dxBackend POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy ${FLEX_ROOT}/ThirdParty/Pix/bin/x64/WinPixEventRuntime.dll ${CMAKE_CURRENT_BINARY_DIR}
#    COMMAND_EXPAND_LISTS)
#
