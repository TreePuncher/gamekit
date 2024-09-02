#EditorReflection
cmake_minimum_required(VERSION 3.23)

project(EditorReflection)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)

set(CLANG_LIBS clangIndex)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_INCLUDE_DIRS})

find_library(zlib_lib zlib.lib)
find_library(zstd_lib zstd.lib)
find_library(clangToolingLibDir clangTooling.lib)
cmake_path(GET clangToolingLibDir PARENT_PATH libClangLibDir)
llvm_map_components_to_libnames(llvm_libs support core irreader)

find_library(clangToolingLibDir clangTooling.lib)

cmake_path(GET clangToolingLibDir PARENT_PATH libClangLibDir)
llvm_map_components_to_libnames(llvm_libs support core irreader)

file(GLOB headerFiles *.hpp)
file(GLOB sourceFiles *.cpp)

ADD_LIBRARY(EditorReflection ${sourceFiles})

target_sources(
    EditorReflection PUBLIC
    FILE_SET reflection_headers
    TYPE HEADERS
    FILES ${headerFiles})

target_link_libraries(EditorReflection PUBLIC
    libclang clangAST clangBasic LLVMFrontendOpenMP
    clangParse clangLex LLVM-C
    clangToolingCore)

install(TARGETS EditorReflection DESTINATION lib)
install(TARGETS EditorReflection FILE_SET reflection_headers)
