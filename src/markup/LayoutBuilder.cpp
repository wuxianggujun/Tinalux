#include "tinalux/markup/LayoutBuilder.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>

#include "BindingExpression.h"
#include "tinalux/core/Reflect.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/RichText.h"
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

std::optional<std::vector<std::string>> stringArrayNode(const ModelNode& node)
{
    const ModelNode::Array* array = node.arrayValue();
    if (array == nullptr) {
        return std::nullopt;
    }

    std::vector<std::string> items;
    items.reserve(array->size());
    for (const ModelNode& entry : *array) {
        const core::Value* scalar = entry.scalar();
        if (scalar == nullptr) {
            return std::nullopt;
        }

        const std::optional<std::string> item = stringLikeValue(*scalar);
        if (!item.has_value()) {
            return std::nullopt;
        }

        items.push_back(*item);
    }

    return items;
}

void installStringListViewSource(ui::ListView& listView, std::vector<std::string> items)
{
    auto sharedItems = std::make_shared<std::vector<std::string>>(std::move(items));
    listView.setItemFactory(
        sharedItems->size(),
        [sharedItems](
            std::size_t index,
            std::shared_ptr<ui::Widget> recycledItem) -> std::shared_ptr<ui::Widget> {
            if (index >= sharedItems->size()) {
                return {};
            }

            std::shared_ptr<ui::Label> label = std::dynamic_pointer_cast<ui::Label>(recycledItem);
            if (!label) {
                label = std::make_shared<ui::Label>((*sharedItems)[index]);
            } else {
                label->setText((*sharedItems)[index]);
            }
            return label;
        });
}

std::optional<ui::RichTextSpanRole> richTextSpanRoleFromValue(const core::Value& value)
{
    const std::optional<std::string> roleName = stringLikeValue(value);
    if (!roleName.has_value()) {
        return std::nullopt;
    }

    if (*roleName == "Body") {
        return ui::RichTextSpanRole::Body;
    }
    if (*roleName == "Paragraph") {
        return ui::RichTextSpanRole::Paragraph;
    }
    if (*roleName == "Heading") {
        return ui::RichTextSpanRole::Heading;
    }
    if (*roleName == "Caption") {
        return ui::RichTextSpanRole::Caption;
    }
    if (*roleName == "InlineCode") {
        return ui::RichTextSpanRole::InlineCode;
    }
    if (*roleName == "Quote") {
        return ui::RichTextSpanRole::Quote;
    }
    if (*roleName == "CodeBlock") {
        return ui::RichTextSpanRole::CodeBlock;
    }
    if (*roleName == "BulletItem") {
        return ui::RichTextSpanRole::BulletItem;
    }
    if (*roleName == "OrderedItem") {
        return ui::RichTextSpanRole::OrderedItem;
    }

    return std::nullopt;
}

std::optional<float> numericNodeValue(const ModelNode* node)
{
    const core::Value* value = node != nullptr ? node->scalar() : nullptr;
    if (value == nullptr) {
        return std::nullopt;
    }

    if (value->type() != core::ValueType::Int && value->type() != core::ValueType::Float) {
        return std::nullopt;
    }

    return value->asNumber();
}

std::optional<bool> boolNodeValue(const ModelNode* node)
{
    const core::Value* value = node != nullptr ? node->scalar() : nullptr;
    if (value == nullptr || value->type() != core::ValueType::Bool) {
        return std::nullopt;
    }

    return value->asBool();
}

std::optional<std::vector<ui::TextSpan>> richTextSpansNode(const ModelNode& node)
{
    const ModelNode::Array* array = node.arrayValue();
    if (array == nullptr) {
        return std::nullopt;
    }

    std::vector<ui::TextSpan> spans;
    spans.reserve(array->size());
    for (const ModelNode& entry : *array) {
        if (entry.objectValue() == nullptr) {
            return std::nullopt;
        }

        const ModelNode* textNode = entry.child("text");
        const core::Value* textValue = textNode != nullptr ? textNode->scalar() : nullptr;
        const std::optional<std::string> text =
            textValue != nullptr ? stringLikeValue(*textValue) : std::nullopt;
        if (!text.has_value()) {
            return std::nullopt;
        }

        ui::TextSpan span;
        span.text = *text;

        if (const ModelNode* roleNode = entry.child("role")) {
            const core::Value* roleValue = roleNode->scalar();
            const std::optional<ui::RichTextSpanRole> role =
                roleValue != nullptr ? richTextSpanRoleFromValue(*roleValue) : std::nullopt;
            if (!role.has_value()) {
                return std::nullopt;
            }
            span.role = *role;
        }

        if (const ModelNode* colorNode = entry.child("color")) {
            const core::Value* colorValue = colorNode->scalar();
            if (colorValue == nullptr || colorValue->type() != core::ValueType::Color) {
                return std::nullopt;
            }
            span.color = colorValue->asColor();
        }

        if (const ModelNode* backgroundColorNode = entry.child("backgroundColor")) {
            const core::Value* backgroundColorValue = backgroundColorNode->scalar();
            if (backgroundColorValue == nullptr
                || backgroundColorValue->type() != core::ValueType::Color) {
                return std::nullopt;
            }
            span.backgroundColor = backgroundColorValue->asColor();
        }

        if (const std::optional<float> fontSize = numericNodeValue(entry.child("fontSize"))) {
            span.fontSize = *fontSize;
        } else if (entry.child("fontSize") != nullptr) {
            return std::nullopt;
        }

        if (const std::optional<float> letterSpacing = numericNodeValue(entry.child("letterSpacing"))) {
            span.letterSpacing = *letterSpacing;
        } else if (entry.child("letterSpacing") != nullptr) {
            return std::nullopt;
        }

        if (const std::optional<float> wordSpacing = numericNodeValue(entry.child("wordSpacing"))) {
            span.wordSpacing = *wordSpacing;
        } else if (entry.child("wordSpacing") != nullptr) {
            return std::nullopt;
        }

        if (const ModelNode* fontFamiliesNode = entry.child("fontFamilies")) {
            const std::optional<std::vector<std::string>> fontFamilies =
                stringArrayNode(*fontFamiliesNode);
            if (!fontFamilies.has_value()) {
                return std::nullopt;
            }
            span.fontFamilies = *fontFamilies;
        }

        if (const ModelNode* blockMarkerNode = entry.child("blockMarker")) {
            const core::Value* blockMarkerValue = blockMarkerNode->scalar();
            const std::optional<std::string> blockMarker =
                blockMarkerValue != nullptr ? stringLikeValue(*blockMarkerValue) : std::nullopt;
            if (!blockMarker.has_value()) {
                return std::nullopt;
            }
            span.blockMarker = *blockMarker;
        }

        if (const std::optional<float> blockLevel = numericNodeValue(entry.child("blockLevel"))) {
            if (*blockLevel < 0.0f) {
                return std::nullopt;
            }
            span.blockLevel = static_cast<std::size_t>(*blockLevel);
        } else if (entry.child("blockLevel") != nullptr) {
            return std::nullopt;
        }

        if (const std::optional<bool> bold = boolNodeValue(entry.child("bold"))) {
            span.bold = *bold;
        } else if (entry.child("bold") != nullptr) {
            return std::nullopt;
        }

        if (const std::optional<bool> italic = boolNodeValue(entry.child("italic"))) {
            span.italic = *italic;
        } else if (entry.child("italic") != nullptr) {
            return std::nullopt;
        }

        if (const std::optional<bool> underline = boolNodeValue(entry.child("underline"))) {
            span.underline = *underline;
        } else if (entry.child("underline") != nullptr) {
            return std::nullopt;
        }

        if (const std::optional<bool> strikethrough = boolNodeValue(entry.child("strikethrough"))) {
            span.strikethrough = *strikethrough;
        } else if (entry.child("strikethrough") != nullptr) {
            return std::nullopt;
        }

        if (const ModelNode* onClickNode = entry.child("onClick")) {
            const ModelNode::Action* action = onClickNode->actionValue();
            if (action == nullptr || !*action) {
                return std::nullopt;
            }

            span.onClick = [action = *action]() {
                action(core::Value());
            };
        }

        spans.push_back(std::move(span));
    }

    return spans;
}

std::optional<std::string> interactionNameFromPropertyName(std::string_view propertyName)
{
    if (propertyName.size() <= 2
        || propertyName[0] != 'o'
        || propertyName[1] != 'n'
        || !std::isupper(static_cast<unsigned char>(propertyName[2]))) {
        return std::nullopt;
    }

    std::string interactionName(propertyName.substr(2));
    interactionName[0] = static_cast<char>(
        std::tolower(static_cast<unsigned char>(interactionName[0])));
    return interactionName;
}

