add_executable(TinaluxCoreSmoke
    core/version_smoke.cpp
)

target_link_libraries(TinaluxCoreSmoke
    PRIVATE
        Tinalux::Core
)

add_test(NAME TinaluxCoreSmoke COMMAND TinaluxCoreSmoke)

add_executable(TinaluxUILayoutSmoke
    ui/layout_smoke.cpp
)

target_link_libraries(TinaluxUILayoutSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUILayoutSmoke COMMAND TinaluxUILayoutSmoke)

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

add_executable(TinaluxUIScrollSmoke
    ui/scroll_smoke.cpp
)

target_link_libraries(TinaluxUIScrollSmoke
    PRIVATE
        Tinalux::App
)

add_test(NAME TinaluxUIScrollSmoke COMMAND TinaluxUIScrollSmoke)

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

add_executable(TinaluxUIAnimationSmoke
    ui/animation_smoke.cpp
)

target_link_libraries(TinaluxUIAnimationSmoke
    PRIVATE
        Tinalux::UI
)

add_test(NAME TinaluxUIAnimationSmoke COMMAND TinaluxUIAnimationSmoke)

add_executable(TinaluxUIParagraphClipboardSmoke
    ui/paragraph_clipboard_smoke.cpp
)

target_link_libraries(TinaluxUIParagraphClipboardSmoke
    PRIVATE
        Tinalux::App
)

tinalux_copy_skia_runtime_data(TinaluxUIParagraphClipboardSmoke)

add_test(NAME TinaluxUIParagraphClipboardSmoke COMMAND TinaluxUIParagraphClipboardSmoke)
