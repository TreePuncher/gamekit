set(CMAKE_MAKE_PROGRAM "/usr/bin/ninja")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_SYSTEM_PROCESSOR "x86_64")
set(VCPKG_TARGET_ARCHITECTURE "x64")
add_compile_options(-Wno-error)
add_compile_definitions(BOOST_REGEX_NO_LIB)

