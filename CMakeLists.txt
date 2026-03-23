cmake_minimum_required(VERSION 3.31)
project(Tinalux LANGUAGES C CXX)

option(TINALUX_BUILD_TESTS "Build tests" ON)
option(TINALUX_BUILD_SAMPLES "Build samples" OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

function(tinalux_add_msvc_compile_include include_dir)
    if(NOT EXISTS "${include_dir}")
        return()
    endif()

    foreach(_tinalux_lang C CXX)
        add_compile_options("$<$<COMPILE_LANGUAGE:${_tinalux_lang}>:/I${include_dir}>")
    endforeach()
    unset(_tinalux_lang)
endfunction()

if(WIN32 AND MSVC)
    # 某些 IDE/Ninja 环境下不会自动带上 MSVC/Windows SDK 的 /LIBPATH。
    # 这里直接补齐标准库目录与头文件目录，避免同一 build 目录对当前终端环境敏感。
    if(CMAKE_CXX_COMPILER)
        file(TO_CMAKE_PATH "${CMAKE_CXX_COMPILER}" _tinalux_cxx_compiler_path)
        string(REGEX REPLACE "^(.*)/bin/Host[^/]+/[^/]+/cl\\.exe$" "\\1" _tinalux_msvc_root "${_tinalux_cxx_compiler_path}")
        set(_tinalux_msvc_include "${_tinalux_msvc_root}/include")
        tinalux_add_msvc_compile_include("${_tinalux_msvc_include}")
        unset(_tinalux_msvc_include)

        set(_tinalux_msvc_lib "${_tinalux_msvc_root}/lib/x64")
        set(_tinalux_msvc_lib_onecore "${_tinalux_msvc_root}/lib/onecore/x64")
        if(EXISTS "${_tinalux_msvc_lib}")
            link_directories("${_tinalux_msvc_lib}")
        endif()
        if(EXISTS "${_tinalux_msvc_lib_onecore}")
            link_directories("${_tinalux_msvc_lib_onecore}")
        endif()
        unset(_tinalux_msvc_lib)
        unset(_tinalux_msvc_lib_onecore)
        unset(_tinalux_cxx_compiler_path)
        unset(_tinalux_msvc_root)
    endif()

    if(CMAKE_RC_COMPILER)
        get_filename_component(_tinalux_rc_dir "${CMAKE_RC_COMPILER}" DIRECTORY)
        get_filename_component(_tinalux_rc_ver_dir "${_tinalux_rc_dir}" DIRECTORY)
        get_filename_component(_tinalux_winsdk_ver "${_tinalux_rc_ver_dir}" NAME)
        get_filename_component(_tinalux_winkits_bin "${_tinalux_rc_ver_dir}" DIRECTORY)
        get_filename_component(_tinalux_winkits_root "${_tinalux_winkits_bin}" DIRECTORY)

        set(_tinalux_winsdk_um "${_tinalux_winkits_root}/Lib/${_tinalux_winsdk_ver}/um/x64")
        set(_tinalux_winsdk_ucrt "${_tinalux_winkits_root}/Lib/${_tinalux_winsdk_ver}/ucrt/x64")
        if(EXISTS "${_tinalux_winsdk_um}")
            link_directories("${_tinalux_winsdk_um}")
        endif()
        if(EXISTS "${_tinalux_winsdk_ucrt}")
            link_directories("${_tinalux_winsdk_ucrt}")
        endif()
        unset(_tinalux_winsdk_um)
        unset(_tinalux_winsdk_ucrt)

        set(_tinalux_winsdk_inc_root "${_tinalux_winkits_root}/Include/${_tinalux_winsdk_ver}")
        foreach(_tinalux_winsdk_inc_sub ucrt um shared winrt)
            set(_tinalux_winsdk_inc "${_tinalux_winsdk_inc_root}/${_tinalux_winsdk_inc_sub}")
            tinalux_add_msvc_compile_include("${_tinalux_winsdk_inc}")
            unset(_tinalux_winsdk_inc)
        endforeach()
        unset(_tinalux_winsdk_inc_root)

        unset(_tinalux_rc_dir)
        unset(_tinalux_rc_ver_dir)
        unset(_tinalux_winsdk_ver)
        unset(_tinalux_winkits_bin)
        unset(_tinalux_winkits_root)
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # 统一按 UTF-8 解释源文件，避免中文注释触发代码页告警（C4819）。
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(TinaluxMarkupTools)

# --- GLFW (轻量窗口管理) ---
if(NOT ANDROID)
    set(TINALUX_GLFW_SOURCE_DIR "${PROJECT_SOURCE_DIR}/3rdparty/tina_glfw")
    if(NOT EXISTS "${TINALUX_GLFW_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing tina_glfw checkout at ${TINALUX_GLFW_SOURCE_DIR}. "
            "Clone https://github.com/wuxianggujun/tina_glfw.git first.")
    endif()

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${TINALUX_GLFW_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/tina_glfw-build")
endif()

# --- spdlog (日志后端) ---
set(TINALUX_SPDLOG_SOURCE_DIR "${PROJECT_SOURCE_DIR}/3rdparty/spdlog")
if(NOT EXISTS "${TINALUX_SPDLOG_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Missing spdlog checkout at ${TINALUX_SPDLOG_SOURCE_DIR}. "
        "Clone https://github.com/gabime/spdlog.git first.")
endif()

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${TINALUX_SPDLOG_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/spdlog-build")

# --- Skia ---
include(SkiaConfig)
tinalux_enable_skia()

function(tinalux_copy_skia_runtime_data target_name)
    add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${SKIA_BUILD_DIR}/Debug/icudtl.dat,${SKIA_BUILD_DIR}/Release/icudtl.dat>"
            "$<TARGET_FILE_DIR:${target_name}>/icudtl.dat"
        COMMENT "Copying Skia ICU data for ${target_name}"
        VERBATIM
    )
endfunction()

add_subdirectory(src)
if(TINALUX_BUILD_SAMPLES AND NOT ANDROID)
    add_subdirectory(samples)
elseif(TINALUX_BUILD_SAMPLES AND ANDROID)
    message(STATUS "Skipping desktop samples for Android builds")
endif()

if(ANDROID)
    add_library(TinaluxAndroidNative SHARED
        src/app/android/AndroidJniBridge.cpp
    )
    target_link_libraries(TinaluxAndroidNative PRIVATE
        Tinalux::App
        android
    )
    target_compile_definitions(TinaluxAndroidNative PRIVATE TINALUX_ANDROID_NATIVE_EXPORTS)
    set_target_properties(TinaluxAndroidNative PROPERTIES OUTPUT_NAME "tinalux_native")
else()
    add_executable(Tinalux main.cpp)
    target_link_libraries(Tinalux PRIVATE Tinalux::App)
    tinalux_copy_skia_runtime_data(Tinalux)
endif()

if(TINALUX_BUILD_TESTS AND NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
elseif(TINALUX_BUILD_TESTS AND ANDROID)
    message(STATUS "Skipping desktop smoke tests for Android builds")
endif()
