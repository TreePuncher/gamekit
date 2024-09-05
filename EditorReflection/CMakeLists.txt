#EditorReflection
cmake_minimum_required(VERSION 3.23)

project(EditorReflection)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_library(zlib_lib zlib.lib)
find_library(zstd_lib zstd.lib)

set(clangLibs ${FLEX_ROOT}/ThirdParty/libs)

file(GLOB headerFiles *.hpp)
file(GLOB sourceFiles *.cpp)

ADD_LIBRARY(EditorReflection ${sourceFiles})

target_sources(
    EditorReflection PUBLIC
    FILE_SET reflection_headers
    TYPE HEADERS
    FILES ${headerFiles})

target_include_directories(EditorReflection PUBLIC ${FLEX_ROOT}/ThirdParty)
target_link_directories(EditorReflection PUBLIC ${clangLibs})

target_link_libraries(EditorReflection PUBLIC
    libclang clangAST clangBasic LLVMFrontendOpenMP
    clangParse clangLex LLVM-C
    clangToolingCore)

install(TARGETS EditorReflection DESTINATION lib)
install(TARGETS EditorReflection FILE_SET reflection_headers)
