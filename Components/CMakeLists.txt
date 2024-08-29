#Components
project(Components)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPTIONAL_HEADERS "GameplayComponents.hpp")

add_library(flex_optional
    GameplayComponents.cpp
)

target_sources(
    flex_optional PUBLIC
    FILE_SET flex_optional_headers
    TYPE HEADERS
    FILES ${PROJECT_SOURCE_DIR}/GameplayComponents.hpp)
target_link_libraries(flex_optional PRIVATE flex)

install(TARGETS flex_optional DESTINATION lib)
install(TARGETS flex_optional FILE_SET flex_optional_headers)
