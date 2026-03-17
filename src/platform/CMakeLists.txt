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

function(tinalux_find_windows_sdk_library out_var library_name)
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
            "C:/Program Files (x86)/Windows Kits/10/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/${_arch}/${library_name}.lib"
            "C:/Program Files (x86)/Windows Kits/10/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/${_arch}/${library_name}.Lib"
        )
    endif()

    file(GLOB _winsdk_lib_dirs "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.*")
    list(SORT _winsdk_lib_dirs COMPARE NATURAL ORDER DESCENDING)
    foreach(_winsdk_dir IN LISTS _winsdk_lib_dirs)
        list(APPEND _candidates
            "${_winsdk_dir}/um/${_arch}/${library_name}.lib"
            "${_winsdk_dir}/um/${_arch}/${library_name}.Lib"
        )
    endforeach()

    foreach(_candidate IN LISTS _candidates)
        if(EXISTS "${_candidate}")
            set(${out_var} "${_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "Failed to locate Windows SDK library ${library_name} for ${_arch}")
endfunction()

if(ANDROID)
    target_sources(TinaluxPlatform PRIVATE
        android/AndroidWindow.cpp
    )
    target_link_libraries(TinaluxPlatform PRIVATE
        android
        ${CMAKE_DL_LIBS}
        EGL
        vulkan
    )
    target_compile_definitions(TinaluxPlatform PRIVATE TINALUX_PLATFORM_ANDROID)
else()
    target_sources(TinaluxPlatform PRIVATE
        glfw/GLFWWindow.cpp
    )
    target_include_directories(TinaluxPlatform PRIVATE
        ${TINALUX_GLFW_SOURCE_DIR}/deps
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
        tinalux_find_windows_sdk_library(TINALUX_IMM32_LIBRARY Imm32)
        target_link_libraries(TinaluxPlatform PRIVATE ${TINALUX_IMM32_LIBRARY})
    endif()
    target_compile_definitions(TinaluxPlatform PRIVATE TINALUX_PLATFORM_GLFW)
endif()
