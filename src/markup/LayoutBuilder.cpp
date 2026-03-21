#include "tinalux/markup/LayoutBuilder.h"

#include <algorithm>
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

std::optional<std::string> stringLikeValue(const core::Value& value)
{
    if (value.type() == core::ValueType::Enum || value.type() == core::ValueType::String) {
        return value.asString();
    }
    return std::nullopt;
}

std::string slotLabel(std::string_view slotName)
{
    if (slotName.empty()) {
        return "default slot";
    }

    return "slot '" + std::string(slotName) + "'";
}

template <typename Style, typename WidgetT, typename Mutator>
void mutateWidgetStyle(WidgetT& widget, const Style& fallbackStyle, Mutator mutator)
{
    Style style = widget.style() ? *widget.style() : fallbackStyle;
    mutator(style);
    widget.setStyle(style);
}

core::ValueType stylePropertyBindingType(std::string_view propertyName)
{
    if (propertyName == "backgroundColor"
        || propertyName == "textColor"
        || propertyName == "borderColor"
        || propertyName == "focusRingColor"
        || propertyName == "placeholderColor"
        || propertyName == "selectionColor"
        || propertyName == "caretColor"
        || propertyName == "backdropColor"
        || propertyName == "titleColor"
        || propertyName == "scrollbarThumbColor"
        || propertyName == "scrollbarTrackColor"
        || propertyName == "color") {
        return core::ValueType::Color;
    }

    if (propertyName == "bold") {
        return core::ValueType::Bool;
    }

    return core::ValueType::None;
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
    result.bindings = std::move(builder.bindings_);
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
            if (prop.hasObjectValue()) {
                applyInlineStyle(widget, node.typeName, prop);
            } else {
                applyNamedStyle(widget, node.typeName, prop);
            }
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
    AstNode mergedNode = mergeComponentNode(component, instanceNode);
    std::shared_ptr<ui::Widget> widget;
    if (!mergedNode.typeName.empty()) {
        widget = buildNode(mergedNode);
    }
    componentStack_.pop_back();
    return widget;
}

AstNode LayoutBuilder::mergeComponentNode(
    const AstComponentDefinition& component,
    const AstNode& instanceNode)
{
    std::unordered_map<std::string, AstProperty> parameterValues;
    parameterValues.reserve(component.parameters.size());
    for (const auto& parameter : component.parameters) {
        parameterValues[parameter.name] = parameter;
    }

    std::vector<AstProperty> rootOverrides;
    rootOverrides.reserve(instanceNode.properties.size());
    for (const auto& instanceProp : instanceNode.properties) {
        bool matchedParameter = false;
        for (const auto& parameter : component.parameters) {
            if (parameter.name == instanceProp.name) {
                parameterValues[parameter.name] = instanceProp;
                matchedParameter = true;
                break;
            }
        }

        if (!matchedParameter) {
            rootOverrides.push_back(instanceProp);
        }
    }

    const bool templateHasSlots = containsSlotNode(component.root);
    std::unordered_map<std::string, std::vector<AstNode>> slotChildren;
    std::vector<AstNode> legacyChildren;

    for (const auto& instanceChild : instanceNode.children) {
        AstNode normalizedChild = instanceChild;
        std::optional<std::string> explicitSlotName;

        for (auto propIt = normalizedChild.properties.begin();
             propIt != normalizedChild.properties.end();) {
            if (propIt->name != "slot") {
                ++propIt;
                continue;
            }

            explicitSlotName = {};
            const std::optional<std::string> slotName = stringLikeValue(propIt->value);
            if (!slotName) {
                std::ostringstream oss;
                oss << "property 'slot' on component child '" << normalizedChild.typeName
                    << "' must be a string or identifier at line " << propIt->line;
                warnings_.push_back(oss.str());
                explicitSlotName = std::string();
            } else {
                explicitSlotName = *slotName;
            }
            propIt = normalizedChild.properties.erase(propIt);
        }

        if (!templateHasSlots) {
            if (explicitSlotName.has_value() && !explicitSlotName->empty()) {
                std::ostringstream oss;
                oss << slotLabel(*explicitSlotName) << " passed to component '"
                    << component.name << "' at line " << normalizedChild.line
                    << ", but template declares no Slot()";
                warnings_.push_back(oss.str());
                continue;
            }

            legacyChildren.push_back(std::move(normalizedChild));
            continue;
        }

        const std::string slotName = explicitSlotName.value_or(std::string());
        slotChildren[slotName].push_back(std::move(normalizedChild));
    }

    if (component.root.typeName == "Slot") {
        std::ostringstream oss;
        oss << "component '" << component.name << "' cannot use Slot as its root node at line "
            << component.root.line;
        warnings_.push_back(oss.str());
        return {};
    }

    std::unordered_set<std::string> declaredSlots;
    AstNode merged = resolveComponentTemplateNode(
        component.root,
        parameterValues,
        slotChildren,
        declaredSlots);
    merged.line = instanceNode.line;
    merged.column = instanceNode.column;

    applyNodePropertyOverrides(merged, rootOverrides);

    if (!templateHasSlots && !legacyChildren.empty()) {
        merged.children.insert(
            merged.children.end(),
            legacyChildren.begin(),
            legacyChildren.end());
    }

    if (templateHasSlots) {
        for (const auto& [slotName, slotNodes] : slotChildren) {
            if (slotNodes.empty() || declaredSlots.contains(slotName)) {
                continue;
            }

            std::ostringstream oss;
            oss << slotLabel(slotName) << " passed to component '" << component.name
                << "' at line " << instanceNode.line << " has no matching Slot()";
            warnings_.push_back(oss.str());
        }
    }

    return merged;
}

