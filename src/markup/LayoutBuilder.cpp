#include "tinalux/markup/LayoutBuilder.h"

#include <optional>
#include <sstream>

#include "tinalux/core/Reflect.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::markup {

namespace {

std::optional<std::string> styleNameFromValue(const core::Value& value)
{
    if (value.type() == core::ValueType::Enum || value.type() == core::ValueType::String) {
        return value.asString();
    }
    return std::nullopt;
}

template <typename Style, typename WidgetT, typename Mutator>
void mutateWidgetStyle(WidgetT& widget, const Style& fallbackStyle, Mutator mutator)
{
    Style style = widget.style() ? *widget.style() : fallbackStyle;
    mutator(style);
    widget.setStyle(style);
}

} // namespace

LayoutBuilder::LayoutBuilder(const ui::Theme& theme)
    : theme_(theme)
{
}

BuildResult LayoutBuilder::build(const AstNode& ast, const ui::Theme& theme)
{
    AstDocument document;
    document.root = ast;
    return build(document, theme);
}

BuildResult LayoutBuilder::build(const AstDocument& document, const ui::Theme& theme)
{
    LayoutBuilder builder(theme);
    builder.registerStyles(document.styles);
    builder.registerComponents(document.components);
    BuildResult result;
    if (document.root) {
        result.root = builder.buildNode(*document.root);
    }
    result.idMap = std::move(builder.idMap_);
    result.warnings = std::move(builder.warnings_);
    return result;
}

void LayoutBuilder::registerStyles(const std::vector<AstStyleDefinition>& styles)
{
    for (const auto& style : styles) {
        styleMap_[style.name] = style;
    }
}

void LayoutBuilder::registerComponents(const std::vector<AstComponentDefinition>& components)
{
    for (const auto& component : components) {
        componentMap_[component.name] = component;
    }
}

std::shared_ptr<ui::Widget> LayoutBuilder::buildNode(const AstNode& node)
{
    const auto componentIt = componentMap_.find(node.typeName);
    if (componentIt != componentMap_.end()) {
        return buildComponentNode(componentIt->second, node);
    }

    auto& registry = core::TypeRegistry::instance();
    const core::TypeInfo* typeInfo = registry.findType(node.typeName);

    if (!typeInfo) {
        std::ostringstream oss;
        oss << "unknown widget type '" << node.typeName << "' at line " << node.line;
        warnings_.push_back(oss.str());
        return nullptr;
    }

    auto widget = typeInfo->factory();
    if (!widget) {
        warnings_.push_back("factory returned null for '" + node.typeName + "'");
        return nullptr;
    }

    for (const auto& prop : node.properties) {
        if (prop.name == "style") {
            applyNamedStyle(widget, node.typeName, prop);
        }
    }

    for (const auto& prop : node.properties) {
        if (prop.name == "style") {
            continue;
        }
        applyStandardProperty(widget, *typeInfo, node.typeName, prop);
    }

    if (!attachChildren(widget, node.typeName, node) && !node.children.empty()) {
        std::ostringstream oss;
        oss << "'" << node.typeName << "' is not a container but has children at line "
            << node.line;
        warnings_.push_back(oss.str());
    }

    return widget;
}

std::shared_ptr<ui::Widget> LayoutBuilder::buildComponentNode(
    const AstComponentDefinition& component,
    const AstNode& instanceNode)
{
    for (const auto& activeComponent : componentStack_) {
        if (activeComponent == component.name) {
            std::ostringstream oss;
            oss << "cyclic component reference '" << component.name << "' at line "
                << instanceNode.line;
            warnings_.push_back(oss.str());
            return nullptr;
        }
    }

    componentStack_.push_back(component.name);
    AstNode mergedNode = mergeComponentNode(component.root, instanceNode);
    auto widget = buildNode(mergedNode);
    componentStack_.pop_back();
    return widget;
}

