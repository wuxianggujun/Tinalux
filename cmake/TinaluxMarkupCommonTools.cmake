include_guard(GLOBAL)

# Markup CMake 内部共享 helper。
# 这里只放不会直接暴露给普通页面开发者的公共工具函数。

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

function(_tinalux_markup_default_page_scaffold_output_dir out_var target_name)
    string(MAKE_C_IDENTIFIER "${target_name}" _tinalux_target_id)
    if(_tinalux_target_id STREQUAL "")
        set(_tinalux_target_id "markup")
    endif()
    set(
        ${out_var}
        "${CMAKE_CURRENT_BINARY_DIR}/tinalux_markup_pages/${_tinalux_target_id}"
        PARENT_SCOPE)
endfunction()

function(_tinalux_markup_default_page_scaffold_index_header out_var target_name)
    string(MAKE_C_IDENTIFIER "${target_name}" _tinalux_target_id)
    if(_tinalux_target_id STREQUAL "")
        set(_tinalux_target_id "markup")
    endif()
    set(${out_var} "${_tinalux_target_id}.pages.h" PARENT_SCOPE)
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

function(_tinalux_markup_pascal_identifier out_var raw_segment)
    string(MAKE_C_IDENTIFIER "${raw_segment}" _tinalux_identifier)
    if(_tinalux_identifier STREQUAL "")
        set(_tinalux_identifier "layout")
    endif()

    string(REPLACE "_" ";" _tinalux_identifier_parts "${_tinalux_identifier}")
    set(_tinalux_pascal "")
    foreach(_tinalux_part IN LISTS _tinalux_identifier_parts)
        if(_tinalux_part STREQUAL "")
            continue()
        endif()

        string(SUBSTRING "${_tinalux_part}" 0 1 _tinalux_first)
        string(TOUPPER "${_tinalux_first}" _tinalux_first)
        string(LENGTH "${_tinalux_part}" _tinalux_part_length)
        if(_tinalux_part_length GREATER 1)
            math(EXPR _tinalux_rest_length "${_tinalux_part_length} - 1")
            string(SUBSTRING "${_tinalux_part}" 1 ${_tinalux_rest_length} _tinalux_rest)
        else()
            set(_tinalux_rest "")
        endif()

        string(APPEND _tinalux_pascal "${_tinalux_first}${_tinalux_rest}")
    endforeach()

    if(_tinalux_pascal STREQUAL "")
        set(_tinalux_pascal "Layout")
    endif()
    if(_tinalux_pascal MATCHES "^[0-9]")
        set(_tinalux_pascal "Generated${_tinalux_pascal}")
    endif()
    set(${out_var} "${_tinalux_pascal}" PARENT_SCOPE)
endfunction()

function(_tinalux_markup_scaffold_class_name_for_relative_path out_var relative_path)
    set(_tinalux_class_name)

    cmake_path(GET relative_path PARENT_PATH _tinalux_parent_path)
    if(_tinalux_parent_path)
        string(REPLACE "/" ";" _tinalux_parent_segments "${_tinalux_parent_path}")
        foreach(_tinalux_segment IN LISTS _tinalux_parent_segments)
            _tinalux_markup_pascal_identifier(
                _tinalux_segment_class_name
                "${_tinalux_segment}")
            string(APPEND _tinalux_class_name "${_tinalux_segment_class_name}")
        endforeach()
    endif()

    cmake_path(GET relative_path STEM _tinalux_stem)
    _tinalux_markup_pascal_identifier(_tinalux_stem_class_name "${_tinalux_stem}")
    string(APPEND _tinalux_class_name "${_tinalux_stem_class_name}Page")
    set(${out_var} "${_tinalux_class_name}" PARENT_SCOPE)
endfunction()
