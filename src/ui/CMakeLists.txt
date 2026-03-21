add_library(TinaluxUI STATIC
    Constraints.cpp
    Widget.cpp
    Container.cpp
    DemoScene.cpp
    showcase/ShowcaseSupport.cpp
    showcase/ShowcasePages.cpp
    showcase/FoundationPages.cpp
    showcase/InputPages.cpp
    showcase/ContentPages.cpp
    showcase/SystemPages.cpp
    IconRegistry.cpp
    ResourceManager.cpp
    ResourceLoader.cpp
    Theme.cpp
    RichTextStyle.cpp
    ButtonStyle.cpp
    DialogStyle.cpp
    ListViewStyle.cpp
    PanelStyle.cpp
    SelectionControlStyle.cpp
    ScrollViewStyle.cpp
    SliderStyle.cpp
    TextInputStyle.cpp
    ThemeManager.cpp
    Animation.cpp
    RuntimeState.cpp
    TextPrimitives.cpp
    TextMetricsBackend.cpp
    ParagraphTextLayout.cpp
    Clipboard.cpp
    layouts/VBoxLayout.cpp
    layouts/HBoxLayout.cpp
    layouts/FlexLayout.cpp
    layouts/GridLayout.cpp
    layouts/ResponsiveLayout.cpp
    widgets/Label.cpp
    widgets/ParagraphLabel.cpp
    widgets/RichText.cpp
    widgets/Button.cpp
    widgets/Checkbox.cpp
    widgets/Dialog.cpp
    widgets/Dropdown.cpp
    widgets/ImageWidget.cpp
    widgets/Panel.cpp
    widgets/ProgressBar.cpp
    widgets/Radio.cpp
    widgets/ScrollView.cpp
    widgets/Slider.cpp
    widgets/ListView.cpp
    widgets/text_input/TextInputLayoutCache.cpp
    widgets/text_input/TextInputModel.cpp
    widgets/TextInput.cpp
    widgets/Toggle.cpp
    widgets/VBox.cpp
    widgets/HBox.cpp
    widgets/Flex.cpp
    widgets/Grid.cpp
)

add_library(Tinalux::UI ALIAS TinaluxUI)

target_include_directories(TinaluxUI
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(TinaluxUI PRIVATE /MP1 /bigobj)
endif()

set(TINALUX_SKIA_STATIC_PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(TINALUX_SKIA_STATIC_SUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")

function(tinalux_skia_runtime_lib out_var config_name lib_name)
    set(${out_var}
        "${SKIA_BUILD_DIR}/${config_name}/${TINALUX_SKIA_STATIC_PREFIX}${lib_name}${TINALUX_SKIA_STATIC_SUFFIX}"
        PARENT_SCOPE
    )
endfunction()

tinalux_skia_runtime_lib(TINALUX_SKIA_DEBUG_SKPARAGRAPH Debug skparagraph)
tinalux_skia_runtime_lib(TINALUX_SKIA_DEBUG_SKSHAPER Debug skshaper)
tinalux_skia_runtime_lib(TINALUX_SKIA_DEBUG_SKUNICODE_CORE Debug skunicode_core)
tinalux_skia_runtime_lib(TINALUX_SKIA_DEBUG_SKUNICODE_ICU Debug skunicode_icu)
tinalux_skia_runtime_lib(TINALUX_SKIA_RELEASE_SKPARAGRAPH Release skparagraph)
tinalux_skia_runtime_lib(TINALUX_SKIA_RELEASE_SKSHAPER Release skshaper)
tinalux_skia_runtime_lib(TINALUX_SKIA_RELEASE_SKUNICODE_CORE Release skunicode_core)
tinalux_skia_runtime_lib(TINALUX_SKIA_RELEASE_SKUNICODE_ICU Release skunicode_icu)

if(CMAKE_CONFIGURATION_TYPES)
    target_link_libraries(TinaluxUI
        PUBLIC
            Tinalux::Core
            Tinalux::Rendering
            Skia::Skia
            "$<$<CONFIG:Debug>:${TINALUX_SKIA_DEBUG_SKPARAGRAPH}>"
            "$<$<CONFIG:Debug>:${TINALUX_SKIA_DEBUG_SKSHAPER}>"
            "$<$<CONFIG:Debug>:${TINALUX_SKIA_DEBUG_SKUNICODE_CORE}>"
            "$<$<CONFIG:Debug>:${TINALUX_SKIA_DEBUG_SKUNICODE_ICU}>"
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:${TINALUX_SKIA_RELEASE_SKPARAGRAPH}>"
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:${TINALUX_SKIA_RELEASE_SKSHAPER}>"
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:${TINALUX_SKIA_RELEASE_SKUNICODE_CORE}>"
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:${TINALUX_SKIA_RELEASE_SKUNICODE_ICU}>"
    )
else()
    target_link_libraries(TinaluxUI
        PUBLIC
            Tinalux::Core
            Tinalux::Rendering
            Skia::Skia
            "${SKIA_BUILD_DIR}/${SKIA_BUILD_TYPE}/${TINALUX_SKIA_STATIC_PREFIX}skparagraph${TINALUX_SKIA_STATIC_SUFFIX}"
            "${SKIA_BUILD_DIR}/${SKIA_BUILD_TYPE}/${TINALUX_SKIA_STATIC_PREFIX}skshaper${TINALUX_SKIA_STATIC_SUFFIX}"
            "${SKIA_BUILD_DIR}/${SKIA_BUILD_TYPE}/${TINALUX_SKIA_STATIC_PREFIX}skunicode_core${TINALUX_SKIA_STATIC_SUFFIX}"
            "${SKIA_BUILD_DIR}/${SKIA_BUILD_TYPE}/${TINALUX_SKIA_STATIC_PREFIX}skunicode_icu${TINALUX_SKIA_STATIC_SUFFIX}"
    )
endif()
