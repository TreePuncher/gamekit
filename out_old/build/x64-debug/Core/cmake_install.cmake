# Install script for directory: E:/repos/gamekit/Core

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "E:/repos/gamekit/out/install/x64-debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/repos/gamekit/out/build/x64-debug/_deps/scn-build/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/repos/gamekit/out/build/x64-debug/Core/flex.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("E:/repos/gamekit/out/build/x64-debug/Core/CMakeFiles/flex.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/repos/gamekit/out/build/x64-debug/Core/flex.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/include" TYPE FILE FILES
    "E:/repos/gamekit/Core/include/AnimationComponents.hpp"
    "E:/repos/gamekit/Core/include/AnimationRuntimeUtilities.hpp"
    "E:/repos/gamekit/Core/include/AnimationUtilities.hpp"
    "E:/repos/gamekit/Core/include/AnimationRendering.hpp"
    "E:/repos/gamekit/Core/include/Application.hpp"
    "E:/repos/gamekit/Core/include/Assets.hpp"
    "E:/repos/gamekit/Core/include/BuildSettings.hpp"
    "E:/repos/gamekit/Core/include/DoOnce.hpp"
    "E:/repos/gamekit/Core/include/CameraComponent.hpp"
    "E:/repos/gamekit/Core/include/CommonStructs.hpp"
    "E:/repos/gamekit/Core/include/CameraComponent.hpp"
    "E:/repos/gamekit/Core/include/ComponentBlobs.hpp"
    "E:/repos/gamekit/Core/include/Components.hpp"
    "E:/repos/gamekit/Core/include/Console.hpp"
    "E:/repos/gamekit/Core/include/Containers.hpp"
    "E:/repos/gamekit/Core/include/Brush.hpp"
    "E:/repos/gamekit/Core/include/DebugPanel.hpp"
    "E:/repos/gamekit/Core/include/DebugUI.hpp"
    "E:/repos/gamekit/Core/include/DefaultPipelineStates.hpp"
    "E:/repos/gamekit/Core/include/DepthBuffer.hpp"
    "E:/repos/gamekit/Core/include/DungeonGen.hpp"
    "E:/repos/gamekit/Core/include/EngineCore.hpp"
    "E:/repos/gamekit/Core/include/Events.hpp"
    "E:/repos/gamekit/Core/include/Fonts.hpp"
    "E:/repos/gamekit/Core/include/FrameGraph.hpp"
    "E:/repos/gamekit/Core/include/GameFramework.hpp"
    "E:/repos/gamekit/Core/include/GPUAllocators.hpp"
    "E:/repos/gamekit/Core/include/Geometry.hpp"
    "E:/repos/gamekit/Core/include/Handle.hpp"
    "E:/repos/gamekit/Core/include/Input.hpp"
    "E:/repos/gamekit/Core/include/Intersection.hpp"
    "E:/repos/gamekit/Core/include/KeycodesEnums.hpp"
    "E:/repos/gamekit/Core/include/KeyValueIDs.hpp"
    "E:/repos/gamekit/Core/include/Level.hpp"
    "E:/repos/gamekit/Core/include/Logging.hpp"
    "E:/repos/gamekit/Core/include/Materials.hpp"
    "E:/repos/gamekit/Core/include/MathUtilities.hpp"
    "E:/repos/gamekit/Core/include/MemoryUtilities.hpp"
    "E:/repos/gamekit/Core/include/MeshUtilities.hpp"
    "E:/repos/gamekit/Core/include/ModifiableShape.hpp"
    "E:/repos/gamekit/Core/include/NetworkUtilities.hpp"
    "E:/repos/gamekit/Core/include/Particles.hpp"
    "E:/repos/gamekit/Core/include/ProfilingUtilities.hpp"
    "E:/repos/gamekit/Core/include/RenderSystemInterface.hpp"
    "E:/repos/gamekit/Core/include/ResourceHandles.hpp"
    "E:/repos/gamekit/Core/include/RMLRenderer.hpp"
    "E:/repos/gamekit/Core/include/RuntimeComponentIDs.hpp"
    "E:/repos/gamekit/Core/include/Scene.hpp"
    "E:/repos/gamekit/Core/include/SceneLoadingContext.hpp"
    "E:/repos/gamekit/Core/include/ScriptingRuntime.hpp"
    "E:/repos/gamekit/Core/include/Serialization.hpp"
    "E:/repos/gamekit/Core/include/Signals.hpp"
    "E:/repos/gamekit/Core/include/static_vector.hpp"
    "E:/repos/gamekit/Core/include/TextureUtilities.hpp"
    "E:/repos/gamekit/Core/include/ThreadUtilities.hpp"
    "E:/repos/gamekit/Core/include/TimeUtilities.hpp"
    "E:/repos/gamekit/Core/include/Transforms.hpp"
    "E:/repos/gamekit/Core/include/TriMeshResource.hpp"
    "E:/repos/gamekit/Core/include/TriggerComponent.hpp"
    "E:/repos/gamekit/Core/include/TriggerSlotIDs.hpp"
    "E:/repos/gamekit/Core/include/Type.hpp"
    "E:/repos/gamekit/Core/include/VertexBuffer.hpp"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/repos/gamekit/out/build/x64-debug/Core/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
