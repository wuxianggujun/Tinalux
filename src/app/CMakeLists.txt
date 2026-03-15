add_library(TinaluxApp STATIC
    Application.cpp
    UIContext.cpp
)

add_library(Tinalux::App ALIAS TinaluxApp)

target_include_directories(TinaluxApp
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(TinaluxApp
    PUBLIC
        Tinalux::Platform
        Tinalux::UI
        Tinalux::Rendering
        Tinalux::Core
)
