cmake_minimum_required(VERSION 3.25)

add_subdirectory(BackendShared)
if(WIN32)
add_subdirectory(DX12)
endif(WIN32)
add_subdirectory(vk)
add_subdirectory(PhysX)
add_subdirectory(HighQualityRendering)

