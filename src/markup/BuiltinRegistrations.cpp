#include "tinalux/markup/LayoutBuilder.h"

#include "tinalux/core/Reflect.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Flex.h"
#include "tinalux/ui/Grid.h"
#include "tinalux/ui/HBox.h"
#include "tinalux/ui/ImageWidget.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Toggle.h"
#include "tinalux/ui/VBox.h"

namespace tinalux::markup {

void registerBuiltinTypes()
{
    auto& reg = core::TypeRegistry::instance();

    // ---- Layout containers ----

    reg.registerType(core::TypeInfo {
        .name = "VBox",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::VBox>(); },
        .properties = {
            {"spacing", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::VBox&>(w).setSpacing(v.asNumber());
                }},
            {"padding", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::VBox&>(w).setPadding(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "HBox",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::HBox>(); },
        .properties = {
            {"spacing", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::HBox&>(w).setSpacing(v.asNumber());
                }},
            {"padding", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::HBox&>(w).setPadding(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Flex",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::Flex>(); },
        .properties = {
            {"direction", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& flex = static_cast<ui::Flex&>(w);
                    if (s == "Row") flex.setDirection(ui::FlexDirection::Row);
                    else if (s == "RowReverse") flex.setDirection(ui::FlexDirection::RowReverse);
                    else if (s == "Column") flex.setDirection(ui::FlexDirection::Column);
                    else if (s == "ColumnReverse") flex.setDirection(ui::FlexDirection::ColumnReverse);
                }},
            {"justifyContent", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& flex = static_cast<ui::Flex&>(w);
                    if (s == "Start") flex.setJustifyContent(ui::JustifyContent::Start);
                    else if (s == "Center") flex.setJustifyContent(ui::JustifyContent::Center);
                    else if (s == "End") flex.setJustifyContent(ui::JustifyContent::End);
                    else if (s == "SpaceBetween") flex.setJustifyContent(ui::JustifyContent::SpaceBetween);
                    else if (s == "SpaceAround") flex.setJustifyContent(ui::JustifyContent::SpaceAround);
                }},
            {"alignItems", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& flex = static_cast<ui::Flex&>(w);
                    if (s == "Start") flex.setAlignItems(ui::AlignItems::Start);
                    else if (s == "Center") flex.setAlignItems(ui::AlignItems::Center);
                    else if (s == "End") flex.setAlignItems(ui::AlignItems::End);
                    else if (s == "Stretch") flex.setAlignItems(ui::AlignItems::Stretch);
                }},
            {"wrap", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& flex = static_cast<ui::Flex&>(w);
                    if (s == "NoWrap") flex.setWrap(ui::FlexWrap::NoWrap);
                    else if (s == "Wrap") flex.setWrap(ui::FlexWrap::Wrap);
                }},
            {"spacing", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Flex&>(w).setSpacing(v.asNumber());
                }},
            {"padding", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Flex&>(w).setPadding(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Grid",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::Grid>(); },
        .properties = {
            {"columns", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Grid&>(w).setColumns(v.asInt());
                }},
            {"rows", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Grid&>(w).setRows(v.asInt());
                }},
            {"columnGap", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Grid&>(w).setColumnGap(v.asNumber());
                }},
            {"rowGap", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Grid&>(w).setRowGap(v.asNumber());
                }},
            {"padding", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Grid&>(w).setPadding(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Container",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::Container>(); },
        .properties = {},
    });

