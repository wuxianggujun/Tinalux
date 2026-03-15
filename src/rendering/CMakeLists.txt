add_library(TinaluxRendering STATIC
    rendering.cpp
    Image.cpp
    ParagraphPainter.cpp
    RasterSurface.cpp
    GLContext.cpp
    WindowSurface.cpp
)

add_library(Tinalux::Rendering ALIAS TinaluxRendering)

target_include_directories(TinaluxRendering
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(TinaluxRendering
    PUBLIC
        Tinalux::Core
        Skia::Skia
)
