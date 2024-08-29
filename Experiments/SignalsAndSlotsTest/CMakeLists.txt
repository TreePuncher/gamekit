#Signals

project(Signals)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file (GLOB CPP_FILES "*.cpp")

add_executable(
    Signals
    ${CPP_FILES}
)

target_include_directories(
    Signals
    PUBLIC
    flex_optional
    ${PROJECT_SOURCE_DIR})

set_property(TARGET Signals PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_libraries(Signals PRIVATE flex flex_optional)

Flex_CopyAssets(Signals)
