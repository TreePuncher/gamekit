#Reflection Test
cmake_minimum_required(VERSION 3.23)
project(Reflection)

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

file(GLOB clangLibs ${libClangLibDir}/clang*.lib)
file(GLOB llvmLibs ${libClangLibDir}/LLVM*.lib)

file(GLOB sourceFiles *.cpp)

add_executable(ReflectionTest ${sourceFiles})
target_link_libraries(ReflectionTest PRIVATE fmt::fmt ${llvmLibs} ${clangLibs} ${zlib_lib} ${zstd_lib} ws2_32.lib Version.lib)

message("libs: ${clangLibs}")

