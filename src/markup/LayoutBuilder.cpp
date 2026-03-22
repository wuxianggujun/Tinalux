#include "tinalux/markup/LayoutBuilder.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <sstream>

#include "BindingExpression.h"
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

void appendUniquePath(std::vector<std::string>& paths, std::string path)
{
    if (path.empty()) {
        return;
    }

    if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
        paths.push_back(std::move(path));
    }
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
    result.structuralPaths = std::move(builder.structuralPaths_);
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

    for (const auto& prop : node.properties) {
        const AstProperty resolvedProp = resolveScopedProperty(prop, scope);
        if (resolvedProp.name == "style") {
            if (resolvedProp.hasObjectValue()) {
                applyInlineStyle(widget, node.typeName, resolvedProp, scope);
            } else {
                applyNamedStyle(widget, node.typeName, resolvedProp, scope);
            }
        }
    }

    for (const auto& prop : node.properties) {
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

void LayoutBuilder::appendExpandedNodes(
    const std::vector<AstNode>& sourceNodes,
    const ScopeBindings& scope,
    std::vector<ExpandedNode>& outNodes)
{
    for (const auto& node : sourceNodes) {
        if (node.isWidget()) {
            outNodes.push_back(ExpandedNode {
                .node = node,
                .scope = scope,
            });
            continue;
        }

        if (node.isIfBlock()) {
            if (node.controlPath.has_value() && !node.controlPath->empty()) {
                trackStructuralDependencies(*node.controlPath, node.line, "@if condition", scope);
            }
            for (const auto& conditionalBranch : node.conditionalBranches) {
                if (conditionalBranch.controlPath.has_value() && !conditionalBranch.controlPath->empty()) {
                    trackStructuralDependencies(
                        *conditionalBranch.controlPath,
                        conditionalBranch.line,
                        "@elseif condition",
                        scope);
                }
            }

            if (node.controlPath.has_value()
                && evaluateConditionExpression(
                    *node.controlPath,
                    node.line,
                    "@if condition",
                    scope)) {
                appendExpandedNodes(node.children, scope, outNodes);
                continue;
            }

            for (const auto& conditionalBranch : node.conditionalBranches) {
                if (conditionalBranch.controlPath.has_value()
                    && !evaluateConditionExpression(
                        *conditionalBranch.controlPath,
                        conditionalBranch.line,
                        "@elseif condition",
                        scope)) {
                    continue;
                }

                appendExpandedNodes(conditionalBranch.children, scope, outNodes);
                break;
            }
            continue;
        }

        if (node.isForBlock()) {
            if (!node.controlPath.has_value()) {
                continue;
            }

            trackStructuralDependencies(*node.controlPath, node.line, "@for source", scope);

            auto expression = compileBindingExpression(
                *node.controlPath,
                node.line,
                "@for source",
                &warnings_);
            if (!expression) {
                continue;
            }

            if (!expression->isDirectPath()) {
                std::ostringstream oss;
                oss << "@for source '${" << *node.controlPath << "}' at line " << node.line
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
                    oss << "@for source '" << *node.controlPath << "' at line "
                        << node.line << " is not an array";
                    warnings_.push_back(oss.str());
                }
                continue;
            }

            for (const ModelNode& itemNode : *array) {
                ScopeBindings loopScope = scope;
                loopScope[node.loopVariable] = &itemNode;
                appendExpandedNodes(node.children, loopScope, outNodes);
            }
        }
    }
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

    resolved.conditionalBranches.clear();
    resolved.conditionalBranches.reserve(templateNode.conditionalBranches.size());
    for (const auto& conditionalBranch : templateNode.conditionalBranches) {
        AstNode resolvedBranch = conditionalBranch;
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

AstProperty LayoutBuilder::resolveScopedProperty(
    const AstProperty& property,
    const ScopeBindings& scope) const
{
    AstProperty resolved = property;
    if (property.hasObjectValue()) {
        resolved.objectProperties.clear();
        resolved.objectProperties.reserve(property.objectProperties.size());
        for (const auto& childProperty : property.objectProperties) {
            resolved.objectProperties.push_back(resolveScopedProperty(childProperty, scope));
        }
        return resolved;
    }

    if (!property.hasBinding()) {
        return resolved;
    }

    const std::optional<core::Value> constantValue =
        tryEvaluateBindingToConstant(*property.bindingPath, scope);
    if (constantValue.has_value()) {
        resolved.bindingPath.reset();
        resolved.value = *constantValue;
    }

    return resolved;
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

    if (allowWriteBack && expression->isDirectPath()) {
        const std::string normalizedPath = normalizeScopedPath(expression->directPath());
        const std::vector<std::string_view> parts = splitScopedPath(normalizedPath);
        if (!normalizedPath.empty()
            && !parts.empty()
            && !scope.contains(std::string(parts.front()))) {
            binding.writeBackPath = normalizedPath;
        }
    }

    binding.evaluate = [expression = std::move(expression),
                           localRoots = std::move(localRoots)](
                           const std::shared_ptr<ViewModel>& viewModel) -> std::optional<core::Value> {
        return expression->evaluateScalar(
            [viewModel, &localRoots](std::string_view path) -> const ModelNode* {
                const std::string normalizedPath = normalizeScopedPath(path);
                if (const ModelNode* localNode =
                        resolveLocalSnapshotNode(normalizedPath, localRoots)) {
                    return localNode;
                }

                return viewModel ? viewModel->findNode(normalizedPath) : nullptr;
            });
    };
    return binding;
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

    return expression->evaluateTruthy([this, &scope](std::string_view path) {
        return resolveScopedNode(path, scope);
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
        if (styleProp.name == "style") {
            if (styleProp.hasBinding()) {
                std::ostringstream oss;
                oss << "dynamic style references are not supported in " << context
                    << " at line " << styleProp.line;
                warnings_.push_back(oss.str());
                continue;
            }

            if (styleProp.hasObjectValue()) {
                applyStyleProperties(widget, nodeType, styleProp.objectProperties, context, scope);
            } else {
                applyNamedStyle(widget, nodeType, styleProp, scope);
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

        const core::StylePropertyInfo* styleInfo = typeInfo->findStyleProperty(styleProp.name);
        if (styleInfo == nullptr) {
            std::ostringstream oss;
            oss << "unsupported style property '" << styleProp.name << "' in " << context
                << " for '" << nodeType << "' at line " << styleProp.line;
            warnings_.push_back(oss.str());
            continue;
        }

        if (styleProp.hasBinding()) {
            auto preparedBinding = prepareBinding(
                *styleProp.bindingPath,
                scope,
                styleProp.line,
                "style property '" + styleProp.name + "' in " + context,
                false);
            if (!preparedBinding.has_value()) {
                continue;
            }

            registerBinding(
                widget,
                styleProp.name,
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

        styleInfo->setter(*widget, styleProp.value, theme_);
    }
}

void LayoutBuilder::applyNamedStyle(
    const std::shared_ptr<ui::Widget>& widget,
    const std::string& nodeType,
    const AstProperty& prop,
    const ScopeBindings& scope)
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
    applyStyleProperties(widget, nodeType, style.properties, "style '" + style.name + "'", scope);
    styleStack_.pop_back();
}

void LayoutBuilder::applyStandardProperty(
    const std::shared_ptr<ui::Widget>& widget,
    const core::TypeInfo& typeInfo,
    const std::string& nodeType,
    const AstProperty& prop,
    const ScopeBindings& scope)
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

    const core::PropertyInfo* propInfo = typeInfo.findProperty(prop.name);
    if (!propInfo) {
        std::ostringstream oss;
        oss << "unknown property '" << prop.name << "' on '" << nodeType
            << "' at line " << prop.line;
        warnings_.push_back(oss.str());
        return;
    }

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

    propInfo->setter(*widget, prop.value);
}

void LayoutBuilder::registerBinding(
    const std::shared_ptr<ui::Widget>& widget,
    std::string propertyName,
    std::vector<std::string> dependencyPaths,
    std::string writeBackPath,
    core::ValueType expectedType,
    std::function<std::optional<core::Value>(const std::shared_ptr<ViewModel>&)> evaluate,
    std::function<void(const core::Value&)> apply)
{
    bindings_.push_back(detail::BindingDescriptor {
        .widget = widget,
        .propertyName = std::move(propertyName),
        .dependencyPaths = std::move(dependencyPaths),
        .writeBackPath = std::move(writeBackPath),
        .expectedType = expectedType,
        .evaluate = std::move(evaluate),
        .apply = std::move(apply),
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

    std::vector<ExpandedNode> expandedChildren;
    appendExpandedNodes(node.children, scope, expandedChildren);
    if (expandedChildren.empty()) {
        return true;
    }

    if (childAttachment.policy == core::ChildAttachmentPolicy::Single
        && expandedChildren.size() > 1) {
        std::ostringstream oss;
        oss << "'" << typeInfo.name << "' accepts only one child content node at line "
            << node.line;
        warnings_.push_back(oss.str());
    }

    const auto attachExpandedChild = [&](const ExpandedNode& childNode) {
        auto child = buildNode(childNode.node, childNode.scope);
        if (child) {
            childAttachment.attach(*widget, std::move(child));
        }
    };

    if (childAttachment.policy == core::ChildAttachmentPolicy::Single) {
        attachExpandedChild(expandedChildren.front());
        return true;
    }

    for (const auto& childNode : expandedChildren) {
        attachExpandedChild(childNode);
    }
    return true;
}

} // namespace tinalux::markup
