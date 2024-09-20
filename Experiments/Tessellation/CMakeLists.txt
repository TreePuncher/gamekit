#Tessellation

project(Tessellation)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES "*.cpp")

add_executable(
    Tessellation
    ${CPP_FILES}
)

target_include_directories(
    Tessellation
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET Tessellation PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(Tessellation PRIVATE flex flex_optional)

Flex_LinkAssets(Tessellation)