AstNode LayoutBuilder::resolveComponentTemplateNode(
    const AstNode& templateNode,
    const std::unordered_map<std::string, AstProperty>& parameterValues,
    const std::unordered_map<std::string, std::vector<AstNode>>& slotChildren,
    std::unordered_set<std::string>& declaredSlots)
{
    AstNode resolved = templateNode;
    resolved.properties.clear();
    resolved.properties.reserve(templateNode.properties.size());
    for (const auto& property : templateNode.properties) {
        resolved.properties.push_back(resolveComponentProperty(property, parameterValues));
    }

    resolved.children.clear();
    for (const auto& child : templateNode.children) {
        appendResolvedComponentChild(
            child,
            parameterValues,
            slotChildren,
            declaredSlots,
            resolved.children);
    }

    return resolved;
}

AstProperty LayoutBuilder::resolveComponentProperty(
    const AstProperty& property,
    const std::unordered_map<std::string, AstProperty>& parameterValues) const
{
    AstProperty resolved = property;
    if (property.hasObjectValue()) {
        resolved.objectProperties.clear();
        resolved.objectProperties.reserve(property.objectProperties.size());
        for (const auto& childProperty : property.objectProperties) {
            resolved.objectProperties.push_back(
                resolveComponentProperty(childProperty, parameterValues));
        }
        return resolved;
    }

    if (property.value.type() == core::ValueType::Enum) {
        const auto parameterIt = parameterValues.find(property.value.asString());
        if (parameterIt != parameterValues.end()) {
            AstProperty parameterProp = parameterIt->second;
            parameterProp.name = property.name;
            parameterProp.line = property.line;
            parameterProp.column = property.column;
            return parameterProp;
        }
    }

    resolved.value = resolveComponentValue(property.value, parameterValues);
    return resolved;
}

