include(CheckIncludeFileCXX)

check_include_file_cxx("vulkan/vulkan_core.h" TINALUX_TEST_HAS_VULKAN_HEADERS)
find_program(TINALUX_POWERSHELL_EXECUTABLE NAMES pwsh pwsh.exe powershell powershell.exe)

function(tinalux_add_smoke_test target_name)
    set(options COPY_SKIA_DATA)
    set(oneValueArgs SOURCE)
    set(multiValueArgs LINK_LIBS)
    cmake_parse_arguments(TINALUX_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TINALUX_TEST_SOURCE)
        message(FATAL_ERROR "tinalux_add_smoke_test(${target_name}) requires SOURCE")
    endif()
    if(NOT TINALUX_TEST_LINK_LIBS)
        message(FATAL_ERROR "tinalux_add_smoke_test(${target_name}) requires LINK_LIBS")
    endif()

    add_executable(${target_name}
        ${TINALUX_TEST_SOURCE}
    )

    target_link_libraries(${target_name}
        PRIVATE
            ${TINALUX_TEST_LINK_LIBS}
    )

    if(TINALUX_TEST_COPY_SKIA_DATA)
        tinalux_copy_skia_runtime_data(${target_name})
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name})
    set_property(GLOBAL APPEND PROPERTY TINALUX_SMOKE_TEST_TARGETS ${target_name})
endfunction()

tinalux_add_smoke_test(TinaluxCoreSmoke
    SOURCE core/version_smoke.cpp
    LINK_LIBS Tinalux::Core
)

tinalux_add_smoke_test(TinaluxCoreLogSmoke
    SOURCE core/log_smoke.cpp
    LINK_LIBS Tinalux::Core
)

tinalux_add_smoke_test(TinaluxCoreRenderBackendSmoke
    SOURCE core/render_backend_smoke.cpp
    LINK_LIBS Tinalux::Rendering
)
target_include_directories(TinaluxCoreRenderBackendSmoke PRIVATE
    ${TINALUX_GLFW_SOURCE_DIR}/deps
)

if(TINALUX_TEST_HAS_VULKAN_HEADERS)
    tinalux_add_smoke_test(TinaluxCoreFramePrepareSmoke
        SOURCE core/frame_prepare_smoke.cpp
        LINK_LIBS Tinalux::Rendering
    )
else()
    message(STATUS "Skipping TinaluxCoreFramePrepareSmoke because Vulkan headers are unavailable")
endif()

tinalux_add_smoke_test(TinaluxCoreMetalContextSmoke
    SOURCE core/metal_context_smoke.cpp
    LINK_LIBS Tinalux::Rendering
)

tinalux_add_smoke_test(TinaluxCoreMetalSurfaceSmoke
    SOURCE core/metal_surface_smoke.cpp
    LINK_LIBS Tinalux::Rendering
)

tinalux_add_smoke_test(TinaluxAppBackendPlanSmoke
    SOURCE app/backend_plan_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppBackendFallbackSmoke
    SOURCE app/application_backend_fallback_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppClipboardBindingSmoke
    SOURCE app/application_clipboard_binding_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppSkiaRuntimeBindingSmoke
    SOURCE app/application_skia_runtime_binding_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppResourceManagerBindingSmoke
    SOURCE app/application_resource_manager_binding_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppAndroidRuntimeLifecycleSmoke
    SOURCE app/android_runtime_lifecycle_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppDpiSmoke
    SOURCE app/application_dpi_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxAppLoopTimingSmoke
    SOURCE app/application_loop_timing_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUILayoutSmoke
    SOURCE ui/layout_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIFlexLayoutSmoke
    SOURCE ui/flex_layout_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIGridLayoutSmoke
    SOURCE ui/grid_layout_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIResponsiveLayoutSmoke
    SOURCE ui/responsive_layout_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIEventSmoke
    SOURCE ui/event_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIDpiSmoke
    SOURCE ui/dpi_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIContextLifecycleSmoke
    SOURCE ui/ui_context_lifecycle_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIContextResourceLoaderIsolationSmoke
    SOURCE ui/ui_context_resource_loader_isolation_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUITextInputSmoke
    SOURCE ui/text_input_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUITextInputModelSmoke
    SOURCE ui/text_input_model_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUITextInputStyleSmoke
    SOURCE ui/text_input_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUITextInputIconSmoke
    SOURCE ui/text_input_icon_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIDemoSceneSmoke
    SOURCE ui/demo_scene_smoke.cpp
    LINK_LIBS Tinalux::UI
    COPY_SKIA_DATA
)

