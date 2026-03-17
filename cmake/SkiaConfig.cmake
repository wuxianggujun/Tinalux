include_guard(GLOBAL)

set(SKIA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/skia" CACHE PATH "Path to Skia source checkout (with BUILD.gn)")
set(SKIA_BUILD_DIR "${CMAKE_BINARY_DIR}/skia" CACHE PATH "Directory used for Skia build output")
set(_tinalux_default_skia_build_type "Release")
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config generator（VS 等）：内部会按 Debug/Release 自动选择对应的 Skia 产物。
elseif(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
    # Single-config generator（Ninja 等）：默认与 CMAKE_BUILD_TYPE 对齐，避免 Debug/Release 混链。
    set(_tinalux_default_skia_build_type "${CMAKE_BUILD_TYPE}")
endif()
set(SKIA_BUILD_TYPE "${_tinalux_default_skia_build_type}" CACHE STRING "Skia build configuration (Debug/Release)")
unset(_tinalux_default_skia_build_type)
option(SKIA_ENABLE_GPU "Enable Skia GPU backend" ON)
option(SKIA_USE_DAWN "Enable Dawn backend (if available)" OFF)
option(SKIA_BUILD_TOOLS "Build Skia tools" OFF)
option(SKIA_BUILD_TESTS "Build Skia tests" OFF)
option(TINALUX_BUILD_SKIA "Build Skia from source during build (requires gn+ninja)" OFF)
option(TINALUX_AUTO_BUILD_SKIA "Auto-build Skia when prebuilt library is missing" ON)
option(TINALUX_SYNC_SKIA_BUILD_TYPE "Sync SKIA_BUILD_TYPE with CMAKE_BUILD_TYPE for single-config generators" ON)
option(TINALUX_USE_SYSTEM_DEPS "Use system third-party deps (zlib/libpng/libjpeg-turbo/libwebp/expat) when building Skia" OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(NINJA_EXECUTABLE ninja)
if(NOT NINJA_EXECUTABLE AND CMAKE_MAKE_PROGRAM)
    get_filename_component(_make_prog_name "${CMAKE_MAKE_PROGRAM}" NAME)
    if(_make_prog_name MATCHES "^ninja(\\.exe)?$")
        set(NINJA_EXECUTABLE "${CMAKE_MAKE_PROGRAM}")
    endif()
    unset(_make_prog_name)
endif()
if(WIN32 AND NOT NINJA_EXECUTABLE)
    # Visual Studio 的 CMake 工具链通常自带 Ninja，但不一定在 PATH 里。
    find_program(_tinalux_vswhere vswhere
        PATHS
            "C:/Program Files (x86)/Microsoft Visual Studio/Installer"
            "C:/Program Files/Microsoft Visual Studio/Installer"
        NO_DEFAULT_PATH
    )
    if(NOT _tinalux_vswhere)
        find_program(_tinalux_vswhere vswhere)
    endif()
    if(_tinalux_vswhere)
        execute_process(
            COMMAND "${_tinalux_vswhere}" -latest -products * -property installationPath
            OUTPUT_VARIABLE _tinalux_vs_install
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(_tinalux_vs_install)
            set(_tinalux_vs_ninja "${_tinalux_vs_install}/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
            if(EXISTS "${_tinalux_vs_ninja}")
                set(NINJA_EXECUTABLE "${_tinalux_vs_ninja}")
            endif()
            unset(_tinalux_vs_ninja)
        endif()
        unset(_tinalux_vs_install)
    endif()
    unset(_tinalux_vswhere)
endif()

function(_tinalux_find_gn out_var)
    unset(_gn CACHE)
    find_program(_gn gn
        PATHS
            "${SKIA_SOURCE_DIR}/bin"
            "${SKIA_SOURCE_DIR}/buildtools/win"
            "${SKIA_SOURCE_DIR}/buildtools/mac"
            "${SKIA_SOURCE_DIR}/buildtools/linux64"
        NO_DEFAULT_PATH
    )
    if(NOT _gn)
        find_program(_gn gn)
    endif()
    set(${out_var} "${_gn}" PARENT_SCOPE)
endfunction()

function(_tinalux_skia_library_path out_var build_dir)
    if(WIN32)
        set(${out_var} "${build_dir}/skia.lib" PARENT_SCOPE)
    else()
        set(${out_var} "${build_dir}/libskia.a" PARENT_SCOPE)
    endif()
endfunction()

function(_tinalux_skia_runtime_library_paths out_var build_dir)
    _tinalux_skia_library_path(_skia_lib "${build_dir}")
    if(WIN32)
        set(_libs
            "${_skia_lib}"
            "${build_dir}/skparagraph.lib"
            "${build_dir}/skshaper.lib"
            "${build_dir}/skunicode_core.lib"
            "${build_dir}/skunicode_icu.lib"
        )
    else()
        set(_libs
            "${_skia_lib}"
            "${build_dir}/libskparagraph.a"
            "${build_dir}/libskshaper.a"
            "${build_dir}/libskunicode_core.a"
            "${build_dir}/libskunicode_icu.a"
        )
    endif()

    set(${out_var} "${_libs}" PARENT_SCOPE)
endfunction()

function(_tinalux_skia_expected_signature_path out_var build_type)
    set(${out_var} "${CMAKE_BINARY_DIR}/CMakeFiles/tinalux_skia_${build_type}_gn_signature.txt" PARENT_SCOPE)
endfunction()

function(_tinalux_write_skia_expected_signature out_var build_type signature_content)
    _tinalux_skia_expected_signature_path(_signature_path "${build_type}")
    file(WRITE "${_signature_path}" "${signature_content}\n")
    set(${out_var} "${_signature_path}" PARENT_SCOPE)
endfunction()

function(_tinalux_skia_signature_matches out_var build_dir expected_signature_path)
    set(_actual_signature_path "${build_dir}/tinalux_gn_signature.txt")
    if(NOT EXISTS "${_actual_signature_path}" OR NOT EXISTS "${expected_signature_path}")
        set(${out_var} FALSE PARENT_SCOPE)
        return()
    endif()

    file(READ "${_actual_signature_path}" _actual_signature)
    file(READ "${expected_signature_path}" _expected_signature)
    if(NOT _actual_signature STREQUAL _expected_signature)
        set(${out_var} FALSE PARENT_SCOPE)
        return()
    endif()

    set(${out_var} TRUE PARENT_SCOPE)
endfunction()

function(_tinalux_skia_outputs_exist out_var build_dir expected_signature_path)
    _tinalux_skia_runtime_library_paths(_required_libs "${build_dir}")
    foreach(_lib IN LISTS _required_libs)
        if(NOT EXISTS "${_lib}")
            set(${out_var} FALSE PARENT_SCOPE)
            return()
        endif()
    endforeach()

    _tinalux_skia_signature_matches(_signature_matches "${build_dir}" "${expected_signature_path}")
    if(NOT _signature_matches)
        set(${out_var} FALSE PARENT_SCOPE)
        return()
    endif()

    set(${out_var} TRUE PARENT_SCOPE)
endfunction()

function(_tinalux_resolve_android_ndk out_var)
    foreach(_candidate IN ITEMS
        "${CMAKE_ANDROID_NDK}"
        "${ANDROID_NDK}"
        "$ENV{ANDROID_NDK_HOME}"
        "$ENV{ANDROID_NDK_ROOT}"
    )
        if(NOT _candidate STREQUAL "" AND EXISTS "${_candidate}")
            file(TO_CMAKE_PATH "${_candidate}" _resolved_candidate)
            set(${out_var} "${_resolved_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_tinalux_android_abi_to_gn_cpu out_var)
    if(CMAKE_ANDROID_ARCH_ABI)
        set(_android_abi "${CMAKE_ANDROID_ARCH_ABI}")
    elseif(DEFINED ANDROID_ABI)
        set(_android_abi "${ANDROID_ABI}")
    else()
        set(_android_abi "")
    endif()

    if(_android_abi STREQUAL "armeabi-v7a")
        set(_gn_cpu "arm")
    elseif(_android_abi STREQUAL "arm64-v8a")
        set(_gn_cpu "arm64")
    elseif(_android_abi STREQUAL "x86")
        set(_gn_cpu "x86")
    elseif(_android_abi STREQUAL "x86_64")
        set(_gn_cpu "x64")
    else()
        message(FATAL_ERROR "Unsupported Android ABI for Skia GN build: '${_android_abi}'")
    endif()

    set(${out_var} "${_gn_cpu}" PARENT_SCOPE)
endfunction()

function(_tinalux_resolve_android_ndk_api out_var)
    set(_ndk_api "")

    if(DEFINED ANDROID_PLATFORM AND NOT ANDROID_PLATFORM STREQUAL "")
        string(REGEX MATCH "android-([0-9]+)" _android_platform_match "${ANDROID_PLATFORM}")
        if(CMAKE_MATCH_1)
            set(_ndk_api "${CMAKE_MATCH_1}")
        endif()
    endif()

    if(_ndk_api STREQUAL "" AND DEFINED CMAKE_ANDROID_API AND NOT CMAKE_ANDROID_API STREQUAL "")
        set(_ndk_api "${CMAKE_ANDROID_API}")
    endif()

    if(_ndk_api STREQUAL "" AND CMAKE_SYSTEM_VERSION)
        set(_ndk_api "${CMAKE_SYSTEM_VERSION}")
    endif()

    set(${out_var} "${_ndk_api}" PARENT_SCOPE)
endfunction()

function(_tinalux_compute_skia_gn_args out_var build_type is_debug)
    set(_gn_args "")

    if("${is_debug}" STREQUAL "true")
        set(_tinalux_is_official_build "false")
    else()
        set(_tinalux_is_official_build "true")
    endif()
    list(APPEND _gn_args "is_official_build=${_tinalux_is_official_build}")
    list(APPEND _gn_args "is_component_build=false")
    list(APPEND _gn_args "is_debug=${is_debug}")
    unset(_tinalux_is_official_build)

    if(ANDROID)
        _tinalux_resolve_android_ndk(_tinalux_android_ndk)
        if(_tinalux_android_ndk STREQUAL "")
            message(FATAL_ERROR "Failed to resolve Android NDK path for Skia GN build.")
        endif()
        _tinalux_android_abi_to_gn_cpu(_tinalux_android_target_cpu)
        _tinalux_resolve_android_ndk_api(_tinalux_android_ndk_api)

        list(APPEND _gn_args "target_os=\"android\"")
        list(APPEND _gn_args "target_cpu=\"${_tinalux_android_target_cpu}\"")
        list(APPEND _gn_args "ndk=\"${_tinalux_android_ndk}\"")
        if(NOT _tinalux_android_ndk_api STREQUAL "")
            list(APPEND _gn_args "ndk_api=${_tinalux_android_ndk_api}")
        endif()

        unset(_tinalux_android_ndk)
        unset(_tinalux_android_ndk_api)
        unset(_tinalux_android_target_cpu)
    endif()

    if(MSVC)
        file(TO_CMAKE_PATH "${CMAKE_CXX_COMPILER}" _cxx_compiler_path)
        string(REGEX REPLACE "^(.*)/VC/Tools/MSVC/.*$" "\\1/VC" _vs_vc_dir "${_cxx_compiler_path}")
        if(EXISTS "${_vs_vc_dir}")
            list(APPEND _gn_args "win_vc=\"${_vs_vc_dir}\"")
        endif()

        if("${is_debug}" STREQUAL "true")
            set(_tinalux_msvc_crt "/MDd")
            set(_tinalux_msvc_common_flags "\"/MDd\"" "\"/D_DEBUG\"")
            set(_tinalux_msvc_cxx_flags "\"/MDd\"" "\"/D_DEBUG\"" "\"/D_ITERATOR_DEBUG_LEVEL=2\"" "\"/D_HAS_ITERATOR_DEBUGGING=1\"")
        else()
            set(_tinalux_msvc_crt "/MD")
            set(_tinalux_msvc_common_flags "\"/MD\"" "\"/DNDEBUG\"")
            set(_tinalux_msvc_cxx_flags "\"/MD\"" "\"/DNDEBUG\"" "\"/D_ITERATOR_DEBUG_LEVEL=0\"" "\"/D_HAS_ITERATOR_DEBUGGING=0\"")
        endif()
        string(REPLACE ";" ", " _tinalux_msvc_common_flags_str "${_tinalux_msvc_common_flags}")
        string(REPLACE ";" ", " _tinalux_msvc_cxx_flags_str "${_tinalux_msvc_cxx_flags}")
        list(APPEND _gn_args "extra_cflags=[${_tinalux_msvc_common_flags_str}]")
        list(APPEND _gn_args "extra_cflags_cc=[${_tinalux_msvc_cxx_flags_str}]")
        unset(_tinalux_msvc_crt)
        unset(_tinalux_msvc_common_flags)
        unset(_tinalux_msvc_common_flags_str)
        unset(_tinalux_msvc_cxx_flags)
        unset(_tinalux_msvc_cxx_flags_str)
    endif()

    if(SKIA_ENABLE_GPU)
        set(_skia_use_gl "true")
    else()
        set(_skia_use_gl "false")
    endif()

    if(SKIA_USE_DAWN)
        set(_skia_use_dawn "true")
    else()
        set(_skia_use_dawn "false")
    endif()

    if(SKIA_BUILD_TOOLS)
        set(_skia_enable_tools "true")
    else()
        set(_skia_enable_tools "false")
    endif()

    list(APPEND _gn_args "skia_use_gl=${_skia_use_gl}")
    list(APPEND _gn_args "skia_use_vulkan=true")
    if(APPLE)
        list(APPEND _gn_args "skia_use_metal=true")
    else()
        list(APPEND _gn_args "skia_use_metal=false")
    endif()
    list(APPEND _gn_args "skia_use_dawn=${_skia_use_dawn}")
    if(TINALUX_USE_SYSTEM_DEPS)
        set(_tinalux_use_system_deps "true")
    else()
        set(_tinalux_use_system_deps "false")
    endif()
    list(APPEND _gn_args "skia_use_system_zlib=${_tinalux_use_system_deps}")
    list(APPEND _gn_args "skia_use_system_libpng=${_tinalux_use_system_deps}")
    list(APPEND _gn_args "skia_use_system_libjpeg_turbo=${_tinalux_use_system_deps}")
    list(APPEND _gn_args "skia_use_system_libwebp=${_tinalux_use_system_deps}")
    list(APPEND _gn_args "skia_use_system_expat=${_tinalux_use_system_deps}")
    if(ANDROID)
        list(APPEND _gn_args "skia_use_system_freetype2=false")
    endif()
    unset(_tinalux_use_system_deps)
    list(APPEND _gn_args "skia_use_harfbuzz=true")
    list(APPEND _gn_args "skia_use_icu=true")
    list(APPEND _gn_args "skia_use_system_harfbuzz=false")
    list(APPEND _gn_args "skia_use_system_icu=false")
    list(APPEND _gn_args "skia_enable_tools=${_skia_enable_tools}")
    list(APPEND _gn_args "skia_enable_skottie=false")
    list(APPEND _gn_args "skia_enable_pdf=false")
    list(APPEND _gn_args "skia_enable_svg=false")
    list(APPEND _gn_args "skia_enable_ganesh=true")
    list(APPEND _gn_args "skia_enable_graphite=false")
    list(APPEND _gn_args "skia_enable_fontmgr_empty=false")
    if(ANDROID)
        list(APPEND _gn_args "skia_enable_fontmgr_android=false")
        list(APPEND _gn_args "skia_enable_fontmgr_android_ndk=true")
        list(APPEND _gn_args "skia_enable_fontmgr_win=false")
        list(APPEND _gn_args "skia_enable_fontmgr_win_gdi=false")
        list(APPEND _gn_args "skia_use_fontconfig=false")
    elseif(WIN32)
        list(APPEND _gn_args "skia_enable_fontmgr_win=true")
    else()
        list(APPEND _gn_args "skia_enable_fontmgr_win=false")
    endif()

    set(${out_var} "${_gn_args}" PARENT_SCOPE)
endfunction()

function(_tinalux_guess_prebuilt_lib_for_type out_lib_var build_type)
    set(_candidates "")
    if(WIN32)
        list(APPEND _candidates
            "${SKIA_SOURCE_DIR}/out/${build_type}/skia.lib"
            "${SKIA_SOURCE_DIR}/out/Release/skia.lib"
            "${SKIA_SOURCE_DIR}/out/Debug/skia.lib"
        )
    else()
        list(APPEND _candidates
            "${SKIA_SOURCE_DIR}/out/${build_type}/libskia.a"
            "${SKIA_SOURCE_DIR}/out/Release/libskia.a"
            "${SKIA_SOURCE_DIR}/out/Debug/libskia.a"
        )
    endif()

    foreach(_p IN LISTS _candidates)
        if(EXISTS "${_p}")
            set(${out_lib_var} "${_p}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_lib_var} "" PARENT_SCOPE)
endfunction()

function(_tinalux_guess_prebuilt_lib out_lib_var)
    _tinalux_guess_prebuilt_lib_for_type(${out_lib_var} "${SKIA_BUILD_TYPE}")
endfunction()

function(_tinalux_define_skia_build out_lib_var build_type is_debug)
    set(_skia_out_dir "${SKIA_BUILD_DIR}/${build_type}")
    _tinalux_compute_skia_gn_args(_gn_args "${build_type}" "${is_debug}")

    string(REPLACE ";" " " _gn_args_str "${_gn_args}")
    _tinalux_write_skia_expected_signature(_expected_signature_file "${build_type}" "${_gn_args_str}")
    set(_signature_marker "${_skia_out_dir}/tinalux_gn_signature.txt")

    add_custom_command(
        OUTPUT "${_skia_out_dir}/build.ninja" "${_signature_marker}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_skia_out_dir}"
        COMMAND "${GN_EXECUTABLE}" gen "${_skia_out_dir}" --args=${_gn_args_str}
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_expected_signature_file}" "${_signature_marker}"
        DEPENDS "${_expected_signature_file}"
        WORKING_DIRECTORY "${SKIA_SOURCE_DIR}"
        COMMENT "Configuring Skia ${build_type} (gn gen)"
        VERBATIM
    )

    _tinalux_skia_library_path(_skia_lib "${_skia_out_dir}")
    if(WIN32)
        set(_skia_paragraph_lib "${_skia_out_dir}/skparagraph.lib")
        set(_skia_shaper_lib "${_skia_out_dir}/skshaper.lib")
        set(_skia_unicode_core_lib "${_skia_out_dir}/skunicode_core.lib")
        set(_skia_unicode_icu_lib "${_skia_out_dir}/skunicode_icu.lib")
    else()
        set(_skia_paragraph_lib "${_skia_out_dir}/libskparagraph.a")
        set(_skia_shaper_lib "${_skia_out_dir}/libskshaper.a")
        set(_skia_unicode_core_lib "${_skia_out_dir}/libskunicode_core.a")
        set(_skia_unicode_icu_lib "${_skia_out_dir}/libskunicode_icu.a")
    endif()
    add_custom_command(
        OUTPUT
            "${_skia_lib}"
            "${_skia_paragraph_lib}"
            "${_skia_shaper_lib}"
            "${_skia_unicode_core_lib}"
            "${_skia_unicode_icu_lib}"
        COMMAND "${NINJA_EXECUTABLE}" -C "${_skia_out_dir}" skia skparagraph skshaper skunicode_core skunicode_icu
        DEPENDS "${_skia_out_dir}/build.ninja" "${_signature_marker}"
        WORKING_DIRECTORY "${SKIA_SOURCE_DIR}"
        COMMENT "Building Skia ${build_type} (ninja)"
        VERBATIM
    )

    set(${out_lib_var} "${_skia_lib}" PARENT_SCOPE)
endfunction()

function(tinalux_enable_skia)
    if(TARGET Skia::Skia)
        return()
    endif()

    unset(_skia_public_target)

    if(NOT EXISTS "${SKIA_SOURCE_DIR}/BUILD.gn")
        message(FATAL_ERROR "Skia source not found at '${SKIA_SOURCE_DIR}'. Expected BUILD.gn. Run sync script or set -DSKIA_SOURCE_DIR=...")
    endif()

    # 默认优先使用预编译产物；若不存在且工具链齐全，则自动切换到构建模式。
    set(_use_build_mode "${TINALUX_BUILD_SKIA}")
    if(NOT _use_build_mode)
        _tinalux_guess_prebuilt_lib(_skia_lib)
        if(NOT _skia_lib AND TINALUX_AUTO_BUILD_SKIA)
            _tinalux_find_gn(GN_EXECUTABLE)
            if(NINJA_EXECUTABLE AND GN_EXECUTABLE)
                message(STATUS "Tinalux: Prebuilt Skia not found; auto-building Skia (set -DTINALUX_AUTO_BUILD_SKIA=OFF to disable).")
                set(_use_build_mode ON)
            elseif(NOT NINJA_EXECUTABLE)
                message(STATUS "Tinalux: Prebuilt Skia not found; auto-build skipped because ninja was not found.")
            elseif(NOT GN_EXECUTABLE)
                message(STATUS "Tinalux: Prebuilt Skia not found; auto-build skipped because gn was not found.")
            endif()
        endif()
    endif()

    if(_use_build_mode)
        if(TINALUX_SYNC_SKIA_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES AND DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
            if(NOT SKIA_BUILD_TYPE STREQUAL CMAKE_BUILD_TYPE)
                message(STATUS "Tinalux: Sync SKIA_BUILD_TYPE='${SKIA_BUILD_TYPE}' -> '${CMAKE_BUILD_TYPE}'")
                set(SKIA_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Skia build configuration (Debug/Release)" FORCE)
            endif()
        endif()

        if(NOT NINJA_EXECUTABLE)
            message(FATAL_ERROR "ninja not found. Install Ninja or set TINALUX_BUILD_SKIA=OFF and point to a prebuilt Skia library.")
        endif()

        _tinalux_find_gn(GN_EXECUTABLE)
        if(NOT GN_EXECUTABLE)
            message(FATAL_ERROR "gn not found. Please run: `${Python3_EXECUTABLE} tools/git-sync-deps` in ${SKIA_SOURCE_DIR}")
        endif()
        message(STATUS "Tinalux: Using gn: ${GN_EXECUTABLE}")
        message(STATUS "Tinalux: Using ninja: ${NINJA_EXECUTABLE}")

        if(CMAKE_CONFIGURATION_TYPES)
            _tinalux_skia_library_path(_skia_debug_lib "${SKIA_BUILD_DIR}/Debug")
            _tinalux_skia_library_path(_skia_release_lib "${SKIA_BUILD_DIR}/Release")
            _tinalux_compute_skia_gn_args(_skia_debug_gn_args "Debug" "true")
            _tinalux_compute_skia_gn_args(_skia_release_gn_args "Release" "false")
            string(REPLACE ";" " " _skia_debug_gn_args_str "${_skia_debug_gn_args}")
            string(REPLACE ";" " " _skia_release_gn_args_str "${_skia_release_gn_args}")
            _tinalux_write_skia_expected_signature(_debug_signature_file "Debug" "${_skia_debug_gn_args_str}")
            _tinalux_write_skia_expected_signature(_release_signature_file "Release" "${_skia_release_gn_args_str}")
            _tinalux_skia_outputs_exist(_has_existing_debug_outputs "${SKIA_BUILD_DIR}/Debug" "${_debug_signature_file}")
            _tinalux_skia_outputs_exist(_has_existing_release_outputs "${SKIA_BUILD_DIR}/Release" "${_release_signature_file}")

            if(_has_existing_debug_outputs AND _has_existing_release_outputs)
                message(STATUS "Tinalux: Reusing existing Skia outputs under ${SKIA_BUILD_DIR}")
            endif()

            # 始终定义两套规则，确保多配置生成器在产物被删掉或过期时仍能重建。
            _tinalux_define_skia_build(_skia_debug_lib "Debug" "true")
            _tinalux_define_skia_build(_skia_release_lib "Release" "false")

            add_custom_target(tinalux_build_skia_debug DEPENDS "${_skia_debug_lib}")
            add_custom_target(tinalux_build_skia_release DEPENDS "${_skia_release_lib}")
            add_custom_target(tinalux_build_skia)
            add_dependencies(tinalux_build_skia tinalux_build_skia_debug tinalux_build_skia_release)

            add_library(tinalux_skia_debug INTERFACE)
            add_dependencies(tinalux_skia_debug tinalux_build_skia_debug)
            target_include_directories(tinalux_skia_debug INTERFACE
                "${SKIA_SOURCE_DIR}/include"
                "${SKIA_SOURCE_DIR}"
            )
            target_link_libraries(tinalux_skia_debug INTERFACE "${_skia_debug_lib}")

            add_library(tinalux_skia_release INTERFACE)
            add_dependencies(tinalux_skia_release tinalux_build_skia_release)
            target_include_directories(tinalux_skia_release INTERFACE
                "${SKIA_SOURCE_DIR}/include"
                "${SKIA_SOURCE_DIR}"
            )
            target_link_libraries(tinalux_skia_release INTERFACE "${_skia_release_lib}")

            add_library(tinalux_skia INTERFACE)
            add_dependencies(tinalux_skia tinalux_build_skia_debug tinalux_build_skia_release)
            target_link_libraries(tinalux_skia INTERFACE
                "$<$<CONFIG:Debug>:tinalux_skia_debug>"
                "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:tinalux_skia_release>"
            )
            add_library(Skia::Skia ALIAS tinalux_skia)
            set(_skia_public_target tinalux_skia)
        else()
            if(SKIA_BUILD_TYPE MATCHES "^[Dd]ebug$")
                set(_skia_is_debug "true")
            else()
                set(_skia_is_debug "false")
            endif()

            _tinalux_skia_library_path(_skia_lib "${SKIA_BUILD_DIR}/${SKIA_BUILD_TYPE}")
            _tinalux_compute_skia_gn_args(_skia_gn_args "${SKIA_BUILD_TYPE}" "${_skia_is_debug}")
            string(REPLACE ";" " " _skia_gn_args_str "${_skia_gn_args}")
            _tinalux_write_skia_expected_signature(_signature_file "${SKIA_BUILD_TYPE}" "${_skia_gn_args_str}")
            _tinalux_skia_outputs_exist(_has_existing_outputs "${SKIA_BUILD_DIR}/${SKIA_BUILD_TYPE}" "${_signature_file}")
            if(_has_existing_outputs)
                message(STATUS "Tinalux: Reusing existing Skia ${SKIA_BUILD_TYPE} outputs under ${SKIA_BUILD_DIR}")
            endif()
            _tinalux_define_skia_build(_skia_lib "${SKIA_BUILD_TYPE}" "${_skia_is_debug}")
            add_custom_target(tinalux_build_skia DEPENDS "${_skia_lib}")
            add_library(Skia::Skia STATIC IMPORTED GLOBAL)
            add_dependencies(Skia::Skia tinalux_build_skia)
            set_target_properties(Skia::Skia PROPERTIES
                IMPORTED_LOCATION "${_skia_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${SKIA_SOURCE_DIR}/include;${SKIA_SOURCE_DIR}"
            )
            set(_skia_public_target Skia::Skia)
        endif()
        if(WIN32 AND SKIA_ENABLE_GPU)
            set_property(TARGET ${_skia_public_target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES opengl32)
        endif()
    else()
        if(CMAKE_CONFIGURATION_TYPES)
            _tinalux_guess_prebuilt_lib_for_type(_skia_debug_lib "Debug")
            _tinalux_guess_prebuilt_lib_for_type(_skia_release_lib "Release")
            if(NOT _skia_release_lib)
                message(FATAL_ERROR "Release Skia library not found. Build Skia under `${SKIA_SOURCE_DIR}/out/Release` or set TINALUX_BUILD_SKIA=ON.")
            endif()
            if(NOT _skia_debug_lib)
                message(FATAL_ERROR "Debug Skia library not found. Build Skia under `${SKIA_SOURCE_DIR}/out/Debug` or set TINALUX_BUILD_SKIA=ON.")
            endif()

            add_library(Skia::Skia STATIC IMPORTED GLOBAL)
            set_target_properties(Skia::Skia PROPERTIES
                IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"
                IMPORTED_LOCATION_DEBUG "${_skia_debug_lib}"
                IMPORTED_LOCATION_RELEASE "${_skia_release_lib}"
                IMPORTED_LOCATION_RELWITHDEBINFO "${_skia_release_lib}"
                IMPORTED_LOCATION_MINSIZEREL "${_skia_release_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${SKIA_SOURCE_DIR}/include;${SKIA_SOURCE_DIR}"
            )
            set(_skia_public_target Skia::Skia)
        else()
            add_library(Skia::Skia STATIC IMPORTED GLOBAL)
            if(NOT _skia_lib)
                _tinalux_guess_prebuilt_lib(_skia_lib)
            endif()
            if(NOT _skia_lib)
                message(FATAL_ERROR "Prebuilt Skia library not found. Build Skia under `${SKIA_SOURCE_DIR}/out/<cfg>` or set TINALUX_BUILD_SKIA=ON (requires gn+ninja).")
            endif()

            set_target_properties(Skia::Skia PROPERTIES
                IMPORTED_LOCATION "${_skia_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${SKIA_SOURCE_DIR}/include;${SKIA_SOURCE_DIR}"
            )
            set(_skia_public_target Skia::Skia)
        endif()
        if(WIN32 AND SKIA_ENABLE_GPU)
            set_property(TARGET ${_skia_public_target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES opengl32)
        endif()
    endif()
endfunction()
