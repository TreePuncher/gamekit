#project root
cmake_minimum_required(VERSION 3.23)
project(FlexEngine  LANGUAGES CXX VERSION 0.0.1)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(FLEX_ROOT ${PROJECT_SOURCE_DIR} CACHE STRING "FLEX ROOT")

option(BuildExamples "Build Examples" ON)
option(BuildExperiments "Build Experiments" ON)
option(BuildEditor "Build Editor" ON)

add_subdirectory(Core)
add_subdirectory(Components)


if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    file(TO_NATIVE_PATH ${FLEX_ROOT}/shaders ASSETSRCPATH)
    file(TO_NATIVE_PATH ${FLEX_ROOT}/assets/shaders ASSETDSTPATH)

    if(NOT EXISTS "${FLEX_ROOT}/assets")
        file(MAKE_DIRECTORY "${FLEX_ROOT}/assets")
    endif()

    add_custom_target(ASSETS ALL
        COMMAND mklink /j ${ASSETDSTPATH} ${ASSETSRCPATH} || echo "")
	
    if(BuildEditor)
        message("Building Editor")
        add_subdirectory(EditorReflection)
        add_subdirectory(Editor)
    endif()

    if(BuildExamples)
        message("Building Examples")
    endif()

    if(BuildExperiments)
        message("Building Experiments")
        add_subdirectory(Experiments)
    endif()
endif()

#Install
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/flex.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/flex.pc
    @ONLY)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/flex.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/spud-config-version.cmake
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/flex-config-version.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/flex-config.cmake
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/flex
)
