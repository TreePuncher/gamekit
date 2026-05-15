#AnimationTest
project(AndroidIOTest)

find_package(zlib-ng CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    AndroidIOTest
    MODULE
    IOTest.cpp
)

target_link_libraries(AndroidIOTest "$<LINK_LIBRARY:WHOLE_ARCHIVE,androidglue>" flex vkBackend)
set_target_properties(AndroidIOTest PROPERTIES OUTPUT_NAME "main")


