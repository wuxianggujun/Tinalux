include_guard(GLOBAL)
include(TinaluxMarkupCommonTools)

# 低层 bindings 生成 helper。

function(tinalux_generate_markup_bindings)
    set(options NO_PRAGMA_ONCE)
    set(oneValueArgs TARGET INPUT OUTPUT NAMESPACE INCLUDE_GUARD)
    cmake_parse_arguments(TINALUX_MARKUP "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT TINALUX_MARKUP_INPUT)
        message(FATAL_ERROR "tinalux_generate_markup_bindings requires INPUT")
    endif()
    if(NOT TINALUX_MARKUP_OUTPUT)
        message(FATAL_ERROR "tinalux_generate_markup_bindings requires OUTPUT")
    endif()

    if(NOT TARGET TinaluxMarkupActionHeaderTool)
        message(FATAL_ERROR
            "TinaluxMarkupActionHeaderTool is unavailable. "
            "Ensure src/markup/CMakeLists.txt is included before calling "
            "tinalux_generate_markup_bindings.")
    endif()

    get_filename_component(_tinalux_input "${TINALUX_MARKUP_INPUT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(_tinalux_output "${TINALUX_MARKUP_OUTPUT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    get_filename_component(_tinalux_output_dir "${_tinalux_output}" DIRECTORY)

    set(_tinalux_guard "${TINALUX_MARKUP_INCLUDE_GUARD}")
    if(NOT _tinalux_guard)
        _tinalux_markup_default_include_guard(_tinalux_guard "${_tinalux_output}")
    endif()

    set(_tinalux_command
        "$<TARGET_FILE:TinaluxMarkupActionHeaderTool>"
        --input "${_tinalux_input}"
        --output "${_tinalux_output}"
        --include-guard "${_tinalux_guard}")
    if(TINALUX_MARKUP_NAMESPACE)
        list(APPEND _tinalux_command --namespace "${TINALUX_MARKUP_NAMESPACE}")
    endif()
    if(TINALUX_MARKUP_NO_PRAGMA_ONCE)
        list(APPEND _tinalux_command --no-pragma-once)
    endif()

    add_custom_command(
        OUTPUT "${_tinalux_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_tinalux_output_dir}"
        COMMAND ${_tinalux_command}
        DEPENDS "${_tinalux_input}" TinaluxMarkupActionHeaderTool
        COMMENT "Generating markup action bindings: ${_tinalux_output}"
        VERBATIM
    )

    if(TINALUX_MARKUP_TARGET)
        if(NOT TARGET "${TINALUX_MARKUP_TARGET}")
            message(FATAL_ERROR
                "tinalux_generate_markup_bindings TARGET '${TINALUX_MARKUP_TARGET}' does not exist")
        endif()

        string(SHA1 _tinalux_binding_hash "${_tinalux_output}")
        string(MAKE_C_IDENTIFIER
            "tinalux_markup_bindings_${TINALUX_MARKUP_TARGET}_${_tinalux_binding_hash}"
            _tinalux_binding_target)
        add_custom_target("${_tinalux_binding_target}" DEPENDS "${_tinalux_output}")
        add_dependencies("${TINALUX_MARKUP_TARGET}" "${_tinalux_binding_target}")
        target_sources("${TINALUX_MARKUP_TARGET}" PRIVATE "${_tinalux_output}")
        target_include_directories("${TINALUX_MARKUP_TARGET}" PRIVATE "${_tinalux_output_dir}")
    endif()
endfunction()

function(tinalux_generate_markup_bindings_for_directory)
    set(options)
    set(oneValueArgs TARGET DIRECTORY OUTPUT_DIRECTORY NAMESPACE_PREFIX INDEX_HEADER)
    cmake_parse_arguments(TINALUX_MARKUP_DIR "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT TINALUX_MARKUP_DIR_DIRECTORY)
        message(FATAL_ERROR "tinalux_generate_markup_bindings_for_directory requires DIRECTORY")
    endif()
    if(NOT TINALUX_MARKUP_DIR_OUTPUT_DIRECTORY)
        message(FATAL_ERROR
            "tinalux_generate_markup_bindings_for_directory requires OUTPUT_DIRECTORY")
    endif()

    get_filename_component(
        _tinalux_markup_input_dir
        "${TINALUX_MARKUP_DIR_DIRECTORY}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(
        _tinalux_markup_output_dir
        "${TINALUX_MARKUP_DIR_OUTPUT_DIRECTORY}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

    file(GLOB_RECURSE _tinalux_markup_files
        RELATIVE "${_tinalux_markup_input_dir}"
        CONFIGURE_DEPENDS
        "${_tinalux_markup_input_dir}/*.tui")
    list(SORT _tinalux_markup_files)

    set(_tinalux_index_includes)
    foreach(_tinalux_markup_file IN LISTS _tinalux_markup_files)
        cmake_path(GET _tinalux_markup_file PARENT_PATH _tinalux_markup_parent)
        cmake_path(GET _tinalux_markup_file STEM _tinalux_markup_stem)
        if(_tinalux_markup_parent)
            set(_tinalux_generated_relative
                "${_tinalux_markup_parent}/${_tinalux_markup_stem}.slots.h")
        else()
            set(_tinalux_generated_relative "${_tinalux_markup_stem}.slots.h")
        endif()

        set(_tinalux_generated_output
            "${_tinalux_markup_output_dir}/${_tinalux_generated_relative}")
        _tinalux_markup_namespace_for_relative_path(
            _tinalux_markup_namespace
            "${TINALUX_MARKUP_DIR_NAMESPACE_PREFIX}"
            "${_tinalux_markup_file}")

        tinalux_generate_markup_bindings(
            TARGET "${TINALUX_MARKUP_DIR_TARGET}"
            INPUT "${_tinalux_markup_input_dir}/${_tinalux_markup_file}"
            OUTPUT "${_tinalux_generated_output}"
            NAMESPACE "${_tinalux_markup_namespace}"
        )

        list(APPEND _tinalux_index_includes "${_tinalux_generated_relative}")
    endforeach()

    if(TINALUX_MARKUP_DIR_TARGET)
        target_include_directories(
            "${TINALUX_MARKUP_DIR_TARGET}"
            PRIVATE
                "${_tinalux_markup_output_dir}")
    endif()

    if(TINALUX_MARKUP_DIR_INDEX_HEADER)
        if(IS_ABSOLUTE "${TINALUX_MARKUP_DIR_INDEX_HEADER}")
            set(_tinalux_index_header_path "${TINALUX_MARKUP_DIR_INDEX_HEADER}")
        else()
            set(_tinalux_index_header_path
                "${_tinalux_markup_output_dir}/${TINALUX_MARKUP_DIR_INDEX_HEADER}")
        endif()

        get_filename_component(_tinalux_index_header_dir "${_tinalux_index_header_path}" DIRECTORY)
        file(MAKE_DIRECTORY "${_tinalux_index_header_dir}")
        file(WRITE "${_tinalux_index_header_path}" "#pragma once\n\n")
        foreach(_tinalux_generated_include IN LISTS _tinalux_index_includes)
            file(APPEND
                "${_tinalux_index_header_path}"
                "#include \"${_tinalux_generated_include}\"\n")
        endforeach()

        if(TINALUX_MARKUP_DIR_TARGET)
            target_sources("${TINALUX_MARKUP_DIR_TARGET}" PRIVATE "${_tinalux_index_header_path}")
        endif()
    endif()
endfunction()
