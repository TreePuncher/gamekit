cmake_minimum_required(VERSION 3.23)

project(FlexEngine  LANGUAGES CXX VERSION 0.1.1)
find_package(Directx-dxc REQUIRED)
find_package(fmt REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ShaderRC src/shaderRC.cpp)
target_link_libraries(ShaderRC PRIVATE flex Microsoft::DXIL fmt::fmt-header-only shaderpreprocessor)
