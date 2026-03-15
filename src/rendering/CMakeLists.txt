add_library(TinaluxRendering STATIC
    FontCache.cpp
    TextBackend.cpp
    rendering.cpp
    Image.cpp
    ImageCache.cpp
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