void LayoutBuilder::appendResolvedComponentChild(
    const AstNode& templateNode,
    const std::unordered_map<std::string, AstProperty>& parameterValues,
    const std::unordered_map<std::string, std::vector<AstNode>>& slotChildren,
    std::unordered_set<std::string>& declaredSlots,
    std::vector<AstNode>& outChildren)
{
    if (templateNode.typeName != "Slot") {
        outChildren.push_back(resolveComponentTemplateNode(
            templateNode,
            parameterValues,
            slotChildren,
            declaredSlots));
        return;
    }

    const std::string slotName = resolveSlotName(templateNode, parameterValues);
    declaredSlots.insert(slotName);

    const auto providedChildren = slotChildren.find(slotName);
    if (providedChildren != slotChildren.end() && !providedChildren->second.empty()) {
        outChildren.insert(
            outChildren.end(),
            providedChildren->second.begin(),
            providedChildren->second.end());
        return;
    }

    for (const auto& fallbackChild : templateNode.children) {
        appendResolvedComponentChild(
            fallbackChild,
            parameterValues,
            slotChildren,
            declaredSlots,
            outChildren);
    }
}

core::Value LayoutBuilder::resolveComponentValue(
    const core::Value& value,
    const std::unordered_map<std::string, AstProperty>& parameterValues) const
{
    if (value.type() != core::ValueType::Enum) {
        return value;
    }

    const auto parameterIt = parameterValues.find(value.asString());
    if (parameterIt == parameterValues.end()) {
        return value;
    }

    if (parameterIt->second.hasObjectValue() || parameterIt->second.hasBinding()) {
        return value;
    }

    return parameterIt->second.value;
}

