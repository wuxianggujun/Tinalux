add_library(TinaluxRendering STATIC
    FontCache.cpp
    TextBackend.cpp
    rendering.cpp
    Image.cpp
    ImageCache.cpp
    ParagraphPainter.cpp
    RasterSurface.cpp
    RenderContextFactory.cpp
    VulkanBackend.cpp
    RenderSurfaceLifecycle.cpp
)

if(APPLE)
    target_sources(TinaluxRendering PRIVATE MetalBackend.mm)
else()
    target_sources(TinaluxRendering PRIVATE MetalBackend.cpp)
endif()

add_library(Tinalux::Rendering ALIAS TinaluxRendering)

target_include_directories(TinaluxRendering
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${TINALUX_GLFW_SOURCE_DIR}/deps
)

target_link_libraries(TinaluxRendering
    PUBLIC
        Tinalux::Core
        Skia::Skia
)

if(ANDROID)
    target_link_libraries(TinaluxRendering PRIVATE GLESv2)
endif()

if(APPLE)
    find_library(TINALUX_METAL_FRAMEWORK Metal REQUIRED)
    find_library(TINALUX_QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(TinaluxRendering PRIVATE
        ${TINALUX_METAL_FRAMEWORK}
        ${TINALUX_QUARTZCORE_FRAMEWORK}
    )
endif()