std::string slotLabel(std::string_view slotName)
{
    if (slotName.empty()) {
        return "default slot";
    }

    return "slot '" + std::string(slotName) + "'";
}

std::string normalizeScopedPath(std::string_view path)
{
    std::size_t start = 0;
    while (start < path.size()
        && (path[start] == ' ' || path[start] == '\t'
            || path[start] == '\r' || path[start] == '\n')) {
        ++start;
    }

    std::size_t end = path.size();
    while (end > start
        && (path[end - 1] == ' ' || path[end - 1] == '\t'
            || path[end - 1] == '\r' || path[end - 1] == '\n')) {
        --end;
    }

    std::string normalized(path.substr(start, end - start));
    constexpr std::string_view kModelPrefix = "model.";
    if (normalized.compare(0, kModelPrefix.size(), kModelPrefix) == 0) {
        normalized.erase(0, kModelPrefix.size());
    }
    return normalized;
}

std::vector<std::string_view> splitScopedPath(std::string_view path)
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (start < path.size()) {
        std::size_t end = path.find('.', start);
        if (end == std::string_view::npos) {
            parts.push_back(path.substr(start));
            break;
        }

        parts.push_back(path.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

bool truthyScalar(const core::Value& value)
{
    switch (value.type()) {
    case core::ValueType::Bool:
        return value.asBool();
    case core::ValueType::Int:
        return value.asInt() != 0;
    case core::ValueType::Float:
        return std::abs(value.asFloat()) > 0.0001f;
    case core::ValueType::String:
    case core::ValueType::Enum:
        return !value.asString().empty();
    case core::ValueType::Color:
        return value.asColor().value() != 0;
    case core::ValueType::None:
        return false;
    }

    return false;
}

bool isArrayIndexSegment(std::string_view segment)
{
    if (segment.empty()) {
        return false;
    }

    for (char ch : segment) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    return true;
}

std::optional<std::size_t> parseArrayIndex(std::string_view segment)
{
    if (!isArrayIndexSegment(segment)) {
        return std::nullopt;
    }

    std::size_t value = 0;
    for (char ch : segment) {
        value = value * 10 + static_cast<std::size_t>(ch - '0');
    }

    return value;
}

const ModelNode* findChildNode(const ModelNode* node, std::string_view segment)
{
    if (node == nullptr) {
        return nullptr;
    }

    if (node->isObject()) {
        return node->child(segment);
    }

    if (!node->isArray()) {
        return nullptr;
    }

    const std::optional<std::size_t> index = parseArrayIndex(segment);
    const ModelNode::Array* array = node->arrayValue();
    if (!index.has_value() || array == nullptr || *index >= array->size()) {
        return nullptr;
    }

    return &(*array)[*index];
}

const ModelNode* resolveLocalNode(
    std::string_view normalizedPath,
    const std::unordered_map<std::string, const ModelNode*>& scope)
{
    if (normalizedPath.empty()) {
        return nullptr;
    }

    const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
    if (parts.empty()) {
        return nullptr;
    }

    const auto localIt = scope.find(std::string(parts.front()));
    if (localIt == scope.end()) {
        return nullptr;
    }

    const ModelNode* currentNode = localIt->second;
    for (std::size_t index = 1; currentNode != nullptr && index < parts.size(); ++index) {
        currentNode = findChildNode(currentNode, parts[index]);
    }

    return currentNode;
}

const ModelNode* resolveLocalSnapshotNode(
    std::string_view normalizedPath,
    const std::unordered_map<std::string, ModelNode>& scope)
{
    if (normalizedPath.empty()) {
        return nullptr;
    }

    const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
    if (parts.empty()) {
        return nullptr;
    }

    const auto localIt = scope.find(std::string(parts.front()));
    if (localIt == scope.end()) {
        return nullptr;
    }

    const ModelNode* currentNode = &localIt->second;
    for (std::size_t index = 1; currentNode != nullptr && index < parts.size(); ++index) {
        currentNode = findChildNode(currentNode, parts[index]);
    }

    return currentNode;
}

std::optional<core::Value> readWidgetPropertyValue(
    const ui::Widget& widget,
    std::string_view propertyName)
{
    if (propertyName == "visible") {
        return core::Value(widget.visible());
    }

    if (propertyName == "enabled") {
        return core::Value(widget.enabled());
    }

    const std::string& typeName = widget.markupTypeName();
    if (typeName.empty()) {
        return std::nullopt;
    }

    const core::TypeInfo* typeInfo = core::TypeRegistry::instance().findType(typeName);
    if (typeInfo == nullptr) {
        return std::nullopt;
    }

    const core::PropertyInfo* propertyInfo = typeInfo->findProperty(propertyName);
    if (propertyInfo == nullptr || !propertyInfo->getter) {
        return std::nullopt;
    }

    return propertyInfo->getter(widget);
}

const ModelNode* resolveWidgetSnapshotNode(
    const std::unordered_map<std::string, std::shared_ptr<ui::Widget>>& idMap,
    std::string_view path,
    std::unordered_map<std::string, ModelNode>& widgetNodeCache)
{
    const std::vector<std::string_view> parts = splitScopedPath(path);
    if (parts.size() != 2) {
        return nullptr;
    }

    const auto widgetIt = idMap.find(std::string(parts.front()));
    if (widgetIt == idMap.end() || widgetIt->second == nullptr) {
        return nullptr;
    }

    const std::string cacheKey(path);
    const auto cacheIt = widgetNodeCache.find(cacheKey);
    if (cacheIt != widgetNodeCache.end()) {
        return &cacheIt->second;
    }

    const std::optional<core::Value> widgetValue =
        readWidgetPropertyValue(*widgetIt->second, parts.back());
    if (!widgetValue.has_value()) {
        return nullptr;
    }

    auto [insertedIt, inserted] = widgetNodeCache.emplace(cacheKey, ModelNode(*widgetValue));
    (void)inserted;
    return &insertedIt->second;
}

std::optional<core::Value> coerceInitialBindingValue(
    const core::Value& value,
    core::ValueType expectedType)
{
    if (expectedType == core::ValueType::None || value.type() == expectedType) {
        return value;
    }

    switch (expectedType) {
    case core::ValueType::None:
        return value;

    case core::ValueType::Bool:
        if (value.type() == core::ValueType::Int) {
            return core::Value(value.asInt() != 0);
        }
        if (value.type() == core::ValueType::Float) {
            return core::Value(value.asFloat() != 0.0f);
        }
        return std::nullopt;

    case core::ValueType::Int:
        if (value.type() == core::ValueType::Float) {
            return core::Value(static_cast<int>(value.asFloat()));
        }
        return std::nullopt;

    case core::ValueType::Float:
        if (value.type() == core::ValueType::Int) {
            return core::Value(static_cast<float>(value.asInt()));
        }
        return std::nullopt;

    case core::ValueType::String:
    case core::ValueType::Color:
    case core::ValueType::Enum:
        return std::nullopt;
    }

    return std::nullopt;
}

void appendUniquePath(std::vector<std::string>& paths, std::string path)
{
    if (path.empty()) {
        return;
    }

    if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
        paths.push_back(std::move(path));
    }
}

bool isBindingIdentifierStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool isBindingIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.';
}

std::string escapeBindingStringLiteral(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    escaped.push_back('"');
    return escaped;
}

std::optional<std::string> componentControlLiteral(const core::Value& value)
{
    switch (value.type()) {
    case core::ValueType::None:
        return std::nullopt;
    case core::ValueType::Bool:
        return value.asBool() ? "true" : "false";
    case core::ValueType::Int:
        return std::to_string(value.asInt());
    case core::ValueType::Float: {
        std::ostringstream oss;
        oss << value.asFloat();
        return oss.str();
    }
    case core::ValueType::String:
    case core::ValueType::Enum:
        return escapeBindingStringLiteral(value.asString());
    case core::ValueType::Color: {
        std::ostringstream oss;
        oss << '#'
            << std::uppercase
            << std::hex
            << std::setw(8)
            << std::setfill('0')
            << value.asColor().value();
        return oss.str();
    }
    }

    return std::nullopt;
}

