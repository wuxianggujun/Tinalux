cmake_minimum_required(VERSION 3.31)
project(Tinalux LANGUAGES CXX)

option(TINALUX_BUILD_TESTS "Build tests" ON)

if(WIN32 AND MSVC)
    # 某些 IDE/Ninja 环境下不会自动带上 Windows SDK 的 /LIBPATH，
    # 导致链接阶段找不到 mpr.lib / userenv.lib 等系统库（LNK1104）。
    find_library(_tinalux_mpr_lib NAMES mpr.lib mpr)
    find_library(_tinalux_msvcprtd_lib NAMES msvcprtd.lib msvcprtd)

    if(CMAKE_CXX_COMPILER)
        file(TO_CMAKE_PATH "${CMAKE_CXX_COMPILER}" _tinalux_cxx_compiler_path)
        string(REGEX REPLACE "^(.*)/bin/Host[^/]+/[^/]+/cl\\.exe$" "\\1" _tinalux_msvc_root "${_tinalux_cxx_compiler_path}")
        if(NOT DEFINED ENV{INCLUDE} OR "$ENV{INCLUDE}" STREQUAL "")
            set(_tinalux_msvc_include "${_tinalux_msvc_root}/include")
            if(EXISTS "${_tinalux_msvc_include}")
                add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/I${_tinalux_msvc_include}>")
            endif()
            unset(_tinalux_msvc_include)
        endif()

        if(NOT _tinalux_msvcprtd_lib)
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
        endif()
        unset(_tinalux_cxx_compiler_path)
        unset(_tinalux_msvc_root)
    endif()

    if(CMAKE_RC_COMPILER)
        get_filename_component(_tinalux_rc_dir "${CMAKE_RC_COMPILER}" DIRECTORY)
        get_filename_component(_tinalux_rc_ver_dir "${_tinalux_rc_dir}" DIRECTORY)
        get_filename_component(_tinalux_winsdk_ver "${_tinalux_rc_ver_dir}" NAME)
        get_filename_component(_tinalux_winkits_bin "${_tinalux_rc_ver_dir}" DIRECTORY)
        get_filename_component(_tinalux_winkits_root "${_tinalux_winkits_bin}" DIRECTORY)

        if(NOT _tinalux_mpr_lib)
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
        endif()

        if(NOT DEFINED ENV{INCLUDE} OR "$ENV{INCLUDE}" STREQUAL "")
            set(_tinalux_winsdk_inc_root "${_tinalux_winkits_root}/Include/${_tinalux_winsdk_ver}")
            foreach(_tinalux_winsdk_inc_sub ucrt um shared winrt)
                set(_tinalux_winsdk_inc "${_tinalux_winsdk_inc_root}/${_tinalux_winsdk_inc_sub}")
                if(EXISTS "${_tinalux_winsdk_inc}")
                    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/I${_tinalux_winsdk_inc}>")
                endif()
                unset(_tinalux_winsdk_inc)
            endforeach()
            unset(_tinalux_winsdk_inc_root)
        endif()

        unset(_tinalux_rc_dir)
        unset(_tinalux_rc_ver_dir)
        unset(_tinalux_winsdk_ver)
        unset(_tinalux_winkits_bin)
        unset(_tinalux_winkits_root)
    endif()
    unset(_tinalux_mpr_lib)
    unset(_tinalux_msvcprtd_lib)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # 统一按 UTF-8 解释源文件，避免中文注释触发代码页告警（C4819）。
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --- GLFW (轻量窗口管理) ---
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

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

add_executable(Tinalux main.cpp)
target_link_libraries(Tinalux PRIVATE Tinalux::App)
tinalux_copy_skia_runtime_data(Tinalux)

if(TINALUX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
