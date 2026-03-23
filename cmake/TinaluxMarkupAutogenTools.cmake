include_guard(GLOBAL)
include(TinaluxMarkupCommonTools)
include(TinaluxMarkupBindingsTools)
include(TinaluxMarkupScaffoldTools)

# 高层 autogen / executable 入口实现。
# 正常页面开发真正常用的只有两个入口：
# - tinalux_target_enable_markup_autogen(...)
# - tinalux_add_markup_executable(...)
#
# 如果你只是想知道“平时怎么写页面”，先去看：
# - `TinaluxMarkupTools.cmake`
# - `samples/markup/README.md`
#
# 这个文件是那两个高层入口的实现，不是额外的一层用户 API。
# 看到这里时如果已经被 `NAMESPACE / INDEX_HEADER / PAGE_SCAFFOLD_*` 绕晕，
# 最好先退回模板区，按单文件 / 目录扫描 / scaffold 四种起手式选一个抄。

function(tinalux_target_enable_markup_autogen)
    set(options PAGE_SCAFFOLD_ONLY_IF_MISSING)
    set(oneValueArgs
        TARGET
        INPUT
        DIRECTORY
        NAMESPACE
        NAMESPACE_PREFIX
        OUTPUT_DIRECTORY
        INDEX_HEADER
        OUTPUT_HEADER
        PAGE_SCAFFOLD_OUTPUT
        PAGE_SCAFFOLD_CLASS_NAME
        PAGE_SCAFFOLD_OUTPUT_DIRECTORY
        PAGE_SCAFFOLD_INDEX_HEADER)
    cmake_parse_arguments(TINALUX_MARKUP_AUTOGEN "${options}" "${oneValueArgs}" "" ${ARGN})

    # 第一段：先做参数合法性校验，尽早把“同时传了互斥参数”这类问题拦掉。
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
    if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT
       AND TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT_DIRECTORY)
        message(FATAL_ERROR
            "tinalux_target_enable_markup_autogen accepts PAGE_SCAFFOLD_OUTPUT or "
            "PAGE_SCAFFOLD_OUTPUT_DIRECTORY, not both")
    endif()

    # 第二段：把输出目录 / 总索引头补成稳定默认值。
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

    # 第三段：目录模式。
    # 这条分支会批量扫描 `ui/**/*.tui`，生成整目录 bindings，
    # 可选顺手再批量起 page scaffold。
    if(TINALUX_MARKUP_AUTOGEN_DIRECTORY)
        if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT
           OR TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_CLASS_NAME)
            message(FATAL_ERROR
                "directory markup autogen does not support PAGE_SCAFFOLD_OUTPUT or "
                "PAGE_SCAFFOLD_CLASS_NAME; use PAGE_SCAFFOLD_OUTPUT_DIRECTORY instead")
        endif()

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
            PROPERTY TINALUX_MARKUP_AUTOGEN_DIRECTORY
            "${TINALUX_MARKUP_AUTOGEN_DIRECTORY}")
        set_property(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PROPERTY TINALUX_MARKUP_AUTOGEN_NAMESPACE_PREFIX
            "${TINALUX_MARKUP_AUTOGEN_NAMESPACE_PREFIX}")
        set_property(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PROPERTY TINALUX_MARKUP_AUTOGEN_OUTPUT_DIRECTORY
            "${_tinalux_autogen_output_dir_abs}")
        set_property(
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            PROPERTY TINALUX_MARKUP_AUTOGEN_INDEX_HEADER
            "${_tinalux_autogen_index_header_abs}")

        if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT_DIRECTORY
           OR TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_INDEX_HEADER
           OR TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_ONLY_IF_MISSING)
            set(_tinalux_page_scaffold_dir_args
                TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}")
            if(NOT "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT_DIRECTORY}" STREQUAL "")
                list(APPEND
                    _tinalux_page_scaffold_dir_args
                    OUTPUT_DIRECTORY
                    "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT_DIRECTORY}")
            endif()
            if(NOT "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_INDEX_HEADER}" STREQUAL "")
                list(APPEND
                    _tinalux_page_scaffold_dir_args
                    INDEX_HEADER
                    "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_INDEX_HEADER}")
            endif()
            if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_ONLY_IF_MISSING)
                list(APPEND _tinalux_page_scaffold_dir_args ONLY_IF_MISSING)
            endif()
            tinalux_generate_markup_page_scaffolds_for_directory(
                ${_tinalux_page_scaffold_dir_args})
        endif()
        return()
    endif()

    # 第四段：单文件模式。
    # 这条分支只处理一个 `.tui`，命名空间和输出头都可以自动推导。
    if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT_DIRECTORY
       OR TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_INDEX_HEADER)
        message(FATAL_ERROR
            "single-file markup autogen does not support PAGE_SCAFFOLD_OUTPUT_DIRECTORY or "
            "PAGE_SCAFFOLD_INDEX_HEADER; use PAGE_SCAFFOLD_OUTPUT instead")
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

    # 第五段：如果用户顺手要页面类骨架，就在同一个高层入口里一起生成。
    if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT
       OR TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_CLASS_NAME
       OR TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_ONLY_IF_MISSING)
        if(NOT TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT)
            message(FATAL_ERROR
                "single-file page scaffold generation requires PAGE_SCAFFOLD_OUTPUT")
        endif()

        set(_tinalux_page_scaffold_args
            TARGET "${TINALUX_MARKUP_AUTOGEN_TARGET}"
            OUTPUT "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_OUTPUT}")
        if(NOT "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_CLASS_NAME}" STREQUAL "")
            list(APPEND
                _tinalux_page_scaffold_args
                CLASS_NAME
                "${TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_CLASS_NAME}")
        endif()
        if(TINALUX_MARKUP_AUTOGEN_PAGE_SCAFFOLD_ONLY_IF_MISSING)
            list(APPEND _tinalux_page_scaffold_args ONLY_IF_MISSING)
        endif()
        tinalux_generate_markup_page_scaffold(${_tinalux_page_scaffold_args})
    endif()