std::optional<std::string> resolveComponentControlToken(
    std::string_view token,
    const std::unordered_map<std::string, AstProperty>& parameterValues)
{
    const std::size_t suffixOffset = token.find('.');
    const std::string_view root = token.substr(0, suffixOffset);
    const auto parameterIt = parameterValues.find(std::string(root));
    if (parameterIt == parameterValues.end()) {
        return std::nullopt;
    }

    const std::string_view suffix = suffixOffset == std::string_view::npos
        ? std::string_view {}
        : token.substr(suffixOffset);
    const AstProperty& parameter = parameterIt->second;
    if (parameter.hasBinding()) {
        if (suffix.empty()) {
            return "(" + *parameter.bindingPath + ")";
        }

        const auto bindingExpression =
            detail::BindingExpression::compile(*parameter.bindingPath, nullptr);
        if (!bindingExpression || !bindingExpression->isDirectPath()) {
            return std::nullopt;
        }

        return "(" + std::string(bindingExpression->directPath()) + std::string(suffix) + ")";
    }

    if (parameter.hasObjectValue() || parameter.hasArrayValue() || !suffix.empty()) {
        return std::nullopt;
    }

    const std::optional<std::string> literal = componentControlLiteral(parameter.value);
    if (!literal.has_value()) {
        return std::nullopt;
    }

    return "(" + *literal + ")";
}

std::string resolveComponentControlExpression(
    std::string_view expressionText,
    const std::unordered_map<std::string, AstProperty>& parameterValues)
{
    std::string resolved;
    resolved.reserve(expressionText.size());

    for (std::size_t index = 0; index < expressionText.size();) {
        const char ch = expressionText[index];
        if (ch == '"') {
            resolved.push_back(expressionText[index++]);
            while (index < expressionText.size()) {
                const char stringChar = expressionText[index++];
                resolved.push_back(stringChar);
                if (stringChar == '\\' && index < expressionText.size()) {
                    resolved.push_back(expressionText[index++]);
                    continue;
                }
                if (stringChar == '"') {
                    break;
                }
            }
            continue;
        }

        if (!isBindingIdentifierStart(ch)) {
            resolved.push_back(ch);
            ++index;
            continue;
        }

        const std::size_t start = index;
        ++index;
        while (index < expressionText.size() && isBindingIdentifierChar(expressionText[index])) {
            ++index;
        }

        const std::string_view token = expressionText.substr(start, index - start);
        const std::optional<std::string> replacement =
            resolveComponentControlToken(token, parameterValues);
        if (replacement.has_value()) {
            resolved += *replacement;
        } else {
            resolved.append(token);
        }
    }

    return resolved;
}

std::shared_ptr<const detail::BindingExpression> compileBindingExpression(
    std::string_view expressionText,
    int line,
    std::string_view context,
    std::vector<std::string>* warnings)
{
    std::string errorMessage;
    auto expression = detail::BindingExpression::compile(expressionText, &errorMessage);
    if (expression || warnings == nullptr) {
        return expression;
    }

    std::ostringstream oss;
    oss << "invalid binding expression '${" << expressionText << "}'";
    if (!context.empty()) {
        oss << " in " << context;
    }
    if (line > 0) {
        oss << " at line " << line;
    }
    if (!errorMessage.empty()) {
        oss << ": " << errorMessage;
    }
    warnings->push_back(oss.str());
    return {};
}

} // namespace

LayoutBuilder::LayoutBuilder(const ui::Theme& theme, std::shared_ptr<ViewModel> viewModel)
    : theme_(theme)
    , viewModel_(std::move(viewModel))
{
}

BuildResult LayoutBuilder::build(
    const AstNode& ast,
    const ui::Theme& theme,
    const std::shared_ptr<ViewModel>& viewModel)
{
    AstDocument document;
    document.root = ast;
    return build(document, theme, viewModel);
}

BuildResult LayoutBuilder::build(
    const AstDocument& document,
    const ui::Theme& theme,
    const std::shared_ptr<ViewModel>& viewModel)
{
    LayoutBuilder builder(theme, viewModel);
    builder.registerLets(document.lets);
    builder.registerStyles(document.styles);
    builder.registerComponents(document.components);
    BuildResult result;
    if (document.root && document.root->isWidget()) {
        result.root = builder.buildNode(*document.root, LayoutBuilder::ScopeBindings {});
    } else if (document.root && !document.root->isWidget()) {
        result.warnings.push_back("markup document root must be a widget node");
    }
    result.idMap = std::move(builder.idMap_);
    result.bindings = std::move(builder.bindings_);
    result.interactionBindings = std::move(builder.interactionBindings_);
    result.structuralPaths = std::move(builder.structuralPaths_);
    result.warnings = std::move(builder.warnings_);
    return result;
}

void LayoutBuilder::registerLets(const std::vector<AstProperty>& lets)
{
    for (const auto& letProperty : lets) {
        if (letProperty.name.empty()) {
            warnings_.push_back("encountered let definition without a name");
            continue;
        }

        if (letMap_.contains(letProperty.name)) {
            warnings_.push_back("duplicate let '" + letProperty.name + "'");
        }

        letMap_[letProperty.name] = letProperty;
    }
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

std::shared_ptr<ui::Widget> LayoutBuilder::buildNode(
    const AstNode& node,
    const ScopeBindings& scope)
{
    if (!node.isWidget()) {
        std::ostringstream oss;
        oss << "control directive cannot be materialized as a standalone widget at line "
            << node.line;
        warnings_.push_back(oss.str());
        return nullptr;
    }

    const auto componentIt = componentMap_.find(node.typeName);
    if (componentIt != componentMap_.end()) {
        return buildComponentNode(componentIt->second, node, scope);
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
    widget->setMarkupTypeName(node.typeName);

    const std::vector<AstProperty> normalizedProperties = normalizeImplicitProperties(
        node.properties,
        node.typeName,
        typeInfo->markupPositionalProperties);

    for (const auto& prop : normalizedProperties) {
        const AstProperty resolvedProp = resolveScopedProperty(prop, scope);
        if (resolvedProp.name == "style") {
            if (resolvedProp.hasObjectValue()) {
                applyInlineStyle(widget, node.typeName, resolvedProp, scope);
            } else {
                applyNamedStyle(widget, node.typeName, resolvedProp, scope);
            }
        }
    }

    std::vector<AstProperty> applicationProperties = normalizedProperties;
    std::stable_sort(
        applicationProperties.begin(),
        applicationProperties.end(),
        [typeInfo](const AstProperty& lhs, const AstProperty& rhs) {
            const core::PropertyInfo* lhsInfo = typeInfo->findProperty(lhs.name);
            const core::PropertyInfo* rhsInfo = typeInfo->findProperty(rhs.name);
            const int lhsOrder = lhsInfo != nullptr ? lhsInfo->applicationOrder : 0;
            const int rhsOrder = rhsInfo != nullptr ? rhsInfo->applicationOrder : 0;
            return lhsOrder < rhsOrder;
        });

    for (const auto& prop : applicationProperties) {
        const AstProperty resolvedProp = resolveScopedProperty(prop, scope);
        if (resolvedProp.name == "style") {
            continue;
        }
        applyStandardProperty(widget, *typeInfo, node.typeName, resolvedProp, scope);
    }

    if (!attachChildren(widget, *typeInfo, node, scope) && !node.children.empty()) {
        std::ostringstream oss;
        oss << "'" << node.typeName << "' is not a container but has children at line "
            << node.line;
        warnings_.push_back(oss.str());
    }

    return widget;
}

std::vector<AstProperty> LayoutBuilder::normalizeImplicitProperties(
    const std::vector<AstProperty>& properties,
    std::string_view ownerName,
    const std::vector<std::string>& positionalPropertyNames)
{
    std::vector<AstProperty> normalized;
    normalized.reserve(properties.size());

    std::unordered_set<std::string> filledPositionalNames;
    filledPositionalNames.reserve(positionalPropertyNames.size());
    std::size_t positionalIndex = 0;
    for (const auto& property : properties) {
        if (!property.hasImplicitName()) {
            normalized.push_back(property);
            if (std::find(
                    positionalPropertyNames.begin(),
                    positionalPropertyNames.end(),
                    property.name) != positionalPropertyNames.end()) {
                filledPositionalNames.insert(property.name);
            }
            continue;
        }

        while (positionalIndex < positionalPropertyNames.size()
            && filledPositionalNames.contains(positionalPropertyNames[positionalIndex])) {
            ++positionalIndex;
        }

        if (positionalIndex >= positionalPropertyNames.size()) {
            std::ostringstream oss;
            oss << "anonymous value syntax is not supported on '" << ownerName
                << "' at line " << property.line;
            warnings_.push_back(oss.str());
            continue;
        }

        AstProperty resolved = property;
        resolved.name = positionalPropertyNames[positionalIndex];
        resolved.implicitName = false;
        filledPositionalNames.insert(resolved.name);
        normalized.push_back(std::move(resolved));
        ++positionalIndex;
    }

    return normalized;
}

std::shared_ptr<ui::Widget> LayoutBuilder::buildComponentNode(
    const AstComponentDefinition& component,
    const AstNode& instanceNode,
    const ScopeBindings& scope)
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
        widget = buildNode(mergedNode, scope);
    }
    componentStack_.pop_back();
    return widget;
}

