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

target_compile_definitions(TinaluxRendering
    PRIVATE
        SK_USE_INTERNAL_VULKAN_HEADERS
)

function(tinalux_detect_windows_sdk_um_dir out_var)
    if(NOT WIN32)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_arch x64)
    else()
        set(_arch x86)
    endif()

    set(_candidates)
    if(DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION
        AND NOT CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION STREQUAL "")
        list(APPEND _candidates
            "C:/Program Files (x86)/Windows Kits/10/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/${_arch}"
        )
    endif()

    file(GLOB _winsdk_lib_dirs "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.*")
    list(SORT _winsdk_lib_dirs COMPARE NATURAL ORDER DESCENDING)
    foreach(_winsdk_dir IN LISTS _winsdk_lib_dirs)
        list(APPEND _candidates "${_winsdk_dir}/um/${_arch}")
    endforeach()

    foreach(_candidate IN LISTS _candidates)
        if(EXISTS "${_candidate}/OpenGL32.Lib" OR EXISTS "${_candidate}/opengl32.lib")
            set(${out_var} "${_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "Failed to locate Windows SDK OpenGL import library directory for ${_arch}")
endfunction()

if(WIN32)
    tinalux_detect_windows_sdk_um_dir(TINALUX_WINSDK_UM_LIB_DIR)
    target_link_directories(TinaluxRendering PUBLIC ${TINALUX_WINSDK_UM_LIB_DIR})
endif()

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
