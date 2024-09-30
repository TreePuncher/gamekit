#Reflection Test
cmake_minimum_required(VERSION 3.23)

if(WIN32 AND BuildEditor)
    project(Reflection)

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(fmt CONFIG REQUIRED)

    find_library(zlib_lib zlib.lib)
    find_library(zstd_lib zstd.lib)
    file(GLOB sourceFiles *.cpp)

    add_executable(ReflectionTest ${sourceFiles})
    target_link_directories(ReflectionTest PRIVATE ${libClangLibDir})
    target_link_libraries(ReflectionTest PRIVATE fmt::fmt ${zlib_lib} ${zstd_lib} ws2_32.lib Version.lib EditorReflection flex)

    file(TO_NATIVE_PATH ${FLEX_ROOT}/core/include SOURCEPATH)
    file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/assets TARGETPATH)

    add_custom_command(TARGET ReflectionTest POST_BUILD
        COMMAND @ECHO OFF || mklink /j ${TARGETPATH} ${SOURCEPATH} > nul 2> nul || echo "")

    file(TO_NATIVE_PATH ${PROJECT_SOURCE_DIR}/testHeaders SOURCEPATH)
    file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/testHeaders TARGETPATH)

    add_custom_command(TARGET ReflectionTest POST_BUILD
        COMMAND @ECHO OFF || mklink /j ${TARGETPATH} ${SOURCEPATH} > nul 2> nul || echo "")

    add_custom_command(TARGET ReflectionTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:ReflectionTest> $<TARGET_FILE_DIR:ReflectionTest>
        COMMAND ${CMAKE_COMMAND} -E copy ${FLEX_ROOT}/ThirdParty/libs/libclang.dll   "$<TARGET_FILE_DIR:ReflectionTest>"
        COMMAND_EXPAND_LISTS)
endif()