AstNode LayoutBuilder::mergeComponentNode(
    const AstComponentDefinition& component,
    const AstNode& instanceNode)
{
    std::vector<std::string> componentPositionalProperties;
    componentPositionalProperties.reserve(component.parameters.size());
    for (const auto& parameter : component.parameters) {
        componentPositionalProperties.push_back(parameter.name);
    }

    const std::vector<AstProperty> normalizedInstanceProperties = normalizeImplicitProperties(
        instanceNode.properties,
        component.name,
        componentPositionalProperties);

    std::unordered_map<std::string, AstProperty> parameterValues;
    parameterValues.reserve(component.parameters.size());
    for (const auto& parameter : component.parameters) {
        parameterValues[parameter.name] = parameter;
    }

    std::vector<AstProperty> rootOverrides;
    rootOverrides.reserve(normalizedInstanceProperties.size());
    for (const auto& instanceProp : normalizedInstanceProperties) {
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
            std::ostringstream oss;
            if (explicitSlotName.has_value() && !explicitSlotName->empty()) {
                oss << slotLabel(*explicitSlotName);
            } else {
                oss << "child '" << normalizedChild.typeName << "'";
            }
            oss << " passed to component '" << component.name << "' at line "
                << normalizedChild.line << ", but template declares no Slot()";
            warnings_.push_back(oss.str());
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

std::size_t LayoutBuilder::visitExpandedNodes(
    const std::vector<AstNode>& sourceNodes,
    const ScopeBindings& scope,
    const std::function<void(const AstNode&, const ScopeBindings&)>& visitor)
{
    std::size_t expandedCount = 0;
    for (const auto& node : sourceNodes) {
        if (node.isWidget()) {
            ++expandedCount;
            visitor(node, scope);
            continue;
        }

        if (node.isIfBlock()) {
            if (node.controlPath.has_value() && !node.controlPath->empty()) {
                trackStructuralDependencies(*node.controlPath, node.line, "if condition", scope);
            }
            for (const auto& conditionalBranch : node.conditionalBranches) {
                if (conditionalBranch.controlPath.has_value() && !conditionalBranch.controlPath->empty()) {
                    trackStructuralDependencies(
                        *conditionalBranch.controlPath,
                        conditionalBranch.line,
                        "elseif condition",
                        scope);
                }
            }

            if (node.controlPath.has_value()
                && evaluateConditionExpression(
                    *node.controlPath,
                    node.line,
                    "if condition",
                    scope)) {
                expandedCount += visitExpandedNodes(node.children, scope, visitor);
                continue;
            }

            for (const auto& conditionalBranch : node.conditionalBranches) {
                if (conditionalBranch.controlPath.has_value()
                    && !evaluateConditionExpression(
                        *conditionalBranch.controlPath,
                        conditionalBranch.line,
                        "elseif condition",
                        scope)) {
                    continue;
                }

                expandedCount += visitExpandedNodes(
                    conditionalBranch.children,
                    scope,
                    visitor);
                break;
            }
            continue;
        }

        if (node.isForBlock()) {
            if (!node.controlPath.has_value()) {
                continue;
            }

            trackStructuralDependencies(*node.controlPath, node.line, "for source", scope);

            auto expression = compileBindingExpression(
                *node.controlPath,
                node.line,
                "for source",
                &warnings_);
            if (!expression) {
                continue;
            }

            if (!expression->isDirectPath()) {
                std::ostringstream oss;
                oss << "for source '${" << *node.controlPath << "}' at line " << node.line
                    << " must be a direct array path";
                warnings_.push_back(oss.str());
                continue;
            }

            const ModelNode* collectionNode = resolveScopedNode(expression->directPath(), scope);
            const ModelNode::Array* array = collectionNode != nullptr
                ? collectionNode->arrayValue()
                : nullptr;
            if (array == nullptr) {
                if (collectionNode != nullptr) {
                    std::ostringstream oss;
                    oss << "for source '" << *node.controlPath << "' at line "
                        << node.line << " is not an array";
                    warnings_.push_back(oss.str());
                }
                continue;
            }

            for (std::size_t index = 0; index < array->size(); ++index) {
                const ModelNode& itemNode = (*array)[index];
                ScopeBindings loopScope = scope;
                loopScope[node.loopVariable] = &itemNode;
                ModelNode loopIndexNode(core::Value(static_cast<int>(index)));
                if (node.loopIndexVariable.has_value()) {
                    loopScope[*node.loopIndexVariable] = &loopIndexNode;
                }
                expandedCount += visitExpandedNodes(node.children, loopScope, visitor);
            }
        }
    }

    return expandedCount;
}

AstNode LayoutBuilder::resolveComponentTemplateNode(
    const AstNode& templateNode,
    const std::unordered_map<std::string, AstProperty>& parameterValues,
    const std::unordered_map<std::string, std::vector<AstNode>>& slotChildren,
    std::unordered_set<std::string>& declaredSlots)
{
    AstNode resolved = templateNode;
    if (templateNode.controlPath.has_value()) {
        resolved.controlPath =
            resolveComponentControlExpression(*templateNode.controlPath, parameterValues);
    }
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

    resolved.conditionalBranches.clear();
    resolved.conditionalBranches.reserve(templateNode.conditionalBranches.size());
    for (const auto& conditionalBranch : templateNode.conditionalBranches) {
        AstNode resolvedBranch = conditionalBranch;
        if (conditionalBranch.controlPath.has_value()) {
            resolvedBranch.controlPath = resolveComponentControlExpression(
                *conditionalBranch.controlPath,
                parameterValues);
        }
        resolvedBranch.children.clear();
        for (const auto& branchChild : conditionalBranch.children) {
            appendResolvedComponentChild(
                branchChild,
                parameterValues,
                slotChildren,
                declaredSlots,
                resolvedBranch.children);
        }

        resolvedBranch.conditionalBranches.clear();
        for (const auto& nestedBranch : conditionalBranch.conditionalBranches) {
            resolvedBranch.conditionalBranches.push_back(resolveComponentTemplateNode(
                nestedBranch,
                parameterValues,
                slotChildren,
                declaredSlots));
        }

        resolved.conditionalBranches.push_back(std::move(resolvedBranch));
    }

    return resolved;
}

AstProperty LayoutBuilder::resolveComponentProperty(
    const AstProperty& property,
    const std::unordered_map<std::string, AstProperty>& parameterValues) const
{
    AstProperty resolved = property;
    if (property.hasArrayValue()) {
        resolved.arrayItems.clear();
        resolved.arrayItems.reserve(property.arrayItems.size());
        for (const AstProperty& entry : property.arrayItems) {
            resolved.arrayItems.push_back(resolveComponentProperty(entry, parameterValues));
        }
        return resolved;
    }

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
            parameterProp.implicitName = property.implicitName;
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

AstProperty LayoutBuilder::resolveScopedProperty(
    const AstProperty& property,
    const ScopeBindings& scope)
{
    std::function<AstProperty(const AstProperty&, std::unordered_set<std::string>&)> resolveRecursive;
    resolveRecursive =
        [this, &scope, &resolveRecursive](
            const AstProperty& source,
            std::unordered_set<std::string>& resolvingLets) -> AstProperty {
        AstProperty resolved = source;
        if (source.hasArrayValue()) {
            resolved.arrayItems.clear();
            resolved.arrayItems.reserve(source.arrayItems.size());
            for (const AstProperty& arrayItem : source.arrayItems) {
                resolved.arrayItems.push_back(resolveRecursive(arrayItem, resolvingLets));
            }
            return resolved;
        }

        if (source.hasObjectValue()) {
            resolved.objectProperties.clear();
            resolved.objectProperties.reserve(source.objectProperties.size());
            for (const auto& childProperty : source.objectProperties) {
                resolved.objectProperties.push_back(resolveRecursive(childProperty, resolvingLets));
            }
            return resolved;
        }

        if (!source.hasBinding() && source.value.type() == core::ValueType::Enum) {
            const std::string letName = source.value.asString();
            const auto letIt = letMap_.find(letName);
            if (letIt != letMap_.end()) {
                if (!resolvingLets.insert(letName).second) {
                    warnings_.push_back("cyclic let reference '" + letName + "'");
                    return resolved;
                }

                const AstProperty letValue =
                    resolveRecursive(letIt->second, resolvingLets);
                resolvingLets.erase(letName);

                resolved.value = letValue.value;
                resolved.arrayItems = letValue.arrayItems;
                resolved.objectProperties = letValue.objectProperties;
                resolved.bindingPath = letValue.bindingPath;
                resolved.arrayValue = letValue.arrayValue;
                resolved.objectValue = letValue.objectValue;
            }
        }

        if (!resolved.hasBinding()) {
            return resolved;
        }

        const std::optional<core::Value> constantValue =
            tryEvaluateBindingToConstant(*resolved.bindingPath, scope);
        if (constantValue.has_value()) {
            resolved.bindingPath.reset();
            resolved.value = *constantValue;
            resolved.arrayValue = false;
            resolved.arrayItems.clear();
            resolved.objectValue = false;
            resolved.objectProperties.clear();
        }

        return resolved;
    };

    std::unordered_set<std::string> resolvingLets;
    return resolveRecursive(property, resolvingLets);
}

const ModelNode* LayoutBuilder::resolveScopedNode(
    std::string_view path,
    const ScopeBindings& scope) const
{
    const std::string normalizedPath = normalizeScopedPath(path);
    if (normalizedPath.empty()) {
        return nullptr;
    }

    const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
    if (parts.empty()) {
        return nullptr;
    }

    if (const ModelNode* localNode = resolveLocalNode(normalizedPath, scope)) {
        return localNode;
    }

    return viewModel_ ? viewModel_->findNode(normalizedPath) : nullptr;
}

const core::Value* LayoutBuilder::resolveScopedValue(
    std::string_view path,
    const ScopeBindings& scope) const
{
    const ModelNode* node = resolveScopedNode(path, scope);
    return node != nullptr ? node->scalar() : nullptr;
}

std::optional<core::Value> LayoutBuilder::tryEvaluateBindingToConstant(
    std::string_view expressionText,
    const ScopeBindings& scope) const
{
    auto expression = compileBindingExpression(expressionText, 0, {}, nullptr);
    if (!expression) {
        return std::nullopt;
    }

    for (const std::string& dependency : expression->dependencies()) {
        const std::string normalizedPath = normalizeScopedPath(dependency);
        const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
        if (parts.empty() || !scope.contains(std::string(parts.front()))) {
            return std::nullopt;
        }
    }

    return expression->evaluateScalar([this, &scope](std::string_view path) {
        return resolveScopedNode(path, scope);
    });
}

std::optional<LayoutBuilder::PreparedBinding> LayoutBuilder::prepareBinding(
    std::string_view expressionText,
    const ScopeBindings& scope,
    int line,
    std::string_view context,
    bool allowWriteBack)
{
    auto expression =
        compileBindingExpression(expressionText, line, context, &warnings_);
    if (!expression) {
        return std::nullopt;
    }

    PreparedBinding binding;
    std::unordered_map<std::string, ModelNode> localRoots;

    for (const std::string& dependency : expression->dependencies()) {
        const std::string normalizedPath = normalizeScopedPath(dependency);
        const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
        if (parts.empty()) {
            continue;
        }

        const std::string rootName(parts.front());
        const auto localIt = scope.find(rootName);
        if (localIt != scope.end()) {
            if (localIt->second != nullptr && !localRoots.contains(rootName)) {
                localRoots.emplace(rootName, *localIt->second);
            }
            continue;
        }

        appendUniquePath(binding.dependencyPaths, normalizedPath);
    }

    const bool directPathBinding = expression->isDirectPath();
    std::string directPath;
    if (directPathBinding) {
        directPath = normalizeScopedPath(expression->directPath());
    }

    if (allowWriteBack && directPathBinding) {
        const std::string& normalizedPath = directPath;
        const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
        if (!normalizedPath.empty()
            && !parts.empty()
            && !scope.contains(std::string(parts.front()))) {
            binding.writeBackPath = normalizedPath;
        }
    }

    const auto scalarExpression = expression;
    const auto scalarLocalRoots = localRoots;
    binding.evaluate = [expression = scalarExpression,
                           localRoots = scalarLocalRoots](
                           const std::shared_ptr<ViewModel>& viewModel,
                           const std::function<const ModelNode*(std::string_view)>& externalResolver)
        -> std::optional<core::Value> {
        return expression->evaluateScalar(
            [viewModel, &localRoots, &externalResolver](std::string_view path) -> const ModelNode* {
                const std::string normalizedPath = normalizeScopedPath(path);
                if (const ModelNode* localNode =
                        resolveLocalSnapshotNode(normalizedPath, localRoots)) {
                    return localNode;
                }

                if (externalResolver) {
                    if (const ModelNode* externalNode = externalResolver(normalizedPath)) {
                        return externalNode;
                    }
                }

                return viewModel ? viewModel->findNode(normalizedPath) : nullptr;
            });
    };

    if (directPathBinding) {
        const auto nodeLocalRoots = localRoots;
        binding.evaluateNode = [directPath,
                                   localRoots = nodeLocalRoots](
                                   const std::shared_ptr<ViewModel>& viewModel,
                                   const std::function<const ModelNode*(std::string_view)>& externalResolver)
            -> const ModelNode* {
            if (const ModelNode* localNode =
                    resolveLocalSnapshotNode(directPath, localRoots)) {
                return localNode;
            }

            if (externalResolver) {
                if (const ModelNode* externalNode = externalResolver(directPath)) {
                    return externalNode;
                }
            }

            return viewModel ? viewModel->findNode(directPath) : nullptr;
        };
    }

    return binding;
}

std::optional<LayoutBuilder::PreparedPropertyNode> LayoutBuilder::preparePropertyNode(
    const AstProperty& property,
    const ScopeBindings& scope,
    std::string_view context)
{
    std::function<std::optional<PreparedPropertyNode>(const AstProperty&, std::string_view)> prepareRecursive;
    prepareRecursive =
        [this, &scope, &prepareRecursive](
            const AstProperty& source,
            std::string_view currentContext) -> std::optional<PreparedPropertyNode> {
        PreparedPropertyNode prepared;

        if (source.hasBinding()) {
            auto binding = prepareBinding(
                *source.bindingPath,
                scope,
                source.line,
                currentContext,
                false);
            if (!binding.has_value()) {
                return std::nullopt;
            }

            prepared.dependencyPaths = std::move(binding->dependencyPaths);
            prepared.evaluate = [binding = std::move(*binding)](
                                   const std::shared_ptr<ViewModel>& viewModel,
                                   const std::function<const ModelNode*(std::string_view)>& externalResolver)
                -> std::optional<ModelNode> {
                if (binding.evaluateNode) {
                    if (const ModelNode* node = binding.evaluateNode(viewModel, externalResolver)) {
                        return *node;
                    }
                }

                if (binding.evaluate) {
                    if (const std::optional<core::Value> value =
                            binding.evaluate(viewModel, externalResolver)) {
                        return ModelNode(*value);
                    }
                }

                return std::nullopt;
            };
            return prepared;
        }

        if (source.hasObjectValue()) {
            std::vector<std::pair<std::string, PreparedPropertyNode>> children;
            children.reserve(source.objectProperties.size());
            for (const AstProperty& childProperty : source.objectProperties) {
                std::optional<PreparedPropertyNode> childPrepared = prepareRecursive(
                    childProperty,
                    currentContext);
                if (!childPrepared.has_value()) {
                    return std::nullopt;
                }

                for (const std::string& dependencyPath : childPrepared->dependencyPaths) {
                    appendUniquePath(prepared.dependencyPaths, dependencyPath);
                }

                children.emplace_back(childProperty.name, std::move(*childPrepared));
            }

            prepared.evaluate = [children = std::move(children)](
                                   const std::shared_ptr<ViewModel>& viewModel,
                                   const std::function<const ModelNode*(std::string_view)>& externalResolver)
                -> std::optional<ModelNode> {
                ModelNode::Object object;
                for (const auto& [name, childPrepared] : children) {
                    if (!childPrepared.evaluate) {
                        return std::nullopt;
                    }

                    const std::optional<ModelNode> childNode =
                        childPrepared.evaluate(viewModel, externalResolver);
                    if (!childNode.has_value()) {
                        return std::nullopt;
                    }

                    object[name] = *childNode;
                }

                return ModelNode::object(std::move(object));
            };
            return prepared;
        }

        if (source.hasArrayValue()) {
            std::vector<PreparedPropertyNode> items;
            items.reserve(source.arrayItems.size());
            for (const AstProperty& arrayItem : source.arrayItems) {
                std::optional<PreparedPropertyNode> itemPrepared =
                    prepareRecursive(arrayItem, currentContext);
                if (!itemPrepared.has_value()) {
                    return std::nullopt;
                }

                for (const std::string& dependencyPath : itemPrepared->dependencyPaths) {
                    appendUniquePath(prepared.dependencyPaths, dependencyPath);
                }

                items.push_back(std::move(*itemPrepared));
            }

            prepared.evaluate = [items = std::move(items)](
                                   const std::shared_ptr<ViewModel>& viewModel,
                                   const std::function<const ModelNode*(std::string_view)>& externalResolver)
                -> std::optional<ModelNode> {
                ModelNode::Array array;
                array.reserve(items.size());
                for (const PreparedPropertyNode& itemPrepared : items) {
                    if (!itemPrepared.evaluate) {
                        return std::nullopt;
                    }

                    const std::optional<ModelNode> itemNode =
                        itemPrepared.evaluate(viewModel, externalResolver);
                    if (!itemNode.has_value()) {
                        return std::nullopt;
                    }

                    array.push_back(*itemNode);
                }

                return ModelNode::array(std::move(array));
            };
            return prepared;
        }

        const core::Value value = source.value;
        prepared.evaluate = [value](
                               const std::shared_ptr<ViewModel>&,
                               const std::function<const ModelNode*(std::string_view)>&)
            -> std::optional<ModelNode> {
            return ModelNode(value);
        };
        return prepared;
    };

    return prepareRecursive(property, context);
}

bool LayoutBuilder::evaluateConditionExpression(
    std::string_view expressionText,
    int line,
    std::string_view context,
    const ScopeBindings& scope)
{
    auto expression =
        compileBindingExpression(expressionText, line, context, &warnings_);
    if (!expression) {
        return false;
    }

    std::unordered_map<std::string, ModelNode> widgetNodeCache;
    return expression->evaluateTruthy(
        [this, &scope, &widgetNodeCache](std::string_view path) -> const ModelNode* {
        const std::string normalizedPath = normalizeScopedPath(path);
        if (const ModelNode* localNode = resolveLocalNode(normalizedPath, scope)) {
            return localNode;
        }

        if (const ModelNode* widgetNode =
                resolveWidgetSnapshotNode(idMap_, normalizedPath, widgetNodeCache)) {
            return widgetNode;
        }

        return viewModel_ ? viewModel_->findNode(normalizedPath) : nullptr;
    });
}

void LayoutBuilder::trackStructuralDependencies(
    std::string_view expressionText,
    int line,
    std::string_view context,
    const ScopeBindings& scope)
{
    auto expression = compileBindingExpression(expressionText, line, context, nullptr);
    if (!expression) {
        return;
    }

    for (const std::string& dependency : expression->dependencies()) {
        const std::string normalizedPath = normalizeScopedPath(dependency);
        const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
        if (parts.empty() || scope.contains(std::string(parts.front()))) {
            continue;
        }

        trackStructuralPath(normalizedPath);
    }
}

void LayoutBuilder::trackStructuralPath(std::string_view path)
{
    const std::string normalizedPath = normalizeScopedPath(path);
    if (normalizedPath.empty()) {
        return;
    }

    appendUniquePath(structuralPaths_, normalizedPath);
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

    for (const auto& conditionalBranch : node.conditionalBranches) {
        if (containsSlotNode(conditionalBranch)) {
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
    const AstProperty& prop,
    const ScopeBindings& scope)
{
    applyStyleProperties(
        widget,
        nodeType,
        prop.objectProperties,
        "inline style on '" + nodeType + "'",
        scope);
}

void LayoutBuilder::applyStyleProperties(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const std::vector<AstProperty>& properties,
    const std::string& context,
    const ScopeBindings& scope)
{
    auto& registry = core::TypeRegistry::instance();
    const core::TypeInfo* typeInfo = registry.findType(nodeType);
    if (typeInfo == nullptr) {
        std::ostringstream oss;
        oss << "unknown widget type '" << nodeType << "' in " << context;
        warnings_.push_back(oss.str());
        return;
    }

    for (const auto& styleProp : properties) {
        const AstProperty resolvedStyleProp = resolveScopedProperty(styleProp, scope);

        if (resolvedStyleProp.name == "style") {
            if (resolvedStyleProp.hasObjectValue()) {
                applyStyleProperties(
                    widget,
                    nodeType,
                    resolvedStyleProp.objectProperties,
                    context,
                    scope);
            } else {
                applyNamedStyle(widget, nodeType, resolvedStyleProp, scope);
            }
            continue;
        }

        if (resolvedStyleProp.hasObjectValue()) {
            std::ostringstream oss;
            oss << "nested object style property '" << resolvedStyleProp.name << "' in " << context
                << " at line " << resolvedStyleProp.line << " is not supported";
            warnings_.push_back(oss.str());
            continue;
        }

        const core::StylePropertyInfo* styleInfo =
            typeInfo->findStyleProperty(resolvedStyleProp.name);
        if (styleInfo == nullptr) {
            std::ostringstream oss;
            oss << "unsupported style property '" << resolvedStyleProp.name << "' in " << context
                << " for '" << nodeType << "' at line " << resolvedStyleProp.line;
            warnings_.push_back(oss.str());
            continue;
        }

        if (resolvedStyleProp.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *resolvedStyleProp.bindingPath,
                scope,
                resolvedStyleProp.line,
                "style property '" + resolvedStyleProp.name + "' in " + context,
                false);
            if (!preparedBinding.has_value()) {
                continue;
            }

            registerBinding(
                widget,
                resolvedStyleProp.name,
                std::move(preparedBinding->dependencyPaths),
                std::move(preparedBinding->writeBackPath),
                styleInfo->expectedType,
                std::move(preparedBinding->evaluate),
                [weakWidget = std::weak_ptr<ui::Widget>(widget),
                 theme = theme_,
                 setter = styleInfo->setter](const core::Value& value) {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    setter(*lockedWidget, value, theme);
                });
            continue;
        }

        styleInfo->setter(*widget, resolvedStyleProp.value, theme_);
    }
}

void LayoutBuilder::applyNamedStyle(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const AstProperty& prop,
    const ScopeBindings& scope)
{
    const auto applyResolvedStyleName = [this, &widget, &nodeType, &prop, &scope](
                                            const std::string& styleName) {
        const auto it = styleMap_.find(styleName);
        if (it == styleMap_.end()) {
            std::ostringstream oss;
            oss << "unknown style '" << styleName << "' on '" << nodeType
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
        applyStyleProperties(widget, nodeType, style.properties, "style '" + style.name + "'", scope);
        styleStack_.pop_back();
    };

    if (prop.hasObjectValue()) {
        std::ostringstream oss;
        oss << "style reference on '" << nodeType << "' must be a string or identifier at line "
            << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    if (prop.hasBinding()) {
        auto preparedBinding = prepareBinding(
            *prop.bindingPath,
            scope,
            prop.line,
            "style reference on '" + nodeType + "'",
            false);
        if (!preparedBinding.has_value()) {
            return;
        }

        for (const std::string& dependencyPath : preparedBinding->dependencyPaths) {
            trackStructuralPath(dependencyPath);
        }

        std::unordered_map<std::string, ModelNode> widgetNodeCache;
        const auto widgetResolver =
            [this, &widgetNodeCache](std::string_view path) -> const ModelNode* {
            const std::string normalizedPath = normalizeScopedPath(path);
            return resolveWidgetSnapshotNode(idMap_, normalizedPath, widgetNodeCache);
        };

        std::optional<core::Value> resolvedStyleValue;
        if (preparedBinding->evaluate) {
            resolvedStyleValue = preparedBinding->evaluate(viewModel_, widgetResolver);
        } else if (preparedBinding->evaluateNode) {
            if (const ModelNode* styleNode =
                    preparedBinding->evaluateNode(viewModel_, widgetResolver);
                styleNode != nullptr && styleNode->scalar() != nullptr) {
                resolvedStyleValue = *styleNode->scalar();
            }
        }

        if (resolvedStyleValue.has_value()) {
            if (const std::optional<std::string> styleName = stringLikeValue(*resolvedStyleValue)) {
                applyResolvedStyleName(*styleName);
            }
        }
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

    applyResolvedStyleName(*styleName);
}

void LayoutBuilder::applyStandardProperty(
    const std::shared_ptr<ui::Widget>& widget,
    const core::TypeInfo& typeInfo,
    const std::string& nodeType,
    const AstProperty& prop,
    const ScopeBindings& scope)
{
    if (prop.name == "items" && nodeType == "ListView") {
        if (prop.hasObjectValue()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' does not accept object values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        if (prop.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *prop.bindingPath,
                scope,
                prop.line,
                "property '" + prop.name + "' on '" + nodeType + "'",
                false);
            if (!preparedBinding.has_value()) {
                return;
            }

            if (!preparedBinding->evaluateNode) {
                std::ostringstream oss;
                oss << "property '" << prop.name << "' on '" << nodeType
                    << "' requires a direct array path at line " << prop.line;
                warnings_.push_back(oss.str());
                return;
            }

            registerNodeBinding(
                widget,
                prop.name,
                std::move(preparedBinding->dependencyPaths),
                {},
                std::move(preparedBinding->evaluateNode),
                [weakWidget = std::weak_ptr<ui::Widget>(widget)](const ModelNode& node) {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    const std::optional<std::vector<std::string>> items = stringArrayNode(node);
                    if (!items.has_value()) {
                        return;
                    }

                    installStringListViewSource(
                        static_cast<ui::ListView&>(*lockedWidget),
                        *items);
                });
            return;
        }

        if (!prop.hasArrayValue()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array literal at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        auto preparedPropertyNode = preparePropertyNode(
            prop,
            scope,
            "property '" + prop.name + "' on '" + nodeType + "'");
        if (!preparedPropertyNode.has_value() || !preparedPropertyNode->evaluate) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array of string-like values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        const auto applyItems =
            [weakWidget = std::weak_ptr<ui::Widget>(widget)](const ModelNode& node) {
            auto lockedWidget = weakWidget.lock();
            if (!lockedWidget) {
                return;
            }

            const std::optional<std::vector<std::string>> items = stringArrayNode(node);
            if (!items.has_value()) {
                return;
            }

            installStringListViewSource(
                static_cast<ui::ListView&>(*lockedWidget),
                *items);
        };

        if (!preparedPropertyNode->dependencyPaths.empty()) {
            registerComputedNodeBinding(
                widget,
                prop.name,
                std::move(preparedPropertyNode->dependencyPaths),
                std::move(preparedPropertyNode->evaluate),
                applyItems);
            return;
        }

        const std::optional<ModelNode> propertyNode =
            preparedPropertyNode->evaluate(viewModel_, {});
        if (!propertyNode.has_value() || !stringArrayNode(*propertyNode).has_value()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array of string-like values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        applyItems(*propertyNode);
        return;
    }

    if (prop.name == "items" && nodeType == "Dropdown") {
        if (prop.hasObjectValue()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' does not accept object values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        if (prop.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *prop.bindingPath,
                scope,
                prop.line,
                "property '" + prop.name + "' on '" + nodeType + "'",
                false);
            if (!preparedBinding.has_value()) {
                return;
            }

            if (!preparedBinding->evaluateNode) {
                std::ostringstream oss;
                oss << "property '" << prop.name << "' on '" << nodeType
                    << "' requires a direct array path at line " << prop.line;
                warnings_.push_back(oss.str());
                return;
            }

            registerNodeBinding(
                widget,
                prop.name,
                std::move(preparedBinding->dependencyPaths),
                {},
                std::move(preparedBinding->evaluateNode),
                [weakWidget = std::weak_ptr<ui::Widget>(widget)](const ModelNode& node) {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    const std::optional<std::vector<std::string>> items = stringArrayNode(node);
                    if (!items.has_value()) {
                        return;
                    }

                    static_cast<ui::Dropdown&>(*lockedWidget).setItems(*items);
                });
            return;
        }

        if (!prop.hasArrayValue()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array literal at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        auto preparedPropertyNode = preparePropertyNode(
            prop,
            scope,
            "property '" + prop.name + "' on '" + nodeType + "'");
        if (!preparedPropertyNode.has_value() || !preparedPropertyNode->evaluate) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array of string-like values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        const auto applyItems =
            [weakWidget = std::weak_ptr<ui::Widget>(widget)](const ModelNode& node) {
            auto lockedWidget = weakWidget.lock();
            if (!lockedWidget) {
                return;
            }

            const std::optional<std::vector<std::string>> items = stringArrayNode(node);
            if (!items.has_value()) {
                return;
            }

            static_cast<ui::Dropdown&>(*lockedWidget).setItems(*items);
        };

        if (!preparedPropertyNode->dependencyPaths.empty()) {
            registerComputedNodeBinding(
                widget,
                prop.name,
                std::move(preparedPropertyNode->dependencyPaths),
                std::move(preparedPropertyNode->evaluate),
                applyItems);
            return;
        }

        const std::optional<ModelNode> propertyNode =
            preparedPropertyNode->evaluate(viewModel_, {});
        if (!propertyNode.has_value() || !stringArrayNode(*propertyNode).has_value()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array of string-like values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        applyItems(*propertyNode);
        return;
    }

    if (prop.name == "spans" && nodeType == "RichText") {
        if (prop.hasObjectValue()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' does not accept object values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        if (prop.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *prop.bindingPath,
                scope,
                prop.line,
                "property '" + prop.name + "' on '" + nodeType + "'",
                false);
            if (!preparedBinding.has_value()) {
                return;
            }

            if (!preparedBinding->evaluateNode) {
                std::ostringstream oss;
                oss << "property '" << prop.name << "' on '" << nodeType
                    << "' requires a direct array path at line " << prop.line;
                warnings_.push_back(oss.str());
                return;
            }

            registerNodeBinding(
                widget,
                prop.name,
                std::move(preparedBinding->dependencyPaths),
                {},
                std::move(preparedBinding->evaluateNode),
                [weakWidget = std::weak_ptr<ui::Widget>(widget)](const ModelNode& node) {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    const std::optional<std::vector<ui::TextSpan>> spans = richTextSpansNode(node);
                    if (!spans.has_value()) {
                        return;
                    }

                    static_cast<ui::RichTextWidget&>(*lockedWidget).setSpans(*spans);
                });
            return;
        }

        if (!prop.hasArrayValue()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array literal or direct array path at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        auto preparedPropertyNode = preparePropertyNode(
            prop,
            scope,
            "property '" + prop.name + "' on '" + nodeType + "'");
        if (!preparedPropertyNode.has_value() || !preparedPropertyNode->evaluate) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array of object values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        const auto applySpans =
            [weakWidget = std::weak_ptr<ui::Widget>(widget)](const ModelNode& node) {
            auto lockedWidget = weakWidget.lock();
            if (!lockedWidget) {
                return;
            }

            const std::optional<std::vector<ui::TextSpan>> spans = richTextSpansNode(node);
            if (!spans.has_value()) {
                return;
            }

            static_cast<ui::RichTextWidget&>(*lockedWidget).setSpans(*spans);
        };

        if (!preparedPropertyNode->dependencyPaths.empty()) {
            registerComputedNodeBinding(
                widget,
                prop.name,
                std::move(preparedPropertyNode->dependencyPaths),
                std::move(preparedPropertyNode->evaluate),
                applySpans);
            return;
        }

        const std::optional<ModelNode> propertyNode =
            preparedPropertyNode->evaluate(viewModel_, {});
        if (!propertyNode.has_value()) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' expects an array of object values at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        applySpans(*propertyNode);
        return;
    }

    if (prop.hasArrayValue()) {
        std::ostringstream oss;
        oss << "property '" << prop.name << "' on '" << nodeType
            << "' does not accept array values at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

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

        const std::optional<std::string> idValue =
            prop.value.type() == core::ValueType::Enum
            ? std::optional<std::string>(prop.value.asString())
            : std::nullopt;
        if (idValue.has_value()) {
            widget->setId(*idValue);
            if (idMap_.contains(*idValue)) {
                std::ostringstream oss;
                oss << "duplicate id '" << *idValue << "' on '" << nodeType
                    << "' at line " << prop.line;
                warnings_.push_back(oss.str());
            }
            idMap_[*idValue] = widget;
        } else {
            std::ostringstream oss;
            oss << "property 'id' on '" << nodeType
                << "' expects an identifier at line "
                << prop.line;
            warnings_.push_back(oss.str());
        }
        return;
    }

    if (prop.name == "visible") {
        if (prop.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *prop.bindingPath,
                scope,
                prop.line,
                "property '" + prop.name + "' on '" + nodeType + "'",
                true);
            if (!preparedBinding.has_value()) {
                return;
            }

            registerBinding(
                widget,
                prop.name,
                std::move(preparedBinding->dependencyPaths),
                std::move(preparedBinding->writeBackPath),
                core::ValueType::Bool,
                std::move(preparedBinding->evaluate),
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

    if (prop.name == "enabled") {
        if (prop.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *prop.bindingPath,
                scope,
                prop.line,
                "property '" + prop.name + "' on '" + nodeType + "'",
                true);
            if (!preparedBinding.has_value()) {
                return;
            }

            registerBinding(
                widget,
                prop.name,
                std::move(preparedBinding->dependencyPaths),
                std::move(preparedBinding->writeBackPath),
                core::ValueType::Bool,
                std::move(preparedBinding->evaluate),
                [weakWidget = std::weak_ptr<ui::Widget>(widget)](const core::Value& value) {
                    auto lockedWidget = weakWidget.lock();
                    if (!lockedWidget) {
                        return;
                    }

                    lockedWidget->setEnabled(value.asBool());
                });
            return;
        }

        if (prop.value.type() == core::ValueType::Bool) {
            widget->setEnabled(prop.value.asBool());
        } else {
            std::ostringstream oss;
            oss << "property 'enabled' on '" << nodeType << "' expects a bool at line "
                << prop.line;
            warnings_.push_back(oss.str());
        }
        return;
    }

    if (const std::optional<std::string> interactionName =
            interactionNameFromPropertyName(prop.name)) {
        if (const core::InteractionInfo* interactionInfo = typeInfo.findInteraction(*interactionName)) {
            (void)interactionInfo;

            if (!prop.hasBinding()) {
                std::ostringstream oss;
                oss << "interaction property '" << prop.name << "' on '" << nodeType
                    << "' expects a direct binding path at line " << prop.line;
                warnings_.push_back(oss.str());
                return;
            }

            auto preparedBinding = prepareBinding(
                *prop.bindingPath,
                scope,
                prop.line,
                "interaction property '" + prop.name + "' on '" + nodeType + "'",
                false);
            if (!preparedBinding.has_value()) {
                return;
            }

            if (!preparedBinding->evaluateNode) {
                std::ostringstream oss;
                oss << "interaction property '" << prop.name << "' on '" << nodeType
                    << "' requires a direct binding path at line " << prop.line;
                warnings_.push_back(oss.str());
                return;
            }

            registerInteractionBinding(
                widget,
                *interactionName,
                std::move(preparedBinding->evaluateNode));
            return;
        }
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
        if (!propInfo->setter) {
            std::ostringstream oss;
            oss << "property '" << prop.name << "' on '" << nodeType
                << "' is read-only and cannot be bound at line " << prop.line;
            warnings_.push_back(oss.str());
            return;
        }

        auto preparedBinding = prepareBinding(
            *prop.bindingPath,
            scope,
            prop.line,
            "property '" + prop.name + "' on '" + nodeType + "'",
            true);
        if (!preparedBinding.has_value()) {
            return;
        }

        registerBinding(
            widget,
            prop.name,
            std::move(preparedBinding->dependencyPaths),
            std::move(preparedBinding->writeBackPath),
            propInfo->expectedType,
            std::move(preparedBinding->evaluate),
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

    if (!propInfo->setter) {
        std::ostringstream oss;
        oss << "property '" << prop.name << "' on '" << nodeType
            << "' is read-only and cannot be assigned at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

    propInfo->setter(*widget, prop.value);
}

void LayoutBuilder::registerBinding(
    const std::shared_ptr<ui::Widget>& widget,
    std::string propertyName,
    std::vector<std::string> dependencyPaths,
    std::string writeBackPath,
    core::ValueType expectedType,
    std::function<std::optional<core::Value>(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> evaluate,
    std::function<void(const core::Value&)> apply)
{
    detail::BindingDescriptor binding {
        .widget = widget,
        .propertyName = std::move(propertyName),
        .dependencyPaths = std::move(dependencyPaths),
        .writeBackPath = std::move(writeBackPath),
        .expectedType = expectedType,
        .evaluate = std::move(evaluate),
        .apply = std::move(apply),
    };

    if (viewModel_ && binding.evaluate && binding.apply) {
        std::unordered_map<std::string, ModelNode> widgetNodeCache;
        const auto widgetResolver =
            [this, &widgetNodeCache](std::string_view path) -> const ModelNode* {
            const std::string normalizedPath = normalizeScopedPath(path);
            return resolveWidgetSnapshotNode(idMap_, normalizedPath, widgetNodeCache);
        };
        const std::optional<core::Value> initialValue = binding.evaluate(viewModel_, widgetResolver);
        if (initialValue.has_value()) {
            const std::optional<core::Value> coercedValue =
                coerceInitialBindingValue(*initialValue, binding.expectedType);
            if (coercedValue.has_value()) {
                binding.apply(*coercedValue);
            }
        }
    }

    bindings_.push_back(std::move(binding));
}

void LayoutBuilder::registerNodeBinding(
    const std::shared_ptr<ui::Widget>& widget,
    std::string propertyName,
    std::vector<std::string> dependencyPaths,
    std::string writeBackPath,
    std::function<const ModelNode*(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> evaluateNode,
    std::function<void(const ModelNode&)> applyNode)
{
    detail::BindingDescriptor binding {
        .widget = widget,
        .propertyName = std::move(propertyName),
        .dependencyPaths = std::move(dependencyPaths),
        .writeBackPath = std::move(writeBackPath),
        .applyResolved =
            [evaluateNode = std::move(evaluateNode),
             applyNode = std::move(applyNode)](
                const std::shared_ptr<ViewModel>& viewModel,
                const std::function<const ModelNode*(std::string_view)>& externalResolver) -> bool {
                if (!evaluateNode || !applyNode) {
                    return false;
                }

                const ModelNode* node = evaluateNode(viewModel, externalResolver);
                if (node == nullptr) {
                    return false;
                }

                applyNode(*node);
                return true;
            },
    };

    if (viewModel_ && binding.applyResolved) {
        std::unordered_map<std::string, ModelNode> widgetNodeCache;
        const auto widgetResolver =
            [this, &widgetNodeCache](std::string_view path) -> const ModelNode* {
            const std::string normalizedPath = normalizeScopedPath(path);
            return resolveWidgetSnapshotNode(idMap_, normalizedPath, widgetNodeCache);
        };
        binding.applyResolved(viewModel_, widgetResolver);
    }

    bindings_.push_back(std::move(binding));
}

void LayoutBuilder::registerComputedNodeBinding(
    const std::shared_ptr<ui::Widget>& widget,
    std::string propertyName,
    std::vector<std::string> dependencyPaths,
    std::function<std::optional<ModelNode>(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> evaluateNode,
    std::function<void(const ModelNode&)> applyNode)
{
    detail::BindingDescriptor binding {
        .widget = widget,
        .propertyName = std::move(propertyName),
        .dependencyPaths = std::move(dependencyPaths),
        .applyResolved =
            [evaluateNode = std::move(evaluateNode),
             applyNode = std::move(applyNode)](
                const std::shared_ptr<ViewModel>& viewModel,
                const std::function<const ModelNode*(std::string_view)>& externalResolver) -> bool {
                if (!evaluateNode || !applyNode) {
                    return false;
                }

                const std::optional<ModelNode> node =
                    evaluateNode(viewModel, externalResolver);
                if (!node.has_value()) {
                    return false;
                }

                applyNode(*node);
                return true;
            },
    };

    if (viewModel_ && binding.applyResolved) {
        std::unordered_map<std::string, ModelNode> widgetNodeCache;
        const auto widgetResolver =
            [this, &widgetNodeCache](std::string_view path) -> const ModelNode* {
            const std::string normalizedPath = normalizeScopedPath(path);
            return resolveWidgetSnapshotNode(idMap_, normalizedPath, widgetNodeCache);
        };
        binding.applyResolved(viewModel_, widgetResolver);
    }

    bindings_.push_back(std::move(binding));
}

void LayoutBuilder::registerInteractionBinding(
    const std::shared_ptr<ui::Widget>& widget,
    std::string interactionName,
    std::function<const ModelNode*(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> evaluateNode)
{
    interactionBindings_.push_back(detail::InteractionBindingDescriptor {
        .widget = widget,
        .interactionName = std::move(interactionName),
        .evaluateNode = std::move(evaluateNode),
    });
}

bool LayoutBuilder::attachChildren(
    const std::shared_ptr<ui::Widget>& widget,
    const core::TypeInfo& typeInfo,
    const AstNode& node,
    const ScopeBindings& scope)
{
    const auto& childAttachment = typeInfo.childAttachment;
    if (!childAttachment.acceptsChildren()) {
        return false;
    }

    std::size_t expandedChildren = 0;
    const auto attachExpandedChild = [&](const AstNode& childNode, const ScopeBindings& childScope) {
        ++expandedChildren;
        if (childAttachment.policy == core::ChildAttachmentPolicy::Single && expandedChildren > 1) {
            return;
        }

        auto child = buildNode(childNode, childScope);
        if (child) {
            childAttachment.attach(*widget, std::move(child));
        }
    };

    visitExpandedNodes(node.children, scope, attachExpandedChild);
    if (expandedChildren == 0) {
        return true;
    }

    if (childAttachment.policy == core::ChildAttachmentPolicy::Single
        && expandedChildren > 1) {
        std::ostringstream oss;
        oss << "'" << typeInfo.name << "' accepts only one child content node at line "
            << node.line;
        warnings_.push_back(oss.str());
    }
    return true;
}

} // namespace tinalux::markup