endfunction()

function(tinalux_add_markup_executable)
    set(options COPY_SKIA_DATA PAGE_SCAFFOLD_ONLY_IF_MISSING)
    set(oneValueArgs
        TARGET
        SOURCE
        INPUT
        DIRECTORY
        NAMESPACE
        NAMESPACE_PREFIX
        OUTPUT_DIRECTORY
        INDEX_HEADER
        OUTPUT_HEADER
        PAGE_SCAFFOLD_OUTPUT
        PAGE_SCAFFOLD_CLASS_NAME
        PAGE_SCAFFOLD_OUTPUT_DIRECTORY
        PAGE_SCAFFOLD_INDEX_HEADER)
    set(multiValueArgs SOURCES LINK_LIBS)
    cmake_parse_arguments(TINALUX_MARKUP_EXE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 这是更高一层的便捷包装：
    # 1. 先 add_executable(...)
    # 2. 默认链接 Tinalux::Markup
    # 3. 再把其余参数原样转发给 autogen 入口
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

    # 把高层 helper 自己消费掉的参数留在本层，
    # 其余 autogen 参数尽量原样平铺转发，减少心智分叉。
    set(_tinalux_markup_exe_autogen_args
        TARGET "${TINALUX_MARKUP_EXE_TARGET}")
    foreach(_tinalux_markup_exe_arg
        INPUT
        DIRECTORY
        NAMESPACE
        NAMESPACE_PREFIX
        OUTPUT_DIRECTORY
        INDEX_HEADER
        OUTPUT_HEADER
        PAGE_SCAFFOLD_OUTPUT
        PAGE_SCAFFOLD_CLASS_NAME
        PAGE_SCAFFOLD_OUTPUT_DIRECTORY
        PAGE_SCAFFOLD_INDEX_HEADER)
        if(NOT "${TINALUX_MARKUP_EXE_${_tinalux_markup_exe_arg}}" STREQUAL "")
            list(APPEND
                _tinalux_markup_exe_autogen_args
                ${_tinalux_markup_exe_arg}
                "${TINALUX_MARKUP_EXE_${_tinalux_markup_exe_arg}}")
        endif()
    endforeach()
    if(TINALUX_MARKUP_EXE_PAGE_SCAFFOLD_ONLY_IF_MISSING)
        list(APPEND _tinalux_markup_exe_autogen_args PAGE_SCAFFOLD_ONLY_IF_MISSING)
    endif()

    tinalux_target_enable_markup_autogen(${_tinalux_markup_exe_autogen_args})
endfunction()
