add_library(TinaluxPlatform STATIC
    Window.cpp
)

add_library(Tinalux::Platform ALIAS TinaluxPlatform)

target_include_directories(TinaluxPlatform
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
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
    target_compile_definitions(TinaluxPlatform PRIVATE TINALUX_PLATFORM_GLFW)
endif()

