if(CMAKE_SYSTEM_NAME STREQUAL "Win32")
    file(DOWNLOAD https://dl.google.com/android/repository/android-ndk-r29-windows.zip adk.zip)
    file(ARCHIVE_EXTRACT adk.zip adk)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(DOWNLOAD https://dl.google.com/android/repository/android-ndk-r29-linux.zip adk.zip)
    file(ARCHIVE_EXTRACT adk.zip adk)
endif()

set(VCPKG_TARGET_ARCHITECTURE armm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic) # This changed from static to dynamic
set(VCPKG_CMAKE_SYSTEM_NAME Android)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "/home/robertm/repos/gamekit/triplets/adk/android-ndk-r29/build/cmake/android.toolchain.cmake)