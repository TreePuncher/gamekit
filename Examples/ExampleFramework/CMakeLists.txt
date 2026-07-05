#sampleframework

project(exampleframework)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    exampleframework
    "src/ExampleFramework.cpp"
)

target_include_directories(flex
    PUBLIC
    "include"
   include
    )

target_link_libraries(exampleframework PRIVATE flex dxBackend)

