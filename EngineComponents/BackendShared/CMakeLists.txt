cmake_minimum_required(VERSION 3.25)
project(flex)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(BackendShared
	"src/ShaderUtilities.cpp"
)

target_sources(
    	BackendShared PUBLIC
    	FILE_SET backendshared_headers
    	TYPE HEADERS
    	FILES
    	"include/ShaderUtilities.hpp"
)

target_include_directories(
	BackendShared
    	PUBLIC
    	"include"
    	"include"
    )

target_link_libraries(BackendShared PRIVATE flex)
