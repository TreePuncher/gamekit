#Reflection Test
cmake_minimum_required(VERSION 3.23)

if(WIN32 AND BuildEditor)
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
    llvm_map_components_to_libnames(llvm_libs support core irreader)

    file(GLOB sourceFiles *.cpp)

    add_executable(ReflectionTest ${sourceFiles})
    target_link_directories(ReflectionTest PRIVATE ${libClangLibDir})
    target_link_libraries(ReflectionTest PRIVATE fmt::fmt ${zlib_lib} ${zstd_lib} ws2_32.lib Version.lib EditorReflection)
    target_link_libraries(ReflectionTest PRIVATE
        libclang clangAST clangBasic LLVMFrontendOpenMP
        clangParse clangLex LLVM-C
        clangToolingCore)

    file(TO_NATIVE_PATH ${FLEX_ROOT}/core/include SOURCEPATH)
    file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/assets TARGETPATH)

    add_custom_command(TARGET ReflectionTest POST_BUILD
        COMMAND @ECHO OFF || mklink /j ${TARGETPATH} ${SOURCEPATH} > nul 2> nul || echo "")

    file(TO_NATIVE_PATH ${PROJECT_SOURCE_DIR}/testHeaders SOURCEPATH)
    file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/testHeaders TARGETPATH)

    add_custom_command(TARGET ReflectionTest POST_BUILD
        COMMAND @ECHO OFF || mklink /j ${TARGETPATH} ${SOURCEPATH} > nul 2> nul || echo "")
endif()