void LayoutBuilder::applyNodePropertyOverrides(
    AstNode& node,
    const std::vector<AstProperty>& overrideProperties) const
{
    for (const auto& overrideProp : overrideProperties) {
        bool replaced = false;
        for (auto& nodeProp : node.properties) {
            if (nodeProp.name == overrideProp.name) {
                nodeProp = overrideProp;
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            node.properties.push_back(overrideProp);
        }
    }
}

bool LayoutBuilder::containsSlotNode(const AstNode& node) const
{
    if (node.typeName == "Slot") {
        return true;
    }

    for (const auto& child : node.children) {
        if (containsSlotNode(child)) {
            return true;
        }
    }

    return false;
}

std::string LayoutBuilder::resolveSlotName(
    const AstNode& slotNode,
    const std::unordered_map<std::string, AstProperty>& parameterValues)
{
    std::string slotName;

    for (const auto& prop : slotNode.properties) {
        if (prop.name != "name") {
            std::ostringstream oss;
            oss << "unknown property '" << prop.name << "' on Slot at line " << prop.line;
            warnings_.push_back(oss.str());
            continue;
        }

        if (prop.hasObjectValue()) {
            std::ostringstream oss;
            oss << "property 'name' on Slot does not accept object values at line "
                << prop.line;
            warnings_.push_back(oss.str());
            continue;
        }

        const core::Value resolvedValue = resolveComponentValue(prop.value, parameterValues);
        const std::optional<std::string> resolvedName = stringLikeValue(resolvedValue);
        if (!resolvedName) {
            std::ostringstream oss;
            oss << "property 'name' on Slot must be a string or identifier at line "
                << prop.line;
            warnings_.push_back(oss.str());
            continue;
        }

        slotName = *resolvedName;
    }

    return slotName;
}

void LayoutBuilder::applyInlineStyle(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const AstProperty& prop)
{
    applyStyleProperties(
        widget,
        nodeType,
        prop.objectProperties,
        "inline style on '" + nodeType + "'");
}

void LayoutBuilder::applyStyleProperties(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const std::vector<AstProperty>& properties,
    const std::string& context)
{
    for (const auto& styleProp : properties) {
        if (styleProp.name == "style") {
            if (styleProp.hasBinding()) {
                std::ostringstream oss;
                oss << "dynamic style references are not supported in " << context
                    << " at line " << styleProp.line;
                warnings_.push_back(oss.str());
                continue;
            }

            if (styleProp.hasObjectValue()) {
                applyStyleProperties(widget, nodeType, styleProp.objectProperties, context);
            } else {
                applyNamedStyle(widget, nodeType, styleProp);
            }
            continue;
        }

        if (styleProp.hasObjectValue()) {
            std::ostringstream oss;
            oss << "nested object style property '" << styleProp.name << "' in " << context
                << " at line " << styleProp.line << " is not supported";
            warnings_.push_back(oss.str());
            continue;
        }

        if (styleProp.hasBinding()) {
            AstProperty boundStyleProp = styleProp;
            boundStyleProp.bindingPath.reset();
            registerBinding(
                widget,
                styleProp.name,
                *styleProp.bindingPath,
                stylePropertyBindingType(styleProp.name),
                [weakWidget = std::weak_ptr<ui::Widget>(widget),
                 theme = theme_,
                 nodeType,
                 boundStyleProp](const core::Value& value) mutable {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    boundStyleProp.value = value;
                    LayoutBuilder builder(theme);
                    builder.applyStyleProperty(*lockedWidget, nodeType, boundStyleProp);
                });
            continue;
        }

        if (!applyStyleProperty(*widget, nodeType, styleProp)) {
            std::ostringstream oss;
            oss << "unsupported style property '" << styleProp.name << "' in " << context
                << " for '" << nodeType << "' at line " << styleProp.line;
            warnings_.push_back(oss.str());
        }
    }
}

void LayoutBuilder::applyNamedStyle(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const AstProperty& prop)
{
    if (prop.hasObjectValue()) {
        std::ostringstream oss;
        oss << "style reference on '" << nodeType << "' must be a string or identifier at line "
            << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    if (prop.hasBinding()) {
        std::ostringstream oss;
        oss << "dynamic style references on '" << nodeType << "' are not supported at line "
            << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    const std::optional<std::string> styleName = stringLikeValue(prop.value);
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

    if (std::find(styleStack_.begin(), styleStack_.end(), style.name) != styleStack_.end()) {
        std::ostringstream oss;
        oss << "cyclic style reference '" << style.name << "' on '" << nodeType
            << "' at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    styleStack_.push_back(style.name);
    applyStyleProperties(widget, nodeType, style.properties, "style '" + style.name + "'");
    styleStack_.pop_back();
}

void LayoutBuilder::applyStandardProperty(
    const std::shared_ptr<ui::Widget>& widget,
    const core::TypeInfo& typeInfo,
    const std::string& nodeType,
    const AstProperty& prop)
{
    if (prop.hasObjectValue()) {
        std::ostringstream oss;
        oss << "property '" << prop.name << "' on '" << nodeType
            << "' does not accept object values at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    if (prop.name == "id") {
        if (prop.hasBinding()) {
            std::ostringstream oss;
            oss << "property 'id' on '" << nodeType
                << "' does not support data binding at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

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
        if (prop.hasBinding()) {
            registerBinding(
                widget,
                prop.name,
                *prop.bindingPath,
                core::ValueType::Bool,
                [weakWidget = std::weak_ptr<ui::Widget>(widget)](const core::Value& value) {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    lockedWidget->setVisible(value.asBool());
                });
            return;
        }

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

    if (prop.hasBinding()) {
        registerBinding(
            widget,
            prop.name,
            *prop.bindingPath,
            propInfo->expectedType,
            [weakWidget = std::weak_ptr<ui::Widget>(widget),
             setter = propInfo->setter](const core::Value& value) {
                auto lockedWidget = weakWidget.lock();
                if (!lockedWidget) {
                    return;
                }

                setter(*lockedWidget, value);
            });
        return;
    }

    propInfo->setter(*widget, prop.value);
}

void LayoutBuilder::registerBinding(
    const std::shared_ptr<ui::Widget>& widget,
    std::string propertyName,
    std::string path,
    core::ValueType expectedType,
    std::function<void(const core::Value&)> apply)
{
    bindings_.push_back(detail::BindingDescriptor {
        .widget = widget,
        .propertyName = std::move(propertyName),
        .path = std::move(path),
        .expectedType = expectedType,
        .apply = std::move(apply),
    });
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
