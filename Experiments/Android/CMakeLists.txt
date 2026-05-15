#AnimationTest

project(AndroidTest)

find_package(zlib-ng CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    AndroidTest
    MODULE
    application.cpp
)

target_link_libraries(AndroidTest "$<LINK_LIBRARY:WHOLE_ARCHIVE,androidglue>" flex vkBackend)
set_target_properties(AndroidTest PROPERTIES OUTPUT_NAME "main")


