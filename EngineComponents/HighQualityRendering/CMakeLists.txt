cmake_minimum_required(VERSION 3.31)

add_subdirectory(CBT)
add_subdirectory(GI)
add_subdirectory(TextureStreaming)
add_subdirectory(Transparency)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(HighQualityRenderingComponent)

target_sources(HighQualityRenderingComponent
    PRIVATE
    "src/AnimationClusteredRendering.cpp"
    "src/ClusteredRendering.cpp"
    "src/OcclusionCulling.cpp"
    "src/ParticleRendering.cpp"
    "src/PBRConstants.cpp"
    "src/ShadowMapping.cpp"
    "src/WorldRender.cpp"
	
)

target_sources(
    HighQualityRenderingComponent
    PUBLIC
    FILE_SET highqualityrendering_headers
    TYPE HEADERS
    FILES
    "includes/AnimationClusteredRendering.hpp"
    "includes/ClusteredRendering.hpp"
    "includes/OcclusionCulling.hpp"
    "includes/PBRConstants.hpp"
    "includes/ParticleRendering.hpp"
    "includes/ShadowMapping.hpp"
    "includes/WorldRender.hpp"
)

target_link_libraries(HighQualityRenderingComponent PRIVATE flex)

target_include_directories(HighQualityRenderingComponent PUBLIC includes)

install(TARGETS HighQualityRenderingComponent FILE_SET highqualityrendering_headers)
install(TARGETS HighQualityRenderingComponent DESTINATION lib)
