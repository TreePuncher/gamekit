set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic) # This changed from static to dynamic
set(VCPKG_CMAKE_SYSTEM_NAME Android)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$env{NDK_ROOT}/build/cmake/android.toolchain.cmake")