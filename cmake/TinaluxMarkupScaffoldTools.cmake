include_guard(GLOBAL)
include(TinaluxMarkupCommonTools)

# 低层 page scaffold 生成 helper。

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

function(tinalux_generate_markup_page_scaffolds_for_directory)
    set(options NO_PRAGMA_ONCE ONLY_IF_MISSING)
    set(oneValueArgs
        TARGET
        DIRECTORY
        OUTPUT_DIRECTORY
        NAMESPACE_PREFIX
        MARKUP_OUTPUT_DIRECTORY
        INDEX_HEADER)
    cmake_parse_arguments(
        TINALUX_MARKUP_SCAFFOLD_DIR
        "${options}"
        "${oneValueArgs}"
        ""
        ${ARGN})

    if(NOT TARGET TinaluxMarkupActionHeaderTool)
        message(FATAL_ERROR
            "TinaluxMarkupActionHeaderTool is unavailable. "
            "Ensure src/markup/CMakeLists.txt is included before calling "
            "tinalux_generate_markup_page_scaffolds_for_directory.")
    endif()

    set(_tinalux_scaffold_dir_input "${TINALUX_MARKUP_SCAFFOLD_DIR_DIRECTORY}")
    set(_tinalux_scaffold_namespace_prefix "${TINALUX_MARKUP_SCAFFOLD_DIR_NAMESPACE_PREFIX}")
    set(_tinalux_scaffold_markup_output_dir "${TINALUX_MARKUP_SCAFFOLD_DIR_MARKUP_OUTPUT_DIRECTORY}")
    set(_tinalux_scaffold_output_dir "${TINALUX_MARKUP_SCAFFOLD_DIR_OUTPUT_DIRECTORY}")
    set(_tinalux_scaffold_index_header "${TINALUX_MARKUP_SCAFFOLD_DIR_INDEX_HEADER}")

    if(TINALUX_MARKUP_SCAFFOLD_DIR_TARGET)
        if(NOT TARGET "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}")
            message(FATAL_ERROR
                "tinalux_generate_markup_page_scaffolds_for_directory TARGET '${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}' does not exist")
        endif()

        if(NOT _tinalux_scaffold_dir_input)
            get_property(
                _tinalux_scaffold_dir_input
                TARGET "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}"
                PROPERTY TINALUX_MARKUP_AUTOGEN_DIRECTORY)
        endif()
        if(NOT _tinalux_scaffold_namespace_prefix)
            get_property(
                _tinalux_scaffold_namespace_prefix
                TARGET "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}"
                PROPERTY TINALUX_MARKUP_AUTOGEN_NAMESPACE_PREFIX)
        endif()
        if(NOT _tinalux_scaffold_markup_output_dir)
            get_property(
                _tinalux_scaffold_markup_output_dir
                TARGET "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}"
                PROPERTY TINALUX_MARKUP_AUTOGEN_OUTPUT_DIRECTORY)
        endif()
        if(NOT _tinalux_scaffold_output_dir)
            _tinalux_markup_default_page_scaffold_output_dir(
                _tinalux_scaffold_output_dir
                "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}")
        endif()
        if(NOT _tinalux_scaffold_index_header)
            _tinalux_markup_default_page_scaffold_index_header(
                _tinalux_scaffold_index_header
                "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}")
        endif()
    endif()

    if(NOT _tinalux_scaffold_dir_input)
        message(FATAL_ERROR
            "tinalux_generate_markup_page_scaffolds_for_directory requires DIRECTORY, or a TARGET with directory markup autogen")
    endif()
    if(NOT _tinalux_scaffold_output_dir)
        message(FATAL_ERROR
            "tinalux_generate_markup_page_scaffolds_for_directory requires OUTPUT_DIRECTORY")
    endif()
    if(NOT _tinalux_scaffold_markup_output_dir)
        message(FATAL_ERROR
            "tinalux_generate_markup_page_scaffolds_for_directory requires MARKUP_OUTPUT_DIRECTORY, or a TARGET with directory markup autogen")
    endif()

    get_filename_component(
        _tinalux_scaffold_dir_input_abs
        "${_tinalux_scaffold_dir_input}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(
        _tinalux_scaffold_dir_output_abs
        "${_tinalux_scaffold_output_dir}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    get_filename_component(
        _tinalux_scaffold_markup_output_dir_abs
        "${_tinalux_scaffold_markup_output_dir}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

    file(GLOB_RECURSE _tinalux_scaffold_markup_files
        RELATIVE "${_tinalux_scaffold_dir_input_abs}"
        CONFIGURE_DEPENDS
        "${_tinalux_scaffold_dir_input_abs}/*.tui")
    list(SORT _tinalux_scaffold_markup_files)

    set(_tinalux_scaffold_index_includes)
    foreach(_tinalux_scaffold_markup_file IN LISTS _tinalux_scaffold_markup_files)
        cmake_path(GET _tinalux_scaffold_markup_file PARENT_PATH _tinalux_scaffold_parent)
        cmake_path(GET _tinalux_scaffold_markup_file STEM _tinalux_scaffold_stem)

        if(_tinalux_scaffold_parent)
            set(_tinalux_scaffold_relative_header
                "${_tinalux_scaffold_parent}/${_tinalux_scaffold_stem}.page.h")
            set(_tinalux_scaffold_relative_markup_header
                "${_tinalux_scaffold_parent}/${_tinalux_scaffold_stem}.slots.h")
        else()
            set(_tinalux_scaffold_relative_header "${_tinalux_scaffold_stem}.page.h")
            set(_tinalux_scaffold_relative_markup_header "${_tinalux_scaffold_stem}.slots.h")
        endif()

        set(_tinalux_scaffold_output_header
            "${_tinalux_scaffold_dir_output_abs}/${_tinalux_scaffold_relative_header}")
        get_filename_component(
            _tinalux_scaffold_output_header_dir
            "${_tinalux_scaffold_output_header}"
            DIRECTORY)

        set(_tinalux_scaffold_markup_header_abs
            "${_tinalux_scaffold_markup_output_dir_abs}/${_tinalux_scaffold_relative_markup_header}")
        file(RELATIVE_PATH
            _tinalux_scaffold_markup_header_include
            "${_tinalux_scaffold_output_header_dir}"
            "${_tinalux_scaffold_markup_header_abs}")
        string(REPLACE "\\" "/" _tinalux_scaffold_markup_header_include "${_tinalux_scaffold_markup_header_include}")

        _tinalux_markup_namespace_for_relative_path(
            _tinalux_scaffold_namespace
            "${_tinalux_scaffold_namespace_prefix}"
            "${_tinalux_scaffold_markup_file}")
        _tinalux_markup_scaffold_class_name_for_relative_path(
            _tinalux_scaffold_class_name
            "${_tinalux_scaffold_markup_file}")
        _tinalux_markup_default_include_guard(
            _tinalux_scaffold_include_guard
            "${_tinalux_scaffold_output_header}")

        set(_tinalux_scaffold_command
            "$<TARGET_FILE:TinaluxMarkupActionHeaderTool>"
            --input "${_tinalux_scaffold_dir_input_abs}/${_tinalux_scaffold_markup_file}"
            --namespace "${_tinalux_scaffold_namespace}"
            --scaffold-output "${_tinalux_scaffold_output_header}"
            --scaffold-markup-header "${_tinalux_scaffold_markup_header_include}"
            --scaffold-class "${_tinalux_scaffold_class_name}"
            --scaffold-include-guard "${_tinalux_scaffold_include_guard}")
        if(TINALUX_MARKUP_SCAFFOLD_DIR_ONLY_IF_MISSING)
            list(APPEND _tinalux_scaffold_command --scaffold-only-if-missing)
        endif()
        if(TINALUX_MARKUP_SCAFFOLD_DIR_NO_PRAGMA_ONCE)
            list(APPEND _tinalux_scaffold_command --no-pragma-once)
        endif()

        add_custom_command(
            OUTPUT "${_tinalux_scaffold_output_header}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${_tinalux_scaffold_output_header_dir}"
            COMMAND ${_tinalux_scaffold_command}
            DEPENDS
                "${_tinalux_scaffold_dir_input_abs}/${_tinalux_scaffold_markup_file}"
                TinaluxMarkupActionHeaderTool
            COMMENT "Generating markup page scaffold: ${_tinalux_scaffold_output_header}"
            VERBATIM
        )

        if(TINALUX_MARKUP_SCAFFOLD_DIR_TARGET)
            string(SHA1 _tinalux_scaffold_hash "${_tinalux_scaffold_output_header}")
            string(MAKE_C_IDENTIFIER
                "tinalux_markup_page_scaffold_${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}_${_tinalux_scaffold_hash}"
                _tinalux_scaffold_target)
            add_custom_target("${_tinalux_scaffold_target}" DEPENDS "${_tinalux_scaffold_output_header}")
            add_dependencies("${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}" "${_tinalux_scaffold_target}")
            target_sources("${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}" PRIVATE "${_tinalux_scaffold_output_header}")
        endif()

        list(APPEND _tinalux_scaffold_index_includes "${_tinalux_scaffold_relative_header}")
    endforeach()

    if(TINALUX_MARKUP_SCAFFOLD_DIR_TARGET)
        target_include_directories(
            "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}"
            PRIVATE
                "${_tinalux_scaffold_dir_output_abs}")
    endif()

    if(_tinalux_scaffold_index_header)
        if(IS_ABSOLUTE "${_tinalux_scaffold_index_header}")
            set(_tinalux_scaffold_index_header_path "${_tinalux_scaffold_index_header}")
        else()
            set(_tinalux_scaffold_index_header_path
                "${_tinalux_scaffold_dir_output_abs}/${_tinalux_scaffold_index_header}")
        endif()

        get_filename_component(
            _tinalux_scaffold_index_header_dir
            "${_tinalux_scaffold_index_header_path}"
            DIRECTORY)
        file(MAKE_DIRECTORY "${_tinalux_scaffold_index_header_dir}")
        file(WRITE "${_tinalux_scaffold_index_header_path}" "#pragma once\n\n")
        foreach(_tinalux_scaffold_include IN LISTS _tinalux_scaffold_index_includes)
            file(APPEND
                "${_tinalux_scaffold_index_header_path}"
                "#include \"${_tinalux_scaffold_include}\"\n")
        endforeach()

        if(TINALUX_MARKUP_SCAFFOLD_DIR_TARGET)
            target_include_directories(
                "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}"
                PRIVATE
                    "${_tinalux_scaffold_index_header_dir}")
            target_sources(
                "${TINALUX_MARKUP_SCAFFOLD_DIR_TARGET}"
                PRIVATE
                    "${_tinalux_scaffold_index_header_path}")
        endif()
    endif()
endfunction()
