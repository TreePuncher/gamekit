#src
project(flex)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
        scn
        GIT_REPOSITORY  https://github.com/eliaskosunen/scnlib
        GIT_TAG         v4.0.1
        GIT_SHALLOW     TRUE
)
set(SCN_REGEX_BACKEND "Boost")

FetchContent_MakeAvailable(scn)

find_package(Angelscript CONFIG REQUIRED)
find_package(Boost COMPONENTS Boost::regex)
find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(loguru CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(PkgConfig)

set(SCN_REGEX_BACKEND "Boost")

add_library(AngelScriptComponents
    "src/AngelScriptAddons.cpp")

target_link_libraries(AngelScriptComponents PUBLIC Angelscript::angelscript)

add_library(flex
    "src/AnimationComponents.cpp"
    "src/AnimationRuntimeUtilities.cpp"
    "src/AnimationUtilities.cpp"
    "src/AnimationRendering.cpp"
    "src/Application.cpp"
    "src/Assets.cpp"
    "src/CameraComponent.cpp"
    "src/Components.cpp"
    "src/Console.cpp"
    "src/Brush.cpp"
    "src/DebugPanel.cpp"
    "src/DebugUI.cpp"
    "src/DefaultPipelineStates.cpp"
    "src/EngineCore.cpp"
    "src/FrameGraph.cpp"
    "src/GameFramework.cpp"
    "src/Geometry.cpp"
    "src/GPUAllocators.cpp"
    "src/Intersection.cpp"
    "src/Level.cpp"
    "src/Logging.cpp"
    "src/MathUtilities.cpp"
    "src/Materials.cpp"
    "src/MemoryUtilities.cpp"
    "src/MeshUtilities.cpp"
    "src/ModifiableShape.cpp"
    "src/NetworkUtilities.cpp"
    "src/Particles.cpp"
    "src/PersistentGPUAllocator.cpp"
    "src/ProfilingUtilities.cpp"
    "src/RenderSystemInterface.cpp"
    "src/RMLRenderer.cpp"
    "src/Scene.cpp"
    "src/ScriptingRuntime.cpp"
    "src/Serialization.cpp"
    "src/Signals.cpp"
    "src/TextureUtilities.cpp"
    "src/ThreadUtilities.cpp"
    "src/TimeUtilities.cpp"
    "src/Transforms.cpp"
    "src/TriggerComponent.cpp"
    "src/TriMeshResource.cpp"
)

target_sources(
    flex PUBLIC
    FILE_SET flex_headers
    TYPE HEADERS
    FILES
    "include/AnimationComponents.hpp"
    "include/AnimationRuntimeUtilities.hpp"
    "include/AnimationUtilities.hpp"
    "include/AnimationRendering.hpp"
    "include/Application.hpp"
    "include/Assets.hpp"
    "include/BuildSettings.hpp"
    "include/DoOnce.hpp"
    "include/CameraComponent.hpp"
    "include/CommonStructs.hpp"
    "include/CameraComponent.hpp"
    "include/ComponentBlobs.hpp"
    "include/Components.hpp"
    "include/Console.hpp"
    "include/Containers.hpp"
    "include/Brush.hpp"
    "include/DebugPanel.hpp"
    "include/DebugUI.hpp"
    "include/DefaultPipelineStates.hpp"
    "include/DepthBuffer.hpp"
    "include/DungeonGen.hpp"
    "include/EngineCore.hpp"
    "include/Events.hpp"
    "include/Fonts.hpp"
    "include/FrameGraph.hpp"
    "include/GameFramework.hpp"
    "include/GPUAllocators.hpp"
    "include/Geometry.hpp"
    "include/Handle.hpp"
    "include/Input.hpp"
    "include/Intersection.hpp"
    "include/KeycodesEnums.hpp"
    "include/KeyValueIDs.hpp"
    "include/Level.hpp"
    "include/Logging.hpp"
    "include/Materials.hpp"
    "include/MathUtilities.hpp"
    "include/MemoryUtilities.hpp"
    "include/MeshUtilities.hpp"
    "include/ModifiableShape.hpp"
    "include/NetworkUtilities.hpp"
    "include/Particles.hpp"
    "include/PersistentGPUAllocator.hpp"
    "include/ProfilingUtilities.hpp"
    "include/RenderSystemInterface.hpp"
    "include/ResourceHandles.hpp"
    "include/RMLRenderer.hpp"
    "include/RuntimeComponentIDs.hpp"
    "include/Scene.hpp"
    "include/SceneLoadingContext.hpp"
    "include/ScriptingRuntime.hpp"
    "include/Serialization.hpp"
    "include/Signals.hpp"
    "include/static_vector.hpp"
    "include/TextureUtilities.hpp"
    "include/ThreadUtilities.hpp"
    "include/TimeUtilities.hpp"
    "include/Transforms.hpp"
    "include/TriMeshResource.hpp"
    "include/TriggerComponent.hpp"
    "include/TriggerSlotIDs.hpp"
    "include/Type.hpp"
    "include/VertexBuffer.hpp"
)

target_include_directories(flex
    PUBLIC
    "include"
    ${FLEX_ROOT}/ThirdParty/
    )


target_link_libraries(flex PUBLIC fmt::fmt-header-only)
target_link_libraries(flex PUBLIC range-v3::meta range-v3::concepts range-v3::range-v3)
#target_link_libraries(flex PRIVATE AngelScriptComponents)
target_link_libraries(flex PUBLIC imgui::imgui)
target_link_libraries(flex PUBLIC imguizmo::imguizmo)
target_link_libraries(flex PUBLIC Angelscript::angelscript)
target_link_libraries(flex PUBLIC implot::implot)
target_link_libraries(flex PRIVATE loguru::loguru)
target_link_libraries(flex PRIVATE scn::scn)
target_link_libraries(flex PUBLIC RmlUi::Core RmlUi::Debugger)

find_path(SIMDE_INCLUDE_DIRS "simde")
target_include_directories(flex PUBLIC ${SIMDE_INCLUDE_DIRS})

if(WIN32)
    #find_package(directx-headers CONFIG REQUIRED)
    #find_package(directx12-agility CONFIG REQUIRED)

    message(${SIMDE_INCLUDE_DIRS})

	ADD_LIBRARY(ZLIB-NG SHARED IMPORTED)
    SET_PROPERTY(TARGET ZLIB-NG PROPERTY IMPORTED_IMPLIB    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/lib/zlib-ng.lib)
    SET_PROPERTY(TARGET ZLIB-NG PROPERTY IMPORTED_LOCATION  ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/zlib-ng2.dll)
    target_link_libraries(flex PUBLIC ZLIB-NG)

    target_compile_definitions(flex PUBLIC WIN32)
    target_compile_definitions(flex PUBLIC WIN32_LEAN_AND_MEAN)

    #target_link_libraries(flex PRIVATE WINPIX)
    #target_link_libraries(flex PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)

    target_compile_definitions(flex PUBLIC UNICODE)
    target_compile_options(flex PUBLIC /UTF-8)
    target_compile_options(flex PUBLIC "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(flex PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(flex PUBLIC /EHsc)

    file(COPY ${D3D12CORE_LOCATION} DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${D3D12LAYERS_LOCATION} DESTINATION ${CMAKE_BINARY_DIR})
endif(WIN32)


if(DEFINED ANDROID_PLATFORM)
    message("ANDROID BUILD!!")
    target_compile_options(flex PUBLIC -std=c++23)
endif()

if(UNIX)
    #ADD_LIBRARY(ZLIB-NG SHARED IMPORTED)
    #SET_PROPERTY(TARGET ZLIB-NG PROPERTY IMPORTED_IMPLIB    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libz-ng.a)
    #target_link_libraries(flex PUBLIC ZLIB-NG)

    find_package(zlib-ng CONFIG REQUIRED)
    target_link_libraries(flex PRIVATE zlib-ng::zlib)
endif(UNIX)

#set_target_properties(flex PROPERTIES UNITY_BUILD ON)

install(TARGETS flex DESTINATION lib)
install(TARGETS flex FILE_SET flex_headers)

function(Flex_LinkAssets TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>
        COMMAND_EXPAND_LISTS)

    if(WIN32)
        file(TO_NATIVE_PATH ${FLEX_ROOT}/assets SOURCEPATH)
        file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/assets TARGETPATH)

    #    add_custom_command(TARGET ${TARGET} POST_BUILD
    #        COMMAND mklink /j ${TARGETPATH} ${SOURCEPATH} || echo ""
    #        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/dxil.dll $<TARGET_FILE_DIR:${TARGET}>
    #        COMMAND_EXPAND_LISTS)
    else()
    #    add_custom_command(
    #        TARGET ${TARGET} POST_BUILD
    #        COMMAND ${CMAKE_COMMAND} -E create_symlink ${FLEX_ROOT}/assets $<TARGET_FILE_DIR:${TARGET}>/assets
    #        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/dxil.dll $<TARGET_FILE_DIR:${TARGET}>
    #        COMMAND_EXPAND_LISTS)
    endif()
endfunction()

function(Flex_CopyAssets TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${FLEX_ROOT}/shaders $<TARGET_FILE_DIR:${TARGET}>/assets/shaders
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/dxil.dll $<TARGET_FILE_DIR:${TARGET}>
        COMMAND_EXPAND_LISTS)
endfunction()

function(Flex_CopyBinaries TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>
        COMMAND_EXPAND_LISTS)

    if(WIN32)
        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/dxil.dll $<TARGET_FILE_DIR:${TARGET}>
            COMMAND_EXPAND_LISTS)
    endif()
endfunction()
