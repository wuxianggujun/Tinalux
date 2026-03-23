include_guard(GLOBAL)

function(_tinalux_markup_default_include_guard out_var output_path)
    get_filename_component(_tinalux_header_name "${output_path}" NAME)
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" _tinalux_guard "${_tinalux_header_name}")
    string(TOUPPER "${_tinalux_guard}" _tinalux_guard)
    if(NOT _tinalux_guard MATCHES "^[A-Z_]")
        set(_tinalux_guard "TINALUX_${_tinalux_guard}")
    endif()
    set(${out_var} "${_tinalux_guard}" PARENT_SCOPE)
endfunction()

function(_tinalux_markup_make_namespace_segment out_var raw_segment)
    string(MAKE_C_IDENTIFIER "${raw_segment}" _tinalux_segment)
    if(_tinalux_segment MATCHES "^[0-9]")
        set(_tinalux_segment "_${_tinalux_segment}")
    endif()
    if(_tinalux_segment STREQUAL "")
        set(_tinalux_segment "layout")
    endif()
    set(${out_var} "${_tinalux_segment}" PARENT_SCOPE)
endfunction()

function(_tinalux_markup_namespace_for_relative_path out_var prefix relative_path)
    set(_tinalux_segments)
    if(prefix)
        string(REPLACE "::" ";" _tinalux_prefix_segments "${prefix}")
        foreach(_tinalux_segment IN LISTS _tinalux_prefix_segments)
            if(NOT _tinalux_segment STREQUAL "")
                list(APPEND _tinalux_segments "${_tinalux_segment}")
            endif()
        endforeach()
    endif()

    cmake_path(GET relative_path PARENT_PATH _tinalux_parent_path)
    if(_tinalux_parent_path)
        string(REPLACE "/" ";" _tinalux_parent_segments "${_tinalux_parent_path}")
        foreach(_tinalux_segment IN LISTS _tinalux_parent_segments)
            _tinalux_markup_make_namespace_segment(
                _tinalux_sanitized_segment
                "${_tinalux_segment}")
            list(APPEND _tinalux_segments "${_tinalux_sanitized_segment}")
        endforeach()
    endif()

    cmake_path(GET relative_path STEM _tinalux_stem)
    _tinalux_markup_make_namespace_segment(_tinalux_stem_segment "${_tinalux_stem}")
    list(APPEND _tinalux_segments "${_tinalux_stem_segment}" "slots")

    string(JOIN "::" _tinalux_namespace ${_tinalux_segments})
    set(${out_var} "${_tinalux_namespace}" PARENT_SCOPE)
endfunction()

function(_tinalux_markup_default_autogen_output_dir out_var target_name)
    string(MAKE_C_IDENTIFIER "${target_name}" _tinalux_target_id)
    if(_tinalux_target_id STREQUAL "")
        set(_tinalux_target_id "markup")
    endif()
    set(
        ${out_var}
        "${CMAKE_CURRENT_BINARY_DIR}/tinalux_markup/${_tinalux_target_id}"
        PARENT_SCOPE)
endfunction()

function(_tinalux_markup_default_autogen_index_header out_var target_name)
    string(MAKE_C_IDENTIFIER "${target_name}" _tinalux_target_id)
    if(_tinalux_target_id STREQUAL "")
        set(_tinalux_target_id "markup")
    endif()
    set(${out_var} "${_tinalux_target_id}.markup.h" PARENT_SCOPE)
endfunction()

function(_tinalux_markup_default_single_namespace out_var input_path namespace_prefix)
    if(namespace_prefix)
        if("${namespace_prefix}" MATCHES "(^|::)slots$")
            set(${out_var} "${namespace_prefix}" PARENT_SCOPE)
        else()
            set(${out_var} "${namespace_prefix}::slots" PARENT_SCOPE)
        endif()
        return()
    endif()

    cmake_path(GET input_path STEM _tinalux_markup_stem)
    _tinalux_markup_make_namespace_segment(_tinalux_markup_segment "${_tinalux_markup_stem}")
    set(${out_var} "${_tinalux_markup_segment}::slots" PARENT_SCOPE)
