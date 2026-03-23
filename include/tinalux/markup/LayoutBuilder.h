#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tinalux/markup/Ast.h"
#include "tinalux/markup/ViewModel.h"

namespace tinalux::ui {
class Widget;
struct Theme;
} // namespace tinalux::ui

namespace tinalux::core {
struct TypeInfo;
}

namespace tinalux::markup {

struct BuildResult {
    std::shared_ptr<ui::Widget> root;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap;
    std::vector<detail::BindingDescriptor> bindings;
    std::vector<detail::InteractionBindingDescriptor> interactionBindings;
    std::vector<std::string> structuralPaths;
    std::vector<std::string> warnings;
};

class LayoutBuilder {
public:
    static BuildResult build(
        const AstNode& ast,
        const ui::Theme& theme,
        const std::shared_ptr<ViewModel>& viewModel = {});
    static BuildResult build(
        const AstDocument& document,
        const ui::Theme& theme,
        const std::shared_ptr<ViewModel>& viewModel = {});

private:
    using ScopeBindings = std::unordered_map<std::string, const ModelNode*>;
    struct PreparedBinding {
        std::vector<std::string> dependencyPaths;
        std::string writeBackPath;
        std::string externalDirectPath;
        std::function<std::optional<core::Value>(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluate;
        std::function<const ModelNode*(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluateNode;
    };
    struct PreparedPropertyNode {
        std::vector<std::string> dependencyPaths;
        std::function<std::optional<ModelNode>(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluate;
    };

    LayoutBuilder(const ui::Theme& theme, std::shared_ptr<ViewModel> viewModel);

    void registerLets(const std::vector<AstProperty>& lets);
    void registerStyles(const std::vector<AstStyleDefinition>& styles);
    void registerComponents(const std::vector<AstComponentDefinition>& components);
    std::shared_ptr<ui::Widget> buildNode(const AstNode& node, const ScopeBindings& scope);
    std::shared_ptr<ui::Widget> buildComponentNode(
        const AstComponentDefinition& component,
        const AstNode& instanceNode,
        const ScopeBindings& scope);
    std::vector<AstProperty> normalizeImplicitProperties(
        const std::vector<AstProperty>& properties,
        std::string_view ownerName,
        const std::vector<std::string>& positionalPropertyNames);
    AstNode mergeComponentNode(
        const AstComponentDefinition& component,
        const AstNode& instanceNode);
    std::size_t visitExpandedNodes(
        const std::vector<AstNode>& sourceNodes,
        const ScopeBindings& scope,
        const std::function<void(const AstNode&, const ScopeBindings&)>& visitor);
    AstNode resolveComponentTemplateNode(
        const AstNode& templateNode,
        const std::unordered_map<std::string, AstProperty>& parameterValues,
        const std::unordered_map<std::string, std::vector<AstNode>>& slotChildren,
        std::unordered_set<std::string>& declaredSlots);
    AstProperty resolveComponentProperty(
        const AstProperty& property,
        const std::unordered_map<std::string, AstProperty>& parameterValues) const;
    void appendResolvedComponentChild(
        const AstNode& templateNode,
        const std::unordered_map<std::string, AstProperty>& parameterValues,
        const std::unordered_map<std::string, std::vector<AstNode>>& slotChildren,
        std::unordered_set<std::string>& declaredSlots,
        std::vector<AstNode>& outChildren);
    core::Value resolveComponentValue(
        const core::Value& value,
        const std::unordered_map<std::string, AstProperty>& parameterValues) const;
    void applyNodePropertyOverrides(
        AstNode& node,
        const std::vector<AstProperty>& overrideProperties) const;
    AstProperty resolveScopedProperty(
        const AstProperty& property,
        const ScopeBindings& scope);
    std::optional<core::Value> tryEvaluateBindingToConstant(
        std::string_view expressionText,
        const ScopeBindings& scope) const;
    std::optional<PreparedBinding> prepareBinding(
        std::string_view expressionText,
        const ScopeBindings& scope,
        int line,
        std::string_view context,
        bool allowWriteBack);
    std::optional<PreparedPropertyNode> preparePropertyNode(
        const AstProperty& property,
        const ScopeBindings& scope,
        std::string_view context);
    bool containsSlotNode(const AstNode& node) const;
    const ModelNode* resolveScopedNode(
        std::string_view path,
        const ScopeBindings& scope) const;
    const core::Value* resolveScopedValue(
        std::string_view path,
        const ScopeBindings& scope) const;
    bool evaluateConditionExpression(
        std::string_view expressionText,
        int line,
        std::string_view context,
        const ScopeBindings& scope);
    void trackStructuralDependencies(
        std::string_view expressionText,
        int line,
        std::string_view context,
        const ScopeBindings& scope);
    void trackStructuralPath(std::string_view path);
    std::string resolveSlotName(
        const AstNode& slotNode,
        const std::unordered_map<std::string, AstProperty>& parameterValues);
    void applyInlineStyle(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstProperty& prop,
        const ScopeBindings& scope);
    void applyStyleProperties(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const std::vector<AstProperty>& properties,
        const std::string& context,
        const ScopeBindings& scope);
    void applyNamedStyle(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstProperty& prop,
        const ScopeBindings& scope);
    void applyStandardProperty(
        const std::shared_ptr<ui::Widget>& widget,
        const core::TypeInfo& typeInfo,
        const std::string& nodeType,
        const AstProperty& prop,
        const ScopeBindings& scope);
    void registerBinding(
        const std::shared_ptr<ui::Widget>& widget,
        std::string propertyName,
        std::vector<std::string> dependencyPaths,
        std::string writeBackPath,
        core::ValueType expectedType,
        std::function<std::optional<core::Value>(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluate,
        std::function<void(const core::Value&)> apply);
    void registerNodeBinding(
        const std::shared_ptr<ui::Widget>& widget,
        std::string propertyName,
        std::vector<std::string> dependencyPaths,
        std::string writeBackPath,
        std::function<const ModelNode*(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluateNode,
        std::function<void(const ModelNode&)> applyNode);
    void registerComputedNodeBinding(
        const std::shared_ptr<ui::Widget>& widget,
        std::string propertyName,
        std::vector<std::string> dependencyPaths,
        std::function<std::optional<ModelNode>(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluateNode,
        std::function<void(const ModelNode&)> applyNode);
    void registerInteractionBinding(
        const std::shared_ptr<ui::Widget>& widget,
        std::string interactionName,
        std::string actionPath,
        core::ValueType payloadType,
        std::function<const ModelNode*(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)> evaluateNode);
    bool attachChildren(
        const std::shared_ptr<ui::Widget>& widget,
        const core::TypeInfo& typeInfo,
        const AstNode& node,
        const ScopeBindings& scope);

    const ui::Theme& theme_;
    std::shared_ptr<ViewModel> viewModel_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap_;
    std::unordered_map<std::string, AstProperty> letMap_;
    std::unordered_map<std::string, AstStyleDefinition> styleMap_;
    std::unordered_map<std::string, AstComponentDefinition> componentMap_;
    std::vector<std::string> componentStack_;
    std::vector<std::string> styleStack_;
    std::vector<detail::BindingDescriptor> bindings_;
    std::vector<detail::InteractionBindingDescriptor> interactionBindings_;
    std::vector<std::string> structuralPaths_;
    std::vector<std::string> warnings_;
};

void registerBuiltinTypes();

} // namespace tinalux::markup