AstNode LayoutBuilder::mergeComponentNode(
    const AstNode& templateNode,
    const AstNode& instanceNode) const
{
    AstNode merged = templateNode;
    merged.line = instanceNode.line;
    merged.column = instanceNode.column;

    for (const auto& instanceProp : instanceNode.properties) {
        bool replaced = false;
        for (auto& mergedProp : merged.properties) {
            if (mergedProp.name == instanceProp.name) {
                mergedProp = instanceProp;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            merged.properties.push_back(instanceProp);
        }
    }

    if (!instanceNode.children.empty()) {
        merged.children.insert(
            merged.children.end(),
            instanceNode.children.begin(),
            instanceNode.children.end());
    }

    return merged;
}

void LayoutBuilder::applyNamedStyle(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const AstProperty& prop)
{
    const std::optional<std::string> styleName = styleNameFromValue(prop.value);
    if (!styleName) {
        std::ostringstream oss;
        oss << "style reference on '" << nodeType << "' must be a string or identifier at line "
            << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    const auto it = styleMap_.find(*styleName);
    if (it == styleMap_.end()) {
        std::ostringstream oss;
        oss << "unknown style '" << *styleName << "' on '" << nodeType
            << "' at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    const AstStyleDefinition& style = it->second;
    if (style.targetType != nodeType) {
        std::ostringstream oss;
        oss << "style '" << style.name << "' targets '" << style.targetType
            << "' but was applied to '" << nodeType << "' at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    for (const auto& styleProp : style.properties) {
        if (!applyStyleProperty(*widget, nodeType, styleProp)) {
            std::ostringstream oss;
            oss << "unsupported style property '" << styleProp.name << "' in style '"
                << style.name << "' for '" << nodeType << "' at line " << styleProp.line;
            warnings_.push_back(oss.str());
        }
    }
}

void LayoutBuilder::applyStandardProperty(
    const std::shared_ptr<ui::Widget>& widget,
    const core::TypeInfo& typeInfo,
    const std::string& nodeType,
    const AstProperty& prop)
{
    if (prop.name == "id") {
        if (prop.value.type() == core::ValueType::String) {
            widget->setId(prop.value.asString());
            if (idMap_.contains(prop.value.asString())) {
                std::ostringstream oss;
                oss << "duplicate id '" << prop.value.asString() << "' on '" << nodeType
                    << "' at line " << prop.line;
                warnings_.push_back(oss.str());
            }
            idMap_[prop.value.asString()] = widget;
        } else {
            std::ostringstream oss;
            oss << "property 'id' on '" << nodeType << "' expects a string at line "
                << prop.line;
            warnings_.push_back(oss.str());
        }
        return;
    }

    if (prop.name == "visible") {
        if (prop.value.type() == core::ValueType::Bool) {
            widget->setVisible(prop.value.asBool());
        } else {
            std::ostringstream oss;
            oss << "property 'visible' on '" << nodeType << "' expects a bool at line "
                << prop.line;
            warnings_.push_back(oss.str());
        }
        return;
    }

    const core::PropertyInfo* propInfo = typeInfo.findProperty(prop.name);
    if (!propInfo) {
        std::ostringstream oss;
        oss << "unknown property '" << prop.name << "' on '" << nodeType
            << "' at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    propInfo->setter(*widget, prop.value);
}

bool LayoutBuilder::applyStyleProperty(
    ui::Widget& widget,
    const std::string& nodeType,
    const AstProperty& prop)
{
    if (nodeType == "Button") {
        auto* button = dynamic_cast<ui::Button*>(&widget);
        if (!button) {
            return false;
        }

        if (prop.name == "backgroundColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.backgroundColor.normal = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "textColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.textColor.normal = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "borderColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.borderColor.normal = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "borderWidth") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.borderWidth.normal = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "borderRadius") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.borderRadius = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "paddingHorizontal") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.paddingHorizontal = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "paddingVertical") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.paddingVertical = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "iconSpacing") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.iconSpacing = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "minWidth") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.minWidth = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "minHeight") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.minHeight = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "focusRingColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.focusRingColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "focusRingWidth") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.focusRingWidth = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "fontSize") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.textStyle.fontSize = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "lineHeight") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.textStyle.lineHeight = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "letterSpacing") {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.textStyle.letterSpacing = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "bold" && prop.value.type() == core::ValueType::Bool) {
            mutateWidgetStyle(*button, theme_.buttonStyle, [&](ui::ButtonStyle& style) {
                style.textStyle.bold = prop.value.asBool();
            });
            return true;
        }
        return false;
    }

    if (nodeType == "Panel") {
        auto* panel = dynamic_cast<ui::Panel*>(&widget);
        if (!panel) {
            return false;
        }

        if (prop.name == "backgroundColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*panel, theme_.panelStyle, [&](ui::PanelStyle& style) {
                style.backgroundColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "cornerRadius") {
            mutateWidgetStyle(*panel, theme_.panelStyle, [&](ui::PanelStyle& style) {
                style.cornerRadius = prop.value.asNumber();
            });
            return true;
        }
        return false;
    }

    if (nodeType == "TextInput") {
        auto* input = dynamic_cast<ui::TextInput*>(&widget);
        if (!input) {
            return false;
        }

        if (prop.name == "backgroundColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.backgroundColor.normal = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "borderColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.borderColor.normal = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "borderWidth") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.borderWidth.normal = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "textColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.textColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "placeholderColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.placeholderColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "selectionColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.selectionColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "caretColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.caretColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "borderRadius") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.borderRadius = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "paddingHorizontal") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.paddingHorizontal = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "paddingVertical") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.paddingVertical = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "selectionCornerRadius") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.selectionCornerRadius = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "minWidth") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.minWidth = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "minHeight") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.minHeight = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "fontSize") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.textStyle.fontSize = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "lineHeight") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.textStyle.lineHeight = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "letterSpacing") {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.textStyle.letterSpacing = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "bold" && prop.value.type() == core::ValueType::Bool) {
            mutateWidgetStyle(*input, theme_.textInputStyle, [&](ui::TextInputStyle& style) {
                style.textStyle.bold = prop.value.asBool();
            });
            return true;
        }
        return false;
    }

    if (nodeType == "Dialog") {
        auto* dialog = dynamic_cast<ui::Dialog*>(&widget);
        if (!dialog) {
            return false;
        }

        if (prop.name == "backdropColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.backdropColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "backgroundColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.backgroundColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "titleColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.titleColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "cornerRadius") {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.cornerRadius = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "padding") {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.padding = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "titleGap") {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.titleGap = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "fontSize") {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.titleTextStyle.fontSize = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "lineHeight") {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.titleTextStyle.lineHeight = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "letterSpacing") {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.titleTextStyle.letterSpacing = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "bold" && prop.value.type() == core::ValueType::Bool) {
            mutateWidgetStyle(*dialog, theme_.dialogStyle, [&](ui::DialogStyle& style) {
                style.titleTextStyle.bold = prop.value.asBool();
            });
            return true;
        }
        return false;
    }

    if (nodeType == "ScrollView") {
        auto* scrollView = dynamic_cast<ui::ScrollView*>(&widget);
        if (!scrollView) {
            return false;
        }

        if (prop.name == "scrollbarThumbColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*scrollView, theme_.scrollViewStyle, [&](ui::ScrollViewStyle& style) {
                style.scrollbarThumbColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "scrollbarTrackColor" && prop.value.type() == core::ValueType::Color) {
            mutateWidgetStyle(*scrollView, theme_.scrollViewStyle, [&](ui::ScrollViewStyle& style) {
                style.scrollbarTrackColor = prop.value.asColor();
            });
            return true;
        }
        if (prop.name == "scrollbarMargin") {
            mutateWidgetStyle(*scrollView, theme_.scrollViewStyle, [&](ui::ScrollViewStyle& style) {
                style.scrollbarMargin = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "scrollbarWidth") {
            mutateWidgetStyle(*scrollView, theme_.scrollViewStyle, [&](ui::ScrollViewStyle& style) {
                style.scrollbarWidth = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "minThumbHeight") {
            mutateWidgetStyle(*scrollView, theme_.scrollViewStyle, [&](ui::ScrollViewStyle& style) {
                style.minThumbHeight = prop.value.asNumber();
            });
            return true;
        }
        if (prop.name == "scrollStep") {
            mutateWidgetStyle(*scrollView, theme_.scrollViewStyle, [&](ui::ScrollViewStyle& style) {
                style.scrollStep = prop.value.asNumber();
            });
            return true;
        }
        return false;
    }

    if (nodeType == "ProgressBar") {
        auto* progressBar = dynamic_cast<ui::ProgressBar*>(&widget);
        if (!progressBar) {
            return false;
        }

        if (prop.name == "color" && prop.value.type() == core::ValueType::Color) {
            progressBar->setColor(prop.value.asColor());
            return true;
        }
        if (prop.name == "backgroundColor" && prop.value.type() == core::ValueType::Color) {
            progressBar->setBackgroundColor(prop.value.asColor());
            return true;
        }
        if (prop.name == "height") {
            progressBar->setHeight(prop.value.asNumber());
            return true;
        }
        return false;
    }

    return false;
}

bool LayoutBuilder::attachChildren(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const AstNode& node)
{
    if (auto* dialog = dynamic_cast<ui::Dialog*>(widget.get())) {
        if (node.children.empty()) {
            return true;
        }

        if (node.children.size() > 1) {
            std::ostringstream oss;
            oss << "'" << nodeType << "' accepts only one child content node at line "
                << node.line;
            warnings_.push_back(oss.str());
        }

        auto child = buildNode(node.children.front());
        if (child) {
            dialog->setContent(std::move(child));
        }
        return true;
    }

    if (auto* scrollView = dynamic_cast<ui::ScrollView*>(widget.get())) {
        if (node.children.empty()) {
            return true;
        }

        if (node.children.size() > 1) {
            std::ostringstream oss;
            oss << "'" << nodeType << "' accepts only one child content node at line "
                << node.line;
            warnings_.push_back(oss.str());
        }

        auto child = buildNode(node.children.front());
        if (child) {
            scrollView->setContent(std::move(child));
        }
        return true;
    }

    if (auto* container = dynamic_cast<ui::Container*>(widget.get())) {
        for (const auto& childNode : node.children) {
            auto child = buildNode(childNode);
            if (child) {
                container->addChild(std::move(child));
            }
        }
        return true;
    }

    return false;
}

} // namespace tinalux::markup