    reg.registerType(core::TypeInfo {
        .name = "Panel",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::Panel>(); },
        .properties = {
            {"backgroundColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Panel&>(w).setBackgroundColor(v.asColor());
                }},
            {"cornerRadius", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Panel&>(w).setCornerRadius(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Dialog",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::Dialog>(); },
        .properties = {
            {"title", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setTitle(v.asString());
                }},
            {"dismissOnBackdrop", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setDismissOnBackdrop(v.asBool());
                }},
            {"dismissOnEscape", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setDismissOnEscape(v.asBool());
                }},
            {"showCloseButton", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setShowCloseButton(v.asBool());
                }},
            {"backdropColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setBackdropColor(v.asColor());
                }},
            {"backgroundColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setBackgroundColor(v.asColor());
                }},
            {"cornerRadius", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setCornerRadius(v.asNumber());
                }},
            {"padding", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setPadding(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "ScrollView",
        .isContainer = true,
        .factory = [] { return std::make_shared<ui::ScrollView>(); },
        .properties = {
            {"preferredHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ScrollView&>(w).setPreferredHeight(v.asNumber());
                }},
        },
    });

    // ---- Leaf widgets ----

    reg.registerType(core::TypeInfo {
        .name = "Label",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Label>(""); },
        .properties = {
            {"text", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Label&>(w).setText(v.asString());
                }},
            {"fontSize", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Label&>(w).setFontSize(v.asNumber());
                }},
            {"color", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Label&>(w).setColor(v.asColor());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "ParagraphLabel",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::ParagraphLabel>(""); },
        .properties = {
            {"text", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ParagraphLabel&>(w).setText(v.asString());
                }},
            {"fontSize", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ParagraphLabel&>(w).setFontSize(v.asNumber());
                }},
            {"color", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ParagraphLabel&>(w).setColor(v.asColor());
                }},
            {"maxLines", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ParagraphLabel&>(w).setMaxLines(
                        static_cast<std::size_t>(v.asInt()));
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Button",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Button>(""); },
        .properties = {
            {"text", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Button&>(w).setLabel(v.asString());
                }},
            {"icon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Button&>(w).loadIconAsync(v.asString());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "TextInput",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::TextInput>(""); },
        .properties = {
            {"placeholder", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).setPlaceholder(v.asString());
                }},
            {"text", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).setText(v.asString());
                }},
            {"obscured", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).setObscured(v.asBool());
                }},
            {"leadingIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).loadLeadingIconAsync(v.asString());
                }},
            {"trailingIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).loadTrailingIconAsync(v.asString());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Dropdown",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Dropdown>(); },
        .properties = {
            {"placeholder", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).setPlaceholder(v.asString());
                }},
            {"maxVisibleItems", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).setMaxVisibleItems(v.asInt());
                }},
            {"selectedIndex", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).setSelectedIndex(v.asInt());
                }},
            {"indicatorIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).loadIndicatorIconAsync(v.asString());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Checkbox",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Checkbox>("", false); },
        .properties = {
            {"label", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Checkbox&>(w).setLabel(v.asString());
                }},
            {"checked", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Checkbox&>(w).setChecked(v.asBool());
                }},
            {"checkmarkIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Checkbox&>(w).loadCheckmarkIconAsync(v.asString());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Radio",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Radio>(""); },
        .properties = {
            {"label", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).setLabel(v.asString());
                }},
            {"group", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).setGroup(v.asString());
                }},
            {"selected", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).setSelected(v.asBool());
                }},
            {"selectionIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).loadSelectionIconAsync(v.asString());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "ImageWidget",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::ImageWidget>(); },
        .properties = {
            {"source", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ImageWidget&>(w).loadImageAsync(v.asString());
                }},
            {"fit", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& image = static_cast<ui::ImageWidget&>(w);
                    if (s == "None") image.setFit(ui::ImageFit::None);
                    else if (s == "Fill") image.setFit(ui::ImageFit::Fill);
                    else if (s == "Contain") image.setFit(ui::ImageFit::Contain);
                    else if (s == "Cover") image.setFit(ui::ImageFit::Cover);
                }},
            {"opacity", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ImageWidget&>(w).setOpacity(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Toggle",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Toggle>(""); },
        .properties = {
            {"label", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Toggle&>(w).setLabel(v.asString());
                }},
            {"on", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Toggle&>(w).setOn(v.asBool());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "Slider",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::Slider>(); },
        .properties = {
            {"value", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Slider&>(w).setValue(v.asNumber());
                }},
            {"min", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& slider = static_cast<ui::Slider&>(w);
                    slider.setRange(v.asNumber(), slider.maximum());
                }},
            {"max", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& slider = static_cast<ui::Slider&>(w);
                    slider.setRange(slider.minimum(), v.asNumber());
                }},
            {"step", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Slider&>(w).setStep(v.asNumber());
                }},
        },
    });

    reg.registerType(core::TypeInfo {
        .name = "ProgressBar",
        .isContainer = false,
        .factory = [] { return std::make_shared<ui::ProgressBar>(); },
        .properties = {
            {"value", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setValue(v.asNumber());
                }},
            {"indeterminate", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setIndeterminate(v.asBool());
                }},
            {"color", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setColor(v.asColor());
                }},
            {"backgroundColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setBackgroundColor(v.asColor());
                }},
            {"height", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setHeight(v.asNumber());
                }},
        },
    });
}

} // namespace tinalux::markup
