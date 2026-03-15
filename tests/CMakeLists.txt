add_executable(TinaluxCoreSmoke
    core/version_smoke.cpp
)

target_link_libraries(TinaluxCoreSmoke
    PRIVATE
        Tinalux::Core
)

add_test(NAME TinaluxCoreSmoke COMMAND TinaluxCoreSmoke)

add_executable(TinaluxCoreLogSmoke
    core/log_smoke.cpp
)

target_link_libraries(TinaluxCoreLogSmoke
    PRIVATE
        Tinalux::Core
)

add_test(NAME TinaluxCoreLogSmoke COMMAND TinaluxCoreLogSmoke)

add_executable(TinaluxUILayoutSmoke
    ui/layout_smoke.cpp
)

target_link_libraries(TinaluxUILayoutSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUILayoutSmoke COMMAND TinaluxUILayoutSmoke)

add_executable(TinaluxUIFlexLayoutSmoke
    ui/flex_layout_smoke.cpp
)

target_link_libraries(TinaluxUIFlexLayoutSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIFlexLayoutSmoke COMMAND TinaluxUIFlexLayoutSmoke)

add_executable(TinaluxUIGridLayoutSmoke
    ui/grid_layout_smoke.cpp
)

target_link_libraries(TinaluxUIGridLayoutSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIGridLayoutSmoke COMMAND TinaluxUIGridLayoutSmoke)

add_executable(TinaluxUIResponsiveLayoutSmoke
    ui/responsive_layout_smoke.cpp
)

target_link_libraries(TinaluxUIResponsiveLayoutSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIResponsiveLayoutSmoke COMMAND TinaluxUIResponsiveLayoutSmoke)

add_executable(TinaluxUIEventSmoke
    ui/event_smoke.cpp
)

target_link_libraries(TinaluxUIEventSmoke
    PRIVATE
        Tinalux::App
)

add_test(NAME TinaluxUIEventSmoke COMMAND TinaluxUIEventSmoke)

add_executable(TinaluxUITextInputSmoke
    ui/text_input_smoke.cpp
)

target_link_libraries(TinaluxUITextInputSmoke
    PRIVATE
        Tinalux::App
)

add_test(NAME TinaluxUITextInputSmoke COMMAND TinaluxUITextInputSmoke)

add_executable(TinaluxUITextInputStyleSmoke
    ui/text_input_style_smoke.cpp
)

target_link_libraries(TinaluxUITextInputStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUITextInputStyleSmoke COMMAND TinaluxUITextInputStyleSmoke)

add_executable(TinaluxUITextInputIconSmoke
    ui/text_input_icon_smoke.cpp
)

target_link_libraries(TinaluxUITextInputIconSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUITextInputIconSmoke COMMAND TinaluxUITextInputIconSmoke)

add_executable(TinaluxUIDemoSceneSmoke
    ui/demo_scene_smoke.cpp
)

target_link_libraries(TinaluxUIDemoSceneSmoke
    PRIVATE
        Tinalux::UI
)

tinalux_copy_skia_runtime_data(TinaluxUIDemoSceneSmoke)

add_test(NAME TinaluxUIDemoSceneSmoke COMMAND TinaluxUIDemoSceneSmoke)

add_executable(TinaluxUITextPrimitivesSmoke
    ui/text_primitives_smoke.cpp
)

target_link_libraries(TinaluxUITextPrimitivesSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUITextPrimitivesSmoke COMMAND TinaluxUITextPrimitivesSmoke)

add_executable(TinaluxUIScrollSmoke
    ui/scroll_smoke.cpp
)

target_link_libraries(TinaluxUIScrollSmoke
    PRIVATE
        Tinalux::App
)

add_test(NAME TinaluxUIScrollSmoke COMMAND TinaluxUIScrollSmoke)

add_executable(TinaluxUIScrollViewStyleSmoke
    ui/scroll_view_style_smoke.cpp
)

target_link_libraries(TinaluxUIScrollViewStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIScrollViewStyleSmoke COMMAND TinaluxUIScrollViewStyleSmoke)

add_executable(TinaluxUIListViewStyleSmoke
    ui/list_view_style_smoke.cpp
)

target_link_libraries(TinaluxUIListViewStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIListViewStyleSmoke COMMAND TinaluxUIListViewStyleSmoke)

add_executable(TinaluxUIListViewSelectionSmoke
    ui/list_view_selection_smoke.cpp
)

target_link_libraries(TinaluxUIListViewSelectionSmoke
    PRIVATE
        Tinalux::UI
        Tinalux::App
)

add_test(NAME TinaluxUIListViewSelectionSmoke COMMAND TinaluxUIListViewSelectionSmoke)

