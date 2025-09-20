cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(D3D12Backend
	DX12Graphics.cpp
	DX12Graphics.hpp)

#set(WINPIX_LOCATION "${FLEX_ROOT}/ThirdParty/pix/bin/x64/")
#ADD_LIBRARY(WINPIX SHARED IMPORTED)
#SET_PROPERTY(TARGET WINPIX PROPERTY IMPORTED_IMPLIB     ${WINPIX_LOCATION}/WinPixEventRuntime.lib)
#SET_PROPERTY(TARGET WINPIX PROPERTY IMPORTED_LOCATION   ${WINPIX_LOCATION}/WinPixEventRuntime.dll)

#target_include_directories(D3D12Backend PUBLIC ${FLEX_ROOT}/ThirdParty/pix/Include/WinPixEventRuntime)    

#add_custom_command(TARGET flex POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy ${FLEX_ROOT}/ThirdParty/Pix/bin/x64/WinPixEventRuntime.dll ${CMAKE_CURRENT_BINARY_DIR}
#    COMMAND_EXPAND_LISTS)