endfunction()

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

function(tinalux_generate_markup_page_scaffold)
    set(options NO_PRAGMA_ONCE ONLY_IF_MISSING)
    set(oneValueArgs TARGET INPUT OUTPUT NAMESPACE MARKUP_HEADER CLASS_NAME INCLUDE_GUARD)
    cmake_parse_arguments(TINALUX_MARKUP_SCAFFOLD "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT TINALUX_MARKUP_SCAFFOLD_OUTPUT)
        message(FATAL_ERROR "tinalux_generate_markup_page_scaffold requires OUTPUT")
    endif()
    if(NOT TARGET TinaluxMarkupActionHeaderTool)
        message(FATAL_ERROR
            "TinaluxMarkupActionHeaderTool is unavailable. "
            "Ensure src/markup/CMakeLists.txt is included before calling "
            "tinalux_generate_markup_page_scaffold.")
    endif()

    set(_tinalux_scaffold_input "${TINALUX_MARKUP_SCAFFOLD_INPUT}")
    set(_tinalux_scaffold_namespace "${TINALUX_MARKUP_SCAFFOLD_NAMESPACE}")
    set(_tinalux_scaffold_markup_header "${TINALUX_MARKUP_SCAFFOLD_MARKUP_HEADER}")
    if(TINALUX_MARKUP_SCAFFOLD_TARGET)
        if(NOT TARGET "${TINALUX_MARKUP_SCAFFOLD_TARGET}")
            message(FATAL_ERROR
                "tinalux_generate_markup_page_scaffold TARGET '${TINALUX_MARKUP_SCAFFOLD_TARGET}' does not exist")
        endif()

        if(NOT _tinalux_scaffold_input)
            get_property(
                _tinalux_scaffold_input
                TARGET "${TINALUX_MARKUP_SCAFFOLD_TARGET}"
                PROPERTY TINALUX_MARKUP_AUTOGEN_INPUT)
        endif()
        if(NOT _tinalux_scaffold_namespace)
            get_property(
                _tinalux_scaffold_namespace
                TARGET "${TINALUX_MARKUP_SCAFFOLD_TARGET}"
                PROPERTY TINALUX_MARKUP_AUTOGEN_NAMESPACE)
        endif()
        if(NOT _tinalux_scaffold_markup_header)
            get_property(
                _tinalux_scaffold_markup_header
                TARGET "${TINALUX_MARKUP_SCAFFOLD_TARGET}"
                PROPERTY TINALUX_MARKUP_AUTOGEN_OUTPUT_HEADER)
        endif()
    endif()

    if(NOT _tinalux_scaffold_input)
        message(FATAL_ERROR
            "tinalux_generate_markup_page_scaffold requires INPUT, or a TARGET with single-file markup autogen")
    endif()
    if(NOT _tinalux_scaffold_namespace)
        message(FATAL_ERROR
            "tinalux_generate_markup_page_scaffold requires NAMESPACE, or a TARGET with single-file markup autogen")
    endif()
    if(NOT _tinalux_scaffold_markup_header)
        message(FATAL_ERROR
            "tinalux_generate_markup_page_scaffold requires MARKUP_HEADER, or a TARGET with single-file markup autogen")
    endif()

    get_filename_component(
        _tinalux_scaffold_input_abs
        "${_tinalux_scaffold_input}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(
        _tinalux_scaffold_output_abs
        "${TINALUX_MARKUP_SCAFFOLD_OUTPUT}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    get_filename_component(
        _tinalux_scaffold_output_dir
        "${_tinalux_scaffold_output_abs}"
        DIRECTORY)

    set(_tinalux_scaffold_markup_include "${_tinalux_scaffold_markup_header}")
    if(IS_ABSOLUTE "${_tinalux_scaffold_markup_header}")
        file(RELATIVE_PATH
            _tinalux_scaffold_markup_include
            "${_tinalux_scaffold_output_dir}"
            "${_tinalux_scaffold_markup_header}")
        string(REPLACE "\\" "/" _tinalux_scaffold_markup_include "${_tinalux_scaffold_markup_include}")
    endif()

    set(_tinalux_scaffold_command
        "$<TARGET_FILE:TinaluxMarkupActionHeaderTool>"
        --input "${_tinalux_scaffold_input_abs}"
        --namespace "${_tinalux_scaffold_namespace}"
        --scaffold-output "${_tinalux_scaffold_output_abs}"
        --scaffold-markup-header "${_tinalux_scaffold_markup_include}")
    if(TINALUX_MARKUP_SCAFFOLD_CLASS_NAME)
        list(APPEND _tinalux_scaffold_command
            --scaffold-class "${TINALUX_MARKUP_SCAFFOLD_CLASS_NAME}")
    endif()
    if(TINALUX_MARKUP_SCAFFOLD_INCLUDE_GUARD)
        list(APPEND _tinalux_scaffold_command
            --scaffold-include-guard "${TINALUX_MARKUP_SCAFFOLD_INCLUDE_GUARD}")
    endif()
    if(TINALUX_MARKUP_SCAFFOLD_ONLY_IF_MISSING)
        list(APPEND _tinalux_scaffold_command --scaffold-only-if-missing)
    endif()
    if(TINALUX_MARKUP_SCAFFOLD_NO_PRAGMA_ONCE)
        list(APPEND _tinalux_scaffold_command --no-pragma-once)
    endif()

    add_custom_command(
        OUTPUT "${_tinalux_scaffold_output_abs}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_tinalux_scaffold_output_dir}"
        COMMAND ${_tinalux_scaffold_command}
        DEPENDS "${_tinalux_scaffold_input_abs}" TinaluxMarkupActionHeaderTool
        COMMENT "Generating markup page scaffold: ${_tinalux_scaffold_output_abs}"
        VERBATIM
    )

    if(TINALUX_MARKUP_SCAFFOLD_TARGET)
        string(SHA1 _tinalux_scaffold_hash "${_tinalux_scaffold_output_abs}")
        string(MAKE_C_IDENTIFIER
            "tinalux_markup_page_scaffold_${TINALUX_MARKUP_SCAFFOLD_TARGET}_${_tinalux_scaffold_hash}"
            _tinalux_scaffold_target)
        add_custom_target("${_tinalux_scaffold_target}" DEPENDS "${_tinalux_scaffold_output_abs}")
        add_dependencies("${TINALUX_MARKUP_SCAFFOLD_TARGET}" "${_tinalux_scaffold_target}")
        target_sources("${TINALUX_MARKUP_SCAFFOLD_TARGET}" PRIVATE "${_tinalux_scaffold_output_abs}")
        target_include_directories("${TINALUX_MARKUP_SCAFFOLD_TARGET}" PRIVATE "${_tinalux_scaffold_output_dir}")
    endif()
endfunction()

function(tinalux_target_enable_markup_autogen)
    set(options)
    set(oneValueArgs
        TARGET
        INPUT
        DIRECTORY
        NAMESPACE
        NAMESPACE_PREFIX
        OUTPUT_DIRECTORY
        INDEX_HEADER
        OUTPUT_HEADER)
    cmake_parse_arguments(TINALUX_MARKUP_AUTOGEN "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT TINALUX_MARKUP_AUTOGEN_TARGET)
        message(FATAL_ERROR "tinalux_target_enable_markup_autogen requires TARGET")
    endif()
    if(NOT TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}")
        message(FATAL_ERROR
            "tinalux_target_enable_markup_autogen TARGET '${TINALUX_MARKUP_AUTOGEN_TARGET}' does not exist")
    endif()
    if(TINALUX_MARKUP_AUTOGEN_INPUT AND TINALUX_MARKUP_AUTOGEN_DIRECTORY)
        message(FATAL_ERROR
            "tinalux_target_enable_markup_autogen accepts either INPUT or DIRECTORY, not both")
    endif()
    if(NOT TINALUX_MARKUP_AUTOGEN_INPUT AND NOT TINALUX_MARKUP_AUTOGEN_DIRECTORY)
        message(FATAL_ERROR
            "tinalux_target_enable_markup_autogen requires either INPUT or DIRECTORY")
    endif()

    set(_tinalux_autogen_output_dir "${TINALUX_MARKUP_AUTOGEN_OUTPUT_DIRECTORY}")
    if(NOT _tinalux_autogen_output_dir)
        _tinalux_markup_default_autogen_output_dir(
            _tinalux_autogen_output_dir
            "${TINALUX_MARKUP_AUTOGEN_TARGET}")
    endif()

    set(_tinalux_autogen_index_header "${TINALUX_MARKUP_AUTOGEN_INDEX_HEADER}")
    if(NOT _tinalux_autogen_index_header)
        _tinalux_markup_default_autogen_index_header(
            _tinalux_autogen_index_header
            "${TINALUX_MARKUP_AUTOGEN_TARGET}")
    endif()

    if(IS_ABSOLUTE "${_tinalux_autogen_output_dir}")
        set(_tinalux_autogen_output_dir_abs "${_tinalux_autogen_output_dir}")
    else()
        get_filename_component(
            _tinalux_autogen_output_dir_abs
            "${_tinalux_autogen_output_dir}"
            ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if(IS_ABSOLUTE "${_tinalux_autogen_index_header}")
        set(_tinalux_autogen_index_header_abs "${_tinalux_autogen_index_header}")
    else()
        set(
            _tinalux_autogen_index_header_abs
            "${_tinalux_autogen_output_dir_abs}/${_tinalux_autogen_index_header}")
    endif()

    get_filename_component(
        _tinalux_autogen_index_header_dir
        "${_tinalux_autogen_index_header_abs}"
        DIRECTORY)

    if(TINALUX_MARKUP_AUTOGEN_DIRECTORY)
        tinalux_generate_markup_bindings_for_directory(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            DIRECTORY "${TINALUX_MARKUP_AUTOGEN_DIRECTORY}"
            OUTPUT_DIRECTORY "${_tinalux_autogen_output_dir}"
            NAMESPACE_PREFIX "${TINALUX_MARKUP_AUTOGEN_NAMESPACE_PREFIX}"
            INDEX_HEADER "${_tinalux_autogen_index_header}"
        )

        target_include_directories(
            "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PRIVATE
                "${_tinalux_autogen_index_header_dir}")

        set_property(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PROPERTY TINALUX_MARKUP_AUTOGEN_MODE
            "DIRECTORY")
        set_property(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PROPERTY TINALUX_MARKUP_AUTOGEN_OUTPUT_DIRECTORY
            "${_tinalux_autogen_output_dir_abs}")
        set_property(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PROPERTY TINALUX_MARKUP_AUTOGEN_INDEX_HEADER
            "${_tinalux_autogen_index_header_abs}")
        return()
    endif()

    set(_tinalux_single_output_header "${TINALUX_MARKUP_AUTOGEN_OUTPUT_HEADER}")
    if(NOT _tinalux_single_output_header)
        _tinalux_markup_default_autogen_index_header(
            _tinalux_single_output_header
            "${TINALUX_MARKUP_AUTOGEN_TARGET}")
    endif()

    if(IS_ABSOLUTE "${_tinalux_single_output_header}")
        set(_tinalux_single_output_header_abs "${_tinalux_single_output_header}")
    else()
        set(
            _tinalux_single_output_header_abs
            "${_tinalux_autogen_output_dir_abs}/${_tinalux_single_output_header}")
    endif()

    set(_tinalux_single_namespace "${TINALUX_MARKUP_AUTOGEN_NAMESPACE}")
    if(NOT _tinalux_single_namespace)
        _tinalux_markup_default_single_namespace(
            _tinalux_single_namespace
            "${TINALUX_MARKUP_AUTOGEN_INPUT}"
            "${TINALUX_MARKUP_AUTOGEN_NAMESPACE_PREFIX}")
    endif()

    tinalux_generate_markup_bindings(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        INPUT "${TINALUX_MARKUP_AUTOGEN_INPUT}"
        OUTPUT "${_tinalux_single_output_header_abs}"
        NAMESPACE "${_tinalux_single_namespace}"
    )

    get_filename_component(
        _tinalux_single_output_header_dir
        "${_tinalux_single_output_header_abs}"
        DIRECTORY)
    target_include_directories(
        "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PRIVATE
            "${_tinalux_single_output_header_dir}")

    set_property(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PROPERTY TINALUX_MARKUP_AUTOGEN_MODE
        "INPUT")
    set_property(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PROPERTY TINALUX_MARKUP_AUTOGEN_INPUT
        "${TINALUX_MARKUP_AUTOGEN_INPUT}")
    set_property(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PROPERTY TINALUX_MARKUP_AUTOGEN_NAMESPACE
        "${_tinalux_single_namespace}")
    set_property(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PROPERTY TINALUX_MARKUP_AUTOGEN_OUTPUT_HEADER
        "${_tinalux_single_output_header_abs}")
    set_property(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PROPERTY TINALUX_MARKUP_AUTOGEN_OUTPUT_DIRECTORY
        "${_tinalux_autogen_output_dir_abs}")
    set_property(
        TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
        PROPERTY TINALUX_MARKUP_AUTOGEN_INDEX_HEADER
        "${_tinalux_single_output_header_abs}")
endfunction()

function(tinalux_add_markup_executable)
    set(options COPY_SKIA_DATA)
    set(oneValueArgs
        TARGET
        SOURCE
        INPUT
        DIRECTORY
        NAMESPACE
        NAMESPACE_PREFIX
        OUTPUT_DIRECTORY
        INDEX_HEADER
        OUTPUT_HEADER)
    set(multiValueArgs SOURCES LINK_LIBS)
    cmake_parse_arguments(TINALUX_MARKUP_EXE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TINALUX_MARKUP_EXE_TARGET)
        message(FATAL_ERROR "tinalux_add_markup_executable requires TARGET")
    endif()

    set(_tinalux_markup_exe_sources ${TINALUX_MARKUP_EXE_SOURCES})
    if(TINALUX_MARKUP_EXE_SOURCE)
        list(APPEND _tinalux_markup_exe_sources "${TINALUX_MARKUP_EXE_SOURCE}")
    endif()
    if(NOT _tinalux_markup_exe_sources)
        message(FATAL_ERROR
            "tinalux_add_markup_executable(${TINALUX_MARKUP_EXE_TARGET}) requires SOURCE or SOURCES")
    endif()

    add_executable(${TINALUX_MARKUP_EXE_TARGET}
        ${_tinalux_markup_exe_sources}
    )

    target_link_libraries(${TINALUX_MARKUP_EXE_TARGET}
        PRIVATE
            Tinalux::Markup
            ${TINALUX_MARKUP_EXE_LINK_LIBS}
    )

    if(TINALUX_MARKUP_EXE_COPY_SKIA_DATA)
        if(COMMAND tinalux_copy_skia_runtime_data)
            tinalux_copy_skia_runtime_data(${TINALUX_MARKUP_EXE_TARGET})
        else()
            message(FATAL_ERROR
                "tinalux_add_markup_executable(${TINALUX_MARKUP_EXE_TARGET} COPY_SKIA_DATA) "
                "requires tinalux_copy_skia_runtime_data() to be defined")
        endif()
    endif()

    set(_tinalux_markup_exe_autogen_args
        TARGET "${TINALUX_MARKUP_EXE_TARGET}")
    foreach(_tinalux_markup_exe_arg
        INPUT
        DIRECTORY
        NAMESPACE
        NAMESPACE_PREFIX
        OUTPUT_DIRECTORY
        INDEX_HEADER
        OUTPUT_HEADER)
        if(NOT "${TINALUX_MARKUP_EXE_${_tinalux_markup_exe_arg}}" STREQUAL "")
            list(APPEND
                _tinalux_markup_exe_autogen_args
                ${_tinalux_markup_exe_arg}
                "${TINALUX_MARKUP_EXE_${_tinalux_markup_exe_arg}}")
        endif()
    endforeach()

    tinalux_target_enable_markup_autogen(${_tinalux_markup_exe_autogen_args})
endfunction()