add_executable(TinaluxUIRenderCacheSmoke
    ui/render_cache_smoke.cpp
)

target_link_libraries(TinaluxUIRenderCacheSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIRenderCacheSmoke COMMAND TinaluxUIRenderCacheSmoke)

add_executable(TinaluxUIPanelStyleSmoke
    ui/panel_style_smoke.cpp
)

target_link_libraries(TinaluxUIPanelStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIPanelStyleSmoke COMMAND TinaluxUIPanelStyleSmoke)

add_executable(TinaluxUIControlsSmoke
    ui/controls_smoke.cpp
)

target_link_libraries(TinaluxUIControlsSmoke
    PRIVATE
        Tinalux::UI
)

if(MSVC)
    target_link_options(TinaluxUIControlsSmoke PRIVATE /DEBUG:FASTLINK /INCREMENTAL:NO /CGTHREADS:1)
endif()

add_test(NAME TinaluxUIControlsSmoke COMMAND TinaluxUIControlsSmoke)

add_executable(TinaluxUIDropdownSmoke
    ui/dropdown_smoke.cpp
)

target_link_libraries(TinaluxUIDropdownSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIDropdownSmoke COMMAND TinaluxUIDropdownSmoke)

add_executable(TinaluxUISelectionControlStyleSmoke
    ui/selection_control_style_smoke.cpp
)

target_link_libraries(TinaluxUISelectionControlStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUISelectionControlStyleSmoke COMMAND TinaluxUISelectionControlStyleSmoke)

add_executable(TinaluxUISliderStyleSmoke
    ui/slider_style_smoke.cpp
)

target_link_libraries(TinaluxUISliderStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUISliderStyleSmoke COMMAND TinaluxUISliderStyleSmoke)

add_executable(TinaluxUIButtonStyleSmoke
    ui/button_style_smoke.cpp
)

target_link_libraries(TinaluxUIButtonStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIButtonStyleSmoke COMMAND TinaluxUIButtonStyleSmoke)

add_executable(TinaluxUIAnimationSmoke
    ui/animation_smoke.cpp
)

target_link_libraries(TinaluxUIAnimationSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIAnimationSmoke COMMAND TinaluxUIAnimationSmoke)

add_executable(TinaluxUIThemeManagerSmoke
    ui/theme_manager_smoke.cpp
)

target_link_libraries(TinaluxUIThemeManagerSmoke
    PRIVATE
        Tinalux::App
)

add_test(NAME TinaluxUIThemeManagerSmoke COMMAND TinaluxUIThemeManagerSmoke)

add_executable(TinaluxUIDialogSmoke
    ui/dialog_smoke.cpp
)

target_link_libraries(TinaluxUIDialogSmoke
    PRIVATE
        Tinalux::App
)

add_test(NAME TinaluxUIDialogSmoke COMMAND TinaluxUIDialogSmoke)

add_executable(TinaluxUIDialogStyleSmoke
    ui/dialog_style_smoke.cpp
)

target_link_libraries(TinaluxUIDialogStyleSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIDialogStyleSmoke COMMAND TinaluxUIDialogStyleSmoke)

add_executable(TinaluxUIImageSmoke
    ui/image_smoke.cpp
)

target_link_libraries(TinaluxUIImageSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIImageSmoke COMMAND TinaluxUIImageSmoke)

add_executable(TinaluxUIIconRegistrySmoke
    ui/icon_registry_smoke.cpp
)

target_link_libraries(TinaluxUIIconRegistrySmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIIconRegistrySmoke COMMAND TinaluxUIIconRegistrySmoke)

add_executable(TinaluxUIResourceManagerSmoke
    ui/resource_manager_smoke.cpp
)

target_link_libraries(TinaluxUIResourceManagerSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIResourceManagerSmoke COMMAND TinaluxUIResourceManagerSmoke)

add_executable(TinaluxUIResourceLoaderSmoke
    ui/resource_loader_smoke.cpp
)

target_link_libraries(TinaluxUIResourceLoaderSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIResourceLoaderSmoke COMMAND TinaluxUIResourceLoaderSmoke)

add_executable(TinaluxUIParagraphClipboardSmoke
    ui/paragraph_clipboard_smoke.cpp
)

target_link_libraries(TinaluxUIParagraphClipboardSmoke
    PRIVATE
        Tinalux::App
)

tinalux_copy_skia_runtime_data(TinaluxUIParagraphClipboardSmoke)

add_test(NAME TinaluxUIParagraphClipboardSmoke COMMAND TinaluxUIParagraphClipboardSmoke)
