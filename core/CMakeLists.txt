#src

project(flex)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Angelscript CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(loguru CONFIG REQUIRED)
find_package(scn CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(PkgConfig)

add_library(flex
    "src/AnimationComponents.cpp"
    "src/AnimationRendering.cpp"
    "src/AnimationRuntimeUtilities.cpp"
    "src/AnimationUtilities.cpp"
    "src/Application.cpp"
    "src/Assets.cpp"
    "src/CameraUtilities.cpp"
    "src/CBT.cpp"
    "src/CBTTerrain.cpp"
    "src/ClusteredRendering.cpp"
    "src/Components.cpp"
    "src/Console.cpp"
    "src/CoreSceneObjects.cpp"
    "src/DDSUtilities.cpp"
    "src/DebugPanel.cpp"
    "src/DebugUI.cpp"
    "src/defaultpipelinestates.cpp"
    "src/DungeonGen.cpp"
    "src/EngineCore.cpp"
    "src/Framegraph.cpp"
    "src/GameFramework.cpp"
    "src/Geometry.cpp"
    "src/GILightingUtilities.cpp"
    "src/GPUAllocators.cpp"
    "src/Graphics.cpp"
    "src/GraphicsComponents.cpp"
    "src/GUIInputState.cpp"
    "src/GuiUtilities.cpp"
    "src/Intersection.cpp"
    "src/Level.cpp"
    "src/Logging.cpp"
    "src/MathUtilities.cpp"
    "src/Materials.cpp"
    "src/MemoryUtilities.cpp"
    "src/Meshutilities.cpp"
    "src/ModifiableShape.cpp"
    "src/NetworkUtilities.cpp"
    "src/OcclusionCulling.cpp"
    "src/ParticleRendering.cpp"
    "src/Particles.cpp"
    "src/PhysicsDebugVis.cpp"
    "src/physicsutilities.cpp"
    "src/PipelineState.cpp"
    "src/PlanetComponent.cpp"
    "src/ProfilingUtilities.cpp"
    "src/RayTracingUtilities.cpp"
    "src/RMLRenderer.cpp"
    "src/Scene.cpp"
    "src/ScriptingRuntime.cpp"
    "src/Serialization.cpp"
    "src/SHADOWMAPPING.CPP"
    "src/Signals.cpp"
    "src/SVOGI.cpp"
    "src/TerrainRendering.cpp"
    "src/TextRendering.cpp"
    "src/TextureStreamingUtilities.cpp"
    "src/TextureUtilities.cpp"
    "src/ThreadUtilities.cpp"
    "src/ThirdPartyTranslationUnit.cpp"
    "src/timeutilities.cpp"
    "src/Transforms.cpp"
    "src/Transparency.cpp"
    "src/TriggerComponent.cpp"
    "src/WorldRender.cpp"
 )

target_sources(
    flex PUBLIC
    FILE_SET flex_headers
    TYPE HEADERS
    FILES
    "include/AnimationComponents.hpp"
    "include/AnimationRendering.hpp"
    "include/AnimationRuntimeUtilities.hpp"
    "include/AnimationUtilities.hpp"
    "include/Application.hpp"
    "include/Assets.hpp"
    "include/BuildSettings.hpp"
    "include/DoOnce.hpp"
    "include/CameraUtilities.hpp"
    "include/CBT.hpp"
    "include/CBTTerrain.hpp"
    "include/ClusteredRendering.hpp"
    "include/CommonStructs.hpp"
    "include/ComponentBlobs.hpp"
    "include/Components.hpp"
    "include/Console.hpp"
    "include/Containers.hpp"
    "include/CoreSceneObjects.hpp"
    "include/DDSUtilities.hpp"
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
    "include/GILightingUtilities.hpp"
    "include/Graphics.hpp"
    "include/GraphicsComponents.hpp"
    "include/GUIInputState.hpp"
    "include/GUIUtilities.hpp"
    "include/Handle.hpp"
    "include/Input.hpp"
    "include/Intersection.hpp"
    "include/KeyCodesEnums.hpp"
    "include/KeyValueIDs.hpp"
    "include/Level.hpp"
    "include/Logging.hpp"
    "include/Materials.hpp"
    "include/MathUtilities.hpp"
    "include/MemoryUtilities.hpp"
    "include/MeshUtilities.hpp"
    "include/ModifiableShape.hpp"
    "include/NetworkUtilities.hpp"
    "include/OcclusionCulling.hpp"
    "include/ParticleRendering.hpp"
    "include/Particles.hpp"
    "include/PhysicsDebugVis.hpp"
    "include/PhysicsUtilities.hpp"
    "include/PipelineState.hpp"
    "include/PlanetComponent.hpp"
    "include/ProfilingUtilities.hpp"
    "include/RayTracingUtilities.hpp"
    "include/RenderSystemInterface.hpp"
    "include/ResourceHandles.hpp"
    "include/RMLRenderer.hpp"
    "include/RuntimeComponentIDs.hpp"
    "include/Scene.hpp"
    "include/SceneLoadingContext.hpp"
    "include/ScriptingRuntime.hpp"
    "include/Serialization.hpp"
    "include/ShadowMapping.hpp"
    "include/Signals.hpp"
    "include/static_vector.hpp"
    "include/SVOGI.hpp"
    "include/TerrainRendering.hpp"
    "include/TextRendering.hpp"
    "include/TextureStreamingUtilities.hpp"
    "include/TextureUtilities.hpp"
    "include/ThreadUtilities.hpp"
    "include/TimeUtilities.hpp"
    "include/Transforms.hpp"
    "include/Transparency.hpp"
    "include/TriggerComponent.hpp"
    "include/TriggerSlotIDs.hpp"
    "include/Type.hpp"
    "include/VertexBuffer.hpp"
    "include/Win32Graphics.hpp"
    "include/WorldRender.hpp"
    "include/XMMathConversion.hpp"
)

target_include_directories(flex
    PUBLIC
    "include"
    ${FLEX_ROOT}/ThirdParty/
    )

target_link_libraries(flex PRIVATE fmt::fmt)
target_link_libraries(flex PRIVATE range-v3::meta range-v3::concepts range-v3::range-v3)

if(WIN32)
    find_path(SIMDE_INCLUDE_DIRS "simde")

    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)

    message(${SIMDE_INCLUDE_DIRS})

    ADD_LIBRARY(ZLIB-NG SHARED IMPORTED)
    SET_PROPERTY(TARGET ZLIB-NG PROPERTY IMPORTED_IMPLIB    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/lib/zlib-ng.lib)
    SET_PROPERTY(TARGET ZLIB-NG PROPERTY IMPORTED_LOCATION  ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/zlib-ng2.dll)

    set(WINPIX_LOCATION "${FLEX_ROOT}/ThirdParty/pix/bin/x64/")
    ADD_LIBRARY(WINPIX SHARED IMPORTED)
    SET_PROPERTY(TARGET WINPIX PROPERTY IMPORTED_IMPLIB     ${WINPIX_LOCATION}/WinPixEventRuntime.lib)
    SET_PROPERTY(TARGET WINPIX PROPERTY IMPORTED_LOCATION   ${WINPIX_LOCATION}/WinPixEventRuntime.dll)

    target_compile_definitions(flex PUBLIC WIN32_LEAN_AND_MEAN)
    target_include_directories(flex PUBLIC ${SIMDE_INCLUDE_DIRS})
    target_include_directories(flex PUBLIC ${FLEX_ROOT}/ThirdParty/pix/Include/WinPixEventRuntime)

    target_link_libraries(flex PRIVATE unofficial::omniverse-physx-sdk::sdk)
    target_link_libraries(flex PRIVATE imgui::imgui)
    target_link_libraries(flex PRIVATE imguizmo::imguizmo)
    target_link_libraries(flex PRIVATE Angelscript::angelscript)
    target_link_libraries(flex PRIVATE implot::implot)
    target_link_libraries(flex PRIVATE loguru::loguru)
    target_link_libraries(flex PRIVATE scn::scn)
    target_link_libraries(flex PRIVATE ZLIB-NG)
    target_link_libraries(flex PRIVATE WINPIX) 
    target_link_libraries(flex PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)
    target_link_libraries(flex PRIVATE RmlUi::Core RmlUi::Debugger)

    target_compile_definitions(flex PUBLIC UNICODE)
    target_compile_options(flex PUBLIC /UTF-8)
    target_compile_options(flex PUBLIC "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(flex PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

    add_custom_command(TARGET flex POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${FLEX_ROOT}/ThirdParty/Pix/bin/x64/WinPixEventRuntime.dll ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND_EXPAND_LISTS)

    file(COPY ${D3D12CORE_LOCATION} DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${D3D12LAYERS_LOCATION} DESTINATION ${CMAKE_BINARY_DIR})
elseif(NOT DEFINED ANDROID_ABI)
    find_package(glfw3 CONFIG REQUIRED)

    target_link_libraries(flex PRIVATE imgui::imgui)
    target_link_libraries(flex PRIVATE imguizmo::imguizmo)
    target_link_libraries(flex PRIVATE Angelscript::angelscript)
    target_link_libraries(flex PRIVATE implot::implot)
    target_link_libraries(flex PRIVATE scn::scn)
    target_link_libraries(flex PRIVATE rmlui RmlDebugger)
    target_link_libraries(flex PRIVATE ZLIB-NG)

    target_sources(flex
        PRIVATE
        PlatformLayer_GLFW.cpp)

    target_link_libraries(flex PRIVATE glfw)
endif()

set_target_properties(flex PROPERTIES UNITY_BUILD ON)

install(TARGETS flex DESTINATION lib)
install(TARGETS flex FILE_SET flex_headers)

function(Flex_LinkAssets TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>
        COMMAND_EXPAND_LISTS)

    if(WIN32)
        file(TO_NATIVE_PATH ${FLEX_ROOT}/assets SOURCEPATH)
        file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/assets TARGETPATH)

        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND mklink /j ${TARGETPATH} ${SOURCEPATH} || echo ""
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/dxil.dll $<TARGET_FILE_DIR:${TARGET}>
            COMMAND_EXPAND_LISTS)
    else()
        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${FLEX_ROOT}/assets $<TARGET_FILE_DIR:${TARGET}>/assets
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/dxil.dll $<TARGET_FILE_DIR:${TARGET}>
            COMMAND_EXPAND_LISTS)
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
