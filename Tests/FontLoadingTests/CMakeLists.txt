find_package(scn CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)

add_executable (FontLoading "main.cpp")
target_link_libraries(FontLoading PRIVATE scn::scn)
target_link_libraries(FontLoading PRIVATE Freetype::Freetype) 
target_link_libraries(FontLoading PRIVATE SFML::Graphics SFML::Window SFML::System)
target_link_libraries(FontLoading PRIVATE flex)

set_property(TARGET FontLoading PROPERTY CXX_STANDARD 23)

