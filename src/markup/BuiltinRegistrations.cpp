#include "tinalux/markup/LayoutBuilder.h"

#include <optional>
#include <string_view>

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
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/RichText.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Toggle.h"
#include "tinalux/ui/VBox.h"

namespace tinalux::markup {

namespace {

template <typename WidgetT, typename StyleT, typename Mutator>
core::StylePropertyInfo makeThemeStyleProperty(
    const char* name,
    core::ValueType expectedType,
    StyleT ui::Theme::*themeStyle,
    Mutator mutator)
{
    return core::StylePropertyInfo {
        .name = name,
        .expectedType = expectedType,
        .setter =
            [themeStyle, mutator](ui::Widget& widget, const core::Value& value, const ui::Theme& theme) {
                auto& typedWidget = static_cast<WidgetT&>(widget);
                StyleT style = typedWidget.style() ? *typedWidget.style() : theme.*themeStyle;
                mutator(style, value);
                typedWidget.setStyle(style);
            },
    };
}

template <typename WidgetT, typename Mutator>
core::StylePropertyInfo makeDirectStyleProperty(
    const char* name,
    core::ValueType expectedType,
    Mutator mutator)
{
    return core::StylePropertyInfo {
        .name = name,
        .expectedType = expectedType,
        .setter = [mutator](ui::Widget& widget, const core::Value& value, const ui::Theme&) {
            mutator(static_cast<WidgetT&>(widget), value);
        },
    };
}

template <typename WidgetT, typename Binder>
core::InteractionInfo makeInteraction(
    const char* name,
    const char* boundProperty,
    core::ValueType payloadType,
    Binder binder)
{
    return core::InteractionInfo {
        .name = name,
        .boundProperty = boundProperty,
        .payloadType = payloadType,
        .bind = [binder](ui::Widget& widget, core::InteractionHandler handler) {
            binder(static_cast<WidgetT&>(widget), std::move(handler));
        },
    };
}

template <typename WidgetT, typename Attacher>
core::ChildAttachmentInfo makeChildAttachment(
    core::ChildAttachmentPolicy policy,
    Attacher attacher)
{
    return core::ChildAttachmentInfo {
        .policy = policy,
        .attach = [attacher](ui::Widget& widget, std::shared_ptr<ui::Widget> child) {
            attacher(static_cast<WidgetT&>(widget), std::move(child));
        },
    };
}

template <typename WidgetT>
core::ChildAttachmentInfo makeContainerChildAttachment()
{
    return makeChildAttachment<WidgetT>(
        core::ChildAttachmentPolicy::Multiple,
        [](WidgetT& widget, std::shared_ptr<ui::Widget> child) {
            widget.addChild(std::move(child));
        });
}

template <typename WidgetT, typename Setter>
core::ChildAttachmentInfo makeSingleContentChildAttachment(Setter setter)
{
    return makeChildAttachment<WidgetT>(
        core::ChildAttachmentPolicy::Single,
        [setter](WidgetT& widget, std::shared_ptr<ui::Widget> child) {
            setter(widget, std::move(child));
        });
}

std::optional<ui::IconType> parseIconType(std::string_view name)
{
    if (name == "Check") {
        return ui::IconType::Check;
    }
    if (name == "Close") {
        return ui::IconType::Close;
    }
    if (name == "Search") {
        return ui::IconType::Search;
    }
    if (name == "Clear") {
        return ui::IconType::Clear;
    }
    if (name == "User") {
        return ui::IconType::User;
    }
    if (name == "Lock") {
        return ui::IconType::Lock;
    }
    if (name == "Refresh") {
        return ui::IconType::Refresh;
    }
    if (name == "ArrowDown") {
        return ui::IconType::ArrowDown;
    }
    return std::nullopt;
}

} // namespace

void registerBuiltinTypes()
{
    auto& reg = core::TypeRegistry::instance();

    // ---- Layout containers ----

    reg.registerType(core::TypeInfo {
        .name = "VBox",
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
        .childAttachment = makeContainerChildAttachment<ui::VBox>(),
        .markupPositionalProperties = {"spacing", "padding"},
    });

    reg.registerType(core::TypeInfo {
        .name = "HBox",
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
        .childAttachment = makeContainerChildAttachment<ui::HBox>(),
        .markupPositionalProperties = {"spacing", "padding"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Flex",
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
        .childAttachment = makeContainerChildAttachment<ui::Flex>(),
        .markupPositionalProperties = {"direction", "spacing", "padding"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Grid",
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
        .childAttachment = makeContainerChildAttachment<ui::Grid>(),
        .markupPositionalProperties = {"columns", "rows", "columnGap", "rowGap", "padding"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Container",
        .factory = [] { return std::make_shared<ui::Container>(); },
        .properties = {},
        .childAttachment = makeContainerChildAttachment<ui::Container>(),
    });

    reg.registerType(core::TypeInfo {
        .name = "Panel",
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
        .styleProperties = {
            makeThemeStyleProperty<ui::Panel, ui::PanelStyle>(
                "backgroundColor",
                core::ValueType::Color,
                &ui::Theme::panelStyle,
                [](ui::PanelStyle& style, const core::Value& value) {
                    style.backgroundColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::Panel, ui::PanelStyle>(
                "cornerRadius",
                core::ValueType::Float,
                &ui::Theme::panelStyle,
                [](ui::PanelStyle& style, const core::Value& value) {
                    style.cornerRadius = value.asNumber();
                }),
        },
        .childAttachment = makeContainerChildAttachment<ui::Panel>(),
        .markupPositionalProperties = {"backgroundColor", "cornerRadius"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Dialog",
        .factory = [] { return std::make_shared<ui::Dialog>(); },
        .properties = {
            {"title", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setTitle(v.asString());
                }},
            {"dismissOnBackdrop", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setDismissOnBackdrop(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dialog&>(w).dismissOnBackdrop());
                }},
            {"dismissOnEscape", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setDismissOnEscape(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dialog&>(w).dismissOnEscape());
                }},
            {"showCloseButton", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dialog&>(w).setShowCloseButton(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dialog&>(w).showCloseButton());
                }},
            {"closeIcon", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const std::optional<ui::IconType> iconType = parseIconType(v.asString());
                    if (iconType.has_value()) {
                        static_cast<ui::Dialog&>(w).setCloseIcon(*iconType, 0.0f);
                    }
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
            {"maxWidth", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& dialog = static_cast<ui::Dialog&>(w);
                    const core::Size current = dialog.maxSize();
                    dialog.setMaxSize(core::Size::Make(v.asNumber(), current.height()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dialog&>(w).maxSize().width());
                }},
            {"maxHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& dialog = static_cast<ui::Dialog&>(w);
                    const core::Size current = dialog.maxSize();
                    dialog.setMaxSize(core::Size::Make(current.width(), v.asNumber()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dialog&>(w).maxSize().height());
                }},
        },
        .styleProperties = {
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "backdropColor",
                core::ValueType::Color,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.backdropColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "backgroundColor",
                core::ValueType::Color,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.backgroundColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "titleColor",
                core::ValueType::Color,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.titleColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "cornerRadius",
                core::ValueType::Float,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.cornerRadius = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "padding",
                core::ValueType::Float,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.padding = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "titleGap",
                core::ValueType::Float,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.titleGap = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "fontSize",
                core::ValueType::Float,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.titleTextStyle.fontSize = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "lineHeight",
                core::ValueType::Float,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.titleTextStyle.lineHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "letterSpacing",
                core::ValueType::Float,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.titleTextStyle.letterSpacing = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Dialog, ui::DialogStyle>(
                "bold",
                core::ValueType::Bool,
                &ui::Theme::dialogStyle,
                [](ui::DialogStyle& style, const core::Value& value) {
                    style.titleTextStyle.bold = value.asBool();
                }),
        },
        .interactions = {
            makeInteraction<ui::Dialog>(
                "dismiss",
                "",
                core::ValueType::None,
                [](ui::Dialog& dialog, core::InteractionHandler handler) {
                    dialog.onDismiss([handler = std::move(handler)] {
                        if (handler) {
                            handler(core::Value());
                        }
                    });
                }),
        },
        .childAttachment = makeSingleContentChildAttachment<ui::Dialog>(
            [](ui::Dialog& dialog, std::shared_ptr<ui::Widget> child) {
                dialog.setContent(std::move(child));
            }),
        .markupPositionalProperties = {"title", "padding"},
    });

    reg.registerType(core::TypeInfo {
        .name = "ScrollView",
        .factory = [] { return std::make_shared<ui::ScrollView>(); },
        .properties = {
            {"preferredHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ScrollView&>(w).setPreferredHeight(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ScrollView&>(w).preferredHeight());
                }},
            {"scrollOffset", core::ValueType::Float,
                {},
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ScrollView&>(w).scrollOffset());
                }},
        },
        .styleProperties = {
            makeThemeStyleProperty<ui::ScrollView, ui::ScrollViewStyle>(
                "scrollbarThumbColor",
                core::ValueType::Color,
                &ui::Theme::scrollViewStyle,
                [](ui::ScrollViewStyle& style, const core::Value& value) {
                    style.scrollbarThumbColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::ScrollView, ui::ScrollViewStyle>(
                "scrollbarTrackColor",
                core::ValueType::Color,
                &ui::Theme::scrollViewStyle,
                [](ui::ScrollViewStyle& style, const core::Value& value) {
                    style.scrollbarTrackColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::ScrollView, ui::ScrollViewStyle>(
                "scrollbarMargin",
                core::ValueType::Float,
                &ui::Theme::scrollViewStyle,
                [](ui::ScrollViewStyle& style, const core::Value& value) {
                    style.scrollbarMargin = value.asNumber();
                }),
            makeThemeStyleProperty<ui::ScrollView, ui::ScrollViewStyle>(
                "scrollbarWidth",
                core::ValueType::Float,
                &ui::Theme::scrollViewStyle,
                [](ui::ScrollViewStyle& style, const core::Value& value) {
                    style.scrollbarWidth = value.asNumber();
                }),
            makeThemeStyleProperty<ui::ScrollView, ui::ScrollViewStyle>(
                "minThumbHeight",
                core::ValueType::Float,
                &ui::Theme::scrollViewStyle,
                [](ui::ScrollViewStyle& style, const core::Value& value) {
                    style.minThumbHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::ScrollView, ui::ScrollViewStyle>(
                "scrollStep",
                core::ValueType::Float,
                &ui::Theme::scrollViewStyle,
                [](ui::ScrollViewStyle& style, const core::Value& value) {
                    style.scrollStep = value.asNumber();
                }),
        },
        .interactions = {
            makeInteraction<ui::ScrollView>(
                "scrollChanged",
                "scrollOffset",
                core::ValueType::Float,
                [](ui::ScrollView& scrollView, core::InteractionHandler handler) {
                    scrollView.onScrollChanged(
                        [handler = std::move(handler)](float offset) {
                            if (handler) {
                                handler(core::Value(offset));
                            }
                        });
                }),
        },
        .childAttachment = makeSingleContentChildAttachment<ui::ScrollView>(
            [](ui::ScrollView& scrollView, std::shared_ptr<ui::Widget> child) {
                scrollView.setContent(std::move(child));
        }),
        .markupPositionalProperties = {"preferredHeight"},
    });

    reg.registerType(core::TypeInfo {
        .name = "ListView",
        .factory = [] { return std::make_shared<ui::ListView>(); },
        .properties = {
            {"preferredHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ListView&>(w).setPreferredHeight(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ListView&>(w).preferredHeight());
                }},
            {"spacing", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ListView&>(w).setSpacing(v.asNumber());
                }},
            {"padding", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ListView&>(w).setPadding(v.asNumber());
                }},
            {"selectedIndex", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ListView&>(w).setSelectedIndex(v.asInt());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ListView&>(w).selectedIndex());
                }},
        },
        .styleProperties = {
            makeThemeStyleProperty<ui::ListView, ui::ListViewStyle>(
                "itemCornerRadius",
                core::ValueType::Float,
                &ui::Theme::listViewStyle,
                [](ui::ListViewStyle& style, const core::Value& value) {
                    style.itemCornerRadius = value.asNumber();
                }),
            makeThemeStyleProperty<ui::ListView, ui::ListViewStyle>(
                "selectionFillColor",
                core::ValueType::Color,
                &ui::Theme::listViewStyle,
                [](ui::ListViewStyle& style, const core::Value& value) {
                    style.selectionFillColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::ListView, ui::ListViewStyle>(
                "selectionBorderColor",
                core::ValueType::Color,
                &ui::Theme::listViewStyle,
                [](ui::ListViewStyle& style, const core::Value& value) {
                    style.selectionBorderColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::ListView, ui::ListViewStyle>(
                "focusBorderColor",
                core::ValueType::Color,
                &ui::Theme::listViewStyle,
                [](ui::ListViewStyle& style, const core::Value& value) {
                    style.focusBorderColor = value.asColor();
                }),
        },
        .interactions = {
            makeInteraction<ui::ListView>(
                "selectionChanged",
                "selectedIndex",
                core::ValueType::Int,
                [](ui::ListView& listView, core::InteractionHandler handler) {
                    listView.onSelectionChanged(
                        [handler = std::move(handler)](int index) {
                            if (handler) {
                                handler(core::Value(index));
                            }
                        });
                }),
        },
        .markupPositionalProperties = {"preferredHeight", "spacing", "padding", "selectedIndex"},
    });

    // ---- Leaf widgets ----

    reg.registerType(core::TypeInfo {
        .name = "Label",
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
        .markupPositionalProperties = {"text"},
    });

    reg.registerType(core::TypeInfo {
        .name = "ParagraphLabel",
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
        .markupPositionalProperties = {"text"},
    });

    reg.registerType(core::TypeInfo {
        .name = "RichText",
        .factory = [] { return std::make_shared<ui::RichTextWidget>(); },
        .properties = {
            {"text", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    ui::TextSpan span;
                    span.text = v.asString();
                    static_cast<ui::RichTextWidget&>(w).setSpans({std::move(span)});
                }},
            {"fontSize", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::RichTextWidget&>(w).setDefaultFontSize(v.asNumber());
                }},
            {"color", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::RichTextWidget&>(w).setDefaultColor(v.asColor());
                }},
            {"linkColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::RichTextWidget&>(w).setLinkColor(v.asColor());
                }},
            {"lineHeightMultiplier", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::RichTextWidget&>(w).setLineHeightMultiplier(v.asNumber());
                }},
            {"maxLines", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    const int maxLines = v.asInt();
                    static_cast<ui::RichTextWidget&>(w).setMaxLines(
                        maxLines > 0 ? static_cast<std::size_t>(maxLines) : 0u);
                }},
            {"align", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& richText = static_cast<ui::RichTextWidget&>(w);
                    if (s == "Start") richText.setTextAlign(ui::RichTextAlign::Start);
                    else if (s == "Left") richText.setTextAlign(ui::RichTextAlign::Left);
                    else if (s == "Center") richText.setTextAlign(ui::RichTextAlign::Center);
                    else if (s == "Right") richText.setTextAlign(ui::RichTextAlign::Right);
                    else if (s == "Justify") richText.setTextAlign(ui::RichTextAlign::Justify);
                    else if (s == "End") richText.setTextAlign(ui::RichTextAlign::End);
                }},
        },
        .styleProperties = {
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "fontSize",
                core::ValueType::Float,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.body.textStyle.fontSize = value.asNumber();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "lineHeight",
                core::ValueType::Float,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.body.textStyle.lineHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "letterSpacing",
                core::ValueType::Float,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.body.textStyle.letterSpacing = value.asNumber();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "bold",
                core::ValueType::Bool,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.body.textStyle.bold = value.asBool();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "color",
                core::ValueType::Color,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.body.textColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "quoteAccentColor",
                core::ValueType::Color,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.quoteAccentColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "listMarkerColor",
                core::ValueType::Color,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.listMarkerColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::RichTextWidget, ui::RichTextStyle>(
                "listLevelIndent",
                core::ValueType::Float,
                &ui::Theme::richTextStyle,
                [](ui::RichTextStyle& style, const core::Value& value) {
                    style.listLevelIndent = value.asNumber();
                }),
            makeDirectStyleProperty<ui::RichTextWidget>(
                "linkColor",
                core::ValueType::Color,
                [](ui::RichTextWidget& widget, const core::Value& value) {
                    widget.setLinkColor(value.asColor());
                }),
        },
        .markupPositionalProperties = {"text"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Button",
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
            {"iconPlacement", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& button = static_cast<ui::Button&>(w);
                    if (s == "Start") button.setIconPlacement(ui::ButtonIconPlacement::Start);
                    else if (s == "End") button.setIconPlacement(ui::ButtonIconPlacement::End);
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    const auto placement = static_cast<const ui::Button&>(w).iconPlacement();
                    return core::Value::enumValue(
                        placement == ui::ButtonIconPlacement::End ? "End" : "Start");
                }},
            {"iconWidth", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& button = static_cast<ui::Button&>(w);
                    const core::Size current = button.iconSize();
                    button.setIconSize(core::Size::Make(v.asNumber(), current.height()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Button&>(w).iconSize().width());
                }},
            {"iconHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& button = static_cast<ui::Button&>(w);
                    const core::Size current = button.iconSize();
                    button.setIconSize(core::Size::Make(current.width(), v.asNumber()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Button&>(w).iconSize().height());
                }},
        },
        .styleProperties = {
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "backgroundColor",
                core::ValueType::Color,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.backgroundColor.normal = value.asColor();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "textColor",
                core::ValueType::Color,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.textColor.normal = value.asColor();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "borderColor",
                core::ValueType::Color,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.borderColor.normal = value.asColor();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "borderWidth",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.borderWidth.normal = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "borderRadius",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.borderRadius = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "paddingHorizontal",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.paddingHorizontal = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "paddingVertical",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.paddingVertical = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "iconSpacing",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.iconSpacing = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "minWidth",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.minWidth = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "minHeight",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.minHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "focusRingColor",
                core::ValueType::Color,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.focusRingColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "focusRingWidth",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.focusRingWidth = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "fontSize",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.textStyle.fontSize = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "lineHeight",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.textStyle.lineHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "letterSpacing",
                core::ValueType::Float,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.textStyle.letterSpacing = value.asNumber();
                }),
            makeThemeStyleProperty<ui::Button, ui::ButtonStyle>(
                "bold",
                core::ValueType::Bool,
                &ui::Theme::buttonStyle,
                [](ui::ButtonStyle& style, const core::Value& value) {
                    style.textStyle.bold = value.asBool();
                }),
        },
        .interactions = {
            makeInteraction<ui::Button>(
                "click",
                "",
                core::ValueType::None,
                [](ui::Button& button, core::InteractionHandler handler) {
                    button.onClick([handler = std::move(handler)] {
                        if (handler) {
                            handler(core::Value());
                        }
                    });
                }),
        },
        .markupPositionalProperties = {"text", "icon", "iconPlacement"},
    });

    reg.registerType(core::TypeInfo {
        .name = "TextInput",
        .factory = [] { return std::make_shared<ui::TextInput>(""); },
        .properties = {
            {"placeholder", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).setPlaceholder(v.asString());
                }},
            {"text", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::TextInput&>(w).setText(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::TextInput&>(w).text());
                }},
            {"selectedText", core::ValueType::String,
                {},
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::TextInput&>(w).selectedText());
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
        .styleProperties = {
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "backgroundColor",
                core::ValueType::Color,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.backgroundColor.normal = value.asColor();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "borderColor",
                core::ValueType::Color,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.borderColor.normal = value.asColor();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "borderWidth",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.borderWidth.normal = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "textColor",
                core::ValueType::Color,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.textColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "placeholderColor",
                core::ValueType::Color,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.placeholderColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "selectionColor",
                core::ValueType::Color,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.selectionColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "caretColor",
                core::ValueType::Color,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.caretColor = value.asColor();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "borderRadius",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.borderRadius = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "paddingHorizontal",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.paddingHorizontal = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "paddingVertical",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.paddingVertical = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "selectionCornerRadius",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.selectionCornerRadius = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "minWidth",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.minWidth = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "minHeight",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.minHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "fontSize",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.textStyle.fontSize = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "lineHeight",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.textStyle.lineHeight = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "letterSpacing",
                core::ValueType::Float,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.textStyle.letterSpacing = value.asNumber();
                }),
            makeThemeStyleProperty<ui::TextInput, ui::TextInputStyle>(
                "bold",
                core::ValueType::Bool,
                &ui::Theme::textInputStyle,
                [](ui::TextInputStyle& style, const core::Value& value) {
                    style.textStyle.bold = value.asBool();
                }),
        },
        .interactions = {
            makeInteraction<ui::TextInput>(
                "textChanged",
                "text",
                core::ValueType::String,
                [](ui::TextInput& input, core::InteractionHandler handler) {
                    input.onTextChanged(
                        [handler = std::move(handler)](const std::string& text) {
                            if (handler) {
                                handler(core::Value(text));
                            }
                        });
                }),
            makeInteraction<ui::TextInput>(
                "selectedTextChanged",
                "selectedText",
                core::ValueType::String,
                [](ui::TextInput& input, core::InteractionHandler handler) {
                    input.onSelectionChanged(
                        [handler = std::move(handler)](const std::string& text) {
                            if (handler) {
                                handler(core::Value(text));
                            }
                        });
                }),
            makeInteraction<ui::TextInput>(
                "leadingIconClick",
                "",
                core::ValueType::None,
                [](ui::TextInput& input, core::InteractionHandler handler) {
                    input.onLeadingIconClick([handler = std::move(handler)] {
                        if (handler) {
                            handler(core::Value());
                        }
                    });
                }),
            makeInteraction<ui::TextInput>(
                "trailingIconClick",
                "",
                core::ValueType::None,
                [](ui::TextInput& input, core::InteractionHandler handler) {
                    input.onTrailingIconClick([handler = std::move(handler)] {
                        if (handler) {
                            handler(core::Value());
                        }
                    });
                }),
        },
        .markupPositionalProperties = {"placeholder", "text", "obscured", "leadingIcon", "trailingIcon"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Dropdown",
        .factory = [] { return std::make_shared<ui::Dropdown>(); },
        .properties = {
            {"placeholder", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).setPlaceholder(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dropdown&>(w).placeholder());
                }},
            {"maxVisibleItems", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).setMaxVisibleItems(v.asInt());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dropdown&>(w).maxVisibleItems());
                }},
            {"selectedIndex", core::ValueType::Int,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).setSelectedIndex(v.asInt());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Dropdown&>(w).selectedIndex());
                }},
            {"indicatorIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Dropdown&>(w).loadIndicatorIconAsync(v.asString());
                }},
        },
        .interactions = {
            makeInteraction<ui::Dropdown>(
                "selectionChanged",
                "selectedIndex",
                core::ValueType::Int,
                [](ui::Dropdown& dropdown, core::InteractionHandler handler) {
                    dropdown.onSelectionChanged(
                        [handler = std::move(handler)](int index) {
                            if (handler) {
                                handler(core::Value(index));
                            }
                        });
                }),
        },
        .markupPositionalProperties = {"placeholder", "maxVisibleItems", "selectedIndex", "indicatorIcon"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Checkbox",
        .factory = [] { return std::make_shared<ui::Checkbox>("", false); },
        .properties = {
            {"label", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Checkbox&>(w).setLabel(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Checkbox&>(w).label());
                }},
            {"checked", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Checkbox&>(w).setChecked(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Checkbox&>(w).checked());
                }},
            {"checkmarkIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Checkbox&>(w).loadCheckmarkIconAsync(v.asString());
                }},
        },
        .interactions = {
            makeInteraction<ui::Checkbox>(
                "toggle",
                "checked",
                core::ValueType::Bool,
                [](ui::Checkbox& checkbox, core::InteractionHandler handler) {
                    checkbox.onToggle(
                        [handler = std::move(handler)](bool checked) {
                            if (handler) {
                                handler(core::Value(checked));
                            }
                        });
                }),
        },
        .markupPositionalProperties = {"label", "checked", "checkmarkIcon"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Radio",
        .factory = [] { return std::make_shared<ui::Radio>(""); },
        .properties = {
            {"label", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).setLabel(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Radio&>(w).label());
                }},
            {"group", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).setGroup(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Radio&>(w).group());
                }},
            {"selected", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).setSelected(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Radio&>(w).selected());
                }},
            {"selectionIcon", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Radio&>(w).loadSelectionIconAsync(v.asString());
                }},
        },
        .interactions = {
            makeInteraction<ui::Radio>(
                "changed",
                "selected",
                core::ValueType::Bool,
                [](ui::Radio& radio, core::InteractionHandler handler) {
                    radio.onChanged(
                        [handler = std::move(handler)](bool selected) {
                            if (handler) {
                                handler(core::Value(selected));
                            }
                        });
                }),
        },
        .markupPositionalProperties = {"label", "group", "selected", "selectionIcon"},
    });

    reg.registerType(core::TypeInfo {
        .name = "ImageWidget",
        .factory = [] { return std::make_shared<ui::ImageWidget>(); },
        .properties = {
            {"source", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ImageWidget&>(w).loadImageAsync(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).imagePath());
                }},
            {"fit", core::ValueType::Enum,
                [](ui::Widget& w, const core::Value& v) {
                    const auto& s = v.asString();
                    auto& image = static_cast<ui::ImageWidget&>(w);
                    if (s == "None") image.setFit(ui::ImageFit::None);
                    else if (s == "Fill") image.setFit(ui::ImageFit::Fill);
                    else if (s == "Contain") image.setFit(ui::ImageFit::Contain);
                    else if (s == "Cover") image.setFit(ui::ImageFit::Cover);
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    const auto fit = static_cast<const ui::ImageWidget&>(w).fit();
                    switch (fit) {
                    case ui::ImageFit::None:
                        return core::Value::enumValue("None");
                    case ui::ImageFit::Fill:
                        return core::Value::enumValue("Fill");
                    case ui::ImageFit::Contain:
                        return core::Value::enumValue("Contain");
                    case ui::ImageFit::Cover:
                        return core::Value::enumValue("Cover");
                    }
                    return std::nullopt;
                }},
            {"opacity", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ImageWidget&>(w).setOpacity(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).opacity());
                }},
            {"preferredWidth", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& image = static_cast<ui::ImageWidget&>(w);
                    const core::Size size = image.preferredSize();
                    image.setPreferredSize(core::Size::Make(v.asNumber(), size.height()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).preferredSize().width());
                }},
            {"preferredHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& image = static_cast<ui::ImageWidget&>(w);
                    const core::Size size = image.preferredSize();
                    image.setPreferredSize(core::Size::Make(size.width(), v.asNumber()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).preferredSize().height());
                }},
            {"placeholderWidth", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& image = static_cast<ui::ImageWidget&>(w);
                    const core::Size size = image.placeholderSize();
                    image.setPlaceholderSize(core::Size::Make(v.asNumber(), size.height()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).placeholderSize().width());
                }},
            {"placeholderHeight", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& image = static_cast<ui::ImageWidget&>(w);
                    const core::Size size = image.placeholderSize();
                    image.setPlaceholderSize(core::Size::Make(size.width(), v.asNumber()));
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).placeholderSize().height());
                }},
            {"loadingPlaceholderColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ImageWidget&>(w).setLoadingPlaceholderColor(v.asColor());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).loadingPlaceholderColor());
                }},
            {"failedPlaceholderColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ImageWidget&>(w).setFailedPlaceholderColor(v.asColor());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ImageWidget&>(w).failedPlaceholderColor());
                }},
        },
        .markupPositionalProperties = {"source", "fit", "opacity", "preferredWidth", "preferredHeight"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Toggle",
        .factory = [] { return std::make_shared<ui::Toggle>(""); },
        .properties = {
            {"label", core::ValueType::String,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Toggle&>(w).setLabel(v.asString());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Toggle&>(w).label());
                }},
            {"on", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Toggle&>(w).setOn(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Toggle&>(w).on());
                }},
        },
        .interactions = {
            makeInteraction<ui::Toggle>(
                "toggle",
                "on",
                core::ValueType::Bool,
                [](ui::Toggle& toggle, core::InteractionHandler handler) {
                    toggle.onToggle(
                        [handler = std::move(handler)](bool on) {
                            if (handler) {
                                handler(core::Value(on));
                            }
                        });
                }),
        },
        .markupPositionalProperties = {"label", "on"},
    });

    reg.registerType(core::TypeInfo {
        .name = "Slider",
        .factory = [] { return std::make_shared<ui::Slider>(); },
        .properties = {
            {"value", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Slider&>(w).setValue(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Slider&>(w).value());
                },
                2},
            {"min", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& slider = static_cast<ui::Slider&>(w);
                    slider.setRange(v.asNumber(), slider.maximum());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Slider&>(w).minimum());
                },
                0},
            {"max", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& slider = static_cast<ui::Slider&>(w);
                    slider.setRange(slider.minimum(), v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Slider&>(w).maximum());
                },
                0},
            {"step", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::Slider&>(w).setStep(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::Slider&>(w).step());
                },
                1},
        },
        .interactions = {
            makeInteraction<ui::Slider>(
                "valueChanged",
                "value",
                core::ValueType::Float,
                [](ui::Slider& slider, core::InteractionHandler handler) {
                    slider.onValueChanged(
                        [handler = std::move(handler)](float value) {
                            if (handler) {
                                handler(core::Value(value));
                            }
                        });
                }),
        },
        .markupPositionalProperties = {"value", "min", "max", "step"},
    });

    reg.registerType(core::TypeInfo {
        .name = "ProgressBar",
        .factory = [] { return std::make_shared<ui::ProgressBar>(); },
        .properties = {
            {"value", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setValue(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).value());
                },
                1},
            {"min", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& progressBar = static_cast<ui::ProgressBar&>(w);
                    progressBar.setRange(v.asNumber(), progressBar.max());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).min());
                },
                0},
            {"max", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    auto& progressBar = static_cast<ui::ProgressBar&>(w);
                    progressBar.setRange(progressBar.min(), v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).max());
                },
                0},
            {"indeterminate", core::ValueType::Bool,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setIndeterminate(v.asBool());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).indeterminate());
                }},
            {"color", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setColor(v.asColor());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).color());
                }},
            {"backgroundColor", core::ValueType::Color,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setBackgroundColor(v.asColor());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).backgroundColor());
                }},
            {"height", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setHeight(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).height());
                }},
            {"preferredWidth", core::ValueType::Float,
                [](ui::Widget& w, const core::Value& v) {
                    static_cast<ui::ProgressBar&>(w).setPreferredWidth(v.asNumber());
                },
                [](const ui::Widget& w) -> std::optional<core::Value> {
                    return core::Value(static_cast<const ui::ProgressBar&>(w).preferredWidth());
                }},
        },
        .styleProperties = {
            makeDirectStyleProperty<ui::ProgressBar>(
                "color",
                core::ValueType::Color,
                [](ui::ProgressBar& widget, const core::Value& value) {
                    widget.setColor(value.asColor());
                }),
            makeDirectStyleProperty<ui::ProgressBar>(
                "backgroundColor",
                core::ValueType::Color,
                [](ui::ProgressBar& widget, const core::Value& value) {
                    widget.setBackgroundColor(value.asColor());
                }),
            makeDirectStyleProperty<ui::ProgressBar>(
                "height",
                core::ValueType::Float,
                [](ui::ProgressBar& widget, const core::Value& value) {
                    widget.setHeight(value.asNumber());
                }),
        },
        .markupPositionalProperties = {"value", "indeterminate", "color", "backgroundColor", "height", "min", "max", "preferredWidth"},
    });
}

} // namespace tinalux::markup
