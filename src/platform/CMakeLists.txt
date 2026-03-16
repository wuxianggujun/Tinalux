add_library(TinaluxPlatform STATIC
    Window.cpp
)

add_library(Tinalux::Platform ALIAS TinaluxPlatform)

target_include_directories(TinaluxPlatform
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${TINALUX_GLFW_SOURCE_DIR}/deps
)

target_link_libraries(TinaluxPlatform
    PUBLIC
        Tinalux::Core
)

if(NOT ANDROID)
    target_sources(TinaluxPlatform PRIVATE
        glfw/GLFWWindow.cpp
    )
    target_link_libraries(TinaluxPlatform PRIVATE glfw)
    if(APPLE)
        target_sources(TinaluxPlatform PRIVATE
            glfw/CocoaTextInputBridge.mm
            glfw/CocoaMetalLayerBridge.mm
        )
        find_library(TINALUX_COCOA_FRAMEWORK Cocoa REQUIRED)
        find_library(TINALUX_METAL_FRAMEWORK Metal REQUIRED)
        find_library(TINALUX_QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
        target_link_libraries(TinaluxPlatform PRIVATE
            ${TINALUX_COCOA_FRAMEWORK}
            ${TINALUX_METAL_FRAMEWORK}
            ${TINALUX_QUARTZCORE_FRAMEWORK}
        )
    endif()
    if(WIN32)
        target_link_libraries(TinaluxPlatform PRIVATE imm32)
    endif()
    target_compile_definitions(TinaluxPlatform PRIVATE TINALUX_PLATFORM_GLFW)
endif()