tinalux_add_smoke_test(TinaluxUITextPrimitivesSmoke
    SOURCE ui/text_primitives_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIRichTextSmoke
    SOURCE ui/rich_text_smoke.cpp
    LINK_LIBS Tinalux::UI
    COPY_SKIA_DATA
)

tinalux_add_smoke_test(TinaluxUIScrollSmoke
    SOURCE ui/scroll_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIScrollViewStyleSmoke
    SOURCE ui/scroll_view_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIListViewStyleSmoke
    SOURCE ui/list_view_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIListViewSelectionSmoke
    SOURCE ui/list_view_selection_smoke.cpp
    LINK_LIBS Tinalux::UI Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIListViewVirtualizationSmoke
    SOURCE ui/list_view_virtualization_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIRenderCacheSmoke
    SOURCE ui/render_cache_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIDirtySubtreeOverlapSmoke
    SOURCE ui/dirty_subtree_overlap_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIDirtySubtreePruneSmoke
    SOURCE ui/dirty_subtree_prune_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIDirtySubtreeIslandsSmoke
    SOURCE ui/dirty_subtree_islands_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIDirtySubtreeFullRedrawFallbackSmoke
    SOURCE ui/dirty_subtree_full_redraw_fallback_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIPanelStyleSmoke
    SOURCE ui/panel_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIControlsSmoke
    SOURCE ui/controls_smoke.cpp
    LINK_LIBS Tinalux::UI
)

if(MSVC)
    target_link_options(TinaluxUIControlsSmoke PRIVATE /DEBUG:FASTLINK /INCREMENTAL:NO /CGTHREADS:1)
endif()

tinalux_add_smoke_test(TinaluxUIDropdownSmoke
    SOURCE ui/dropdown_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUISelectionControlStyleSmoke
    SOURCE ui/selection_control_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUISliderStyleSmoke
    SOURCE ui/slider_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIButtonStyleSmoke
    SOURCE ui/button_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIAnimationSmoke
    SOURCE ui/animation_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIThemeManagerSmoke
    SOURCE ui/theme_manager_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIDialogSmoke
    SOURCE ui/dialog_smoke.cpp
    LINK_LIBS Tinalux::App
)

tinalux_add_smoke_test(TinaluxUIDialogStyleSmoke
    SOURCE ui/dialog_style_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIImageSmoke
    SOURCE ui/image_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIIconRegistrySmoke
    SOURCE ui/icon_registry_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIResourceManagerSmoke
    SOURCE ui/resource_manager_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIResourceLoaderSmoke
    SOURCE ui/resource_loader_smoke.cpp
    LINK_LIBS Tinalux::UI
)

tinalux_add_smoke_test(TinaluxUIParagraphClipboardSmoke
    SOURCE ui/paragraph_clipboard_smoke.cpp
    LINK_LIBS Tinalux::App
    COPY_SKIA_DATA
)

if(TINALUX_POWERSHELL_EXECUTABLE)
    add_test(NAME TinaluxAndroidStageScriptSmoke
        COMMAND
            ${TINALUX_POWERSHELL_EXECUTABLE}
            -NoProfile
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/android_stage_script_smoke.ps1
            -RepoRoot
            ${CMAKE_SOURCE_DIR}
    )

    add_test(NAME TinaluxAndroidBuildValidateScriptSmoke
        COMMAND
            ${TINALUX_POWERSHELL_EXECUTABLE}
            -NoProfile
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/android_build_validate_smoke.ps1
            -RepoRoot
            ${CMAKE_SOURCE_DIR}
    )
else()
    message(STATUS "Skipping Android PowerShell smoke tests because no PowerShell executable was found")
endif()

get_property(TINALUX_SMOKE_TEST_TARGETS GLOBAL PROPERTY TINALUX_SMOKE_TEST_TARGETS)

if(TINALUX_SMOKE_TEST_TARGETS)
    add_custom_target(TinaluxBuildSmokeTests
        DEPENDS
            ${TINALUX_SMOKE_TEST_TARGETS}
    )

    set(_tinalux_ctest_config_args)
    if(CMAKE_CONFIGURATION_TYPES)
        list(APPEND _tinalux_ctest_config_args -C $<CONFIG>)
    elseif(CMAKE_BUILD_TYPE)
        list(APPEND _tinalux_ctest_config_args -C ${CMAKE_BUILD_TYPE})
    endif()

    add_custom_target(TinaluxRunSmokeTests
        COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure --timeout 60 ${_tinalux_ctest_config_args}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        DEPENDS
            TinaluxBuildSmokeTests
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
    )
endif()
