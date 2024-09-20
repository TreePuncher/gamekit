#ProdGen

project(ProcGen)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES "*.cpp")

add_executable(
    ProcGen
    ${CPP_FILES}
)

target_include_directories(
    ProcGen
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET ProcGen PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(ProcGen PRIVATE flex flex_optional)

Flex_LinkAssets(ProcGen)
