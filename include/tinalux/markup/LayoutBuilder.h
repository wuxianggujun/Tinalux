#pragma once

#include <memory>
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
    struct ExpandedNode {
        AstNode node;
        ScopeBindings scope;
    };

    LayoutBuilder(const ui::Theme& theme, std::shared_ptr<ViewModel> viewModel);

    void registerStyles(const std::vector<AstStyleDefinition>& styles);
    void registerComponents(const std::vector<AstComponentDefinition>& components);
    std::shared_ptr<ui::Widget> buildNode(const AstNode& node, const ScopeBindings& scope);
    std::shared_ptr<ui::Widget> buildComponentNode(
        const AstComponentDefinition& component,
        const AstNode& instanceNode,
        const ScopeBindings& scope);
    AstNode mergeComponentNode(
        const AstComponentDefinition& component,
        const AstNode& instanceNode);
    void appendExpandedNodes(
        const std::vector<AstNode>& sourceNodes,
        const ScopeBindings& scope,
        std::vector<ExpandedNode>& outNodes);
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
        const ScopeBindings& scope) const;
    bool containsSlotNode(const AstNode& node) const;
    const ModelNode* resolveScopedNode(
        std::string_view path,
        const ScopeBindings& scope) const;
    const core::Value* resolveScopedValue(
        std::string_view path,
        const ScopeBindings& scope) const;
    bool evaluateConditionPath(
        std::string_view path,
        const ScopeBindings& scope) const;
    void trackStructuralPath(std::string_view path);
    std::string resolveSlotName(
        const AstNode& slotNode,
        const std::unordered_map<std::string, AstProperty>& parameterValues);
    void applyInlineStyle(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstProperty& prop);
    void applyStyleProperties(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const std::vector<AstProperty>& properties,
        const std::string& context);
    void applyNamedStyle(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstProperty& prop);
    void applyStandardProperty(
        const std::shared_ptr<ui::Widget>& widget,
        const core::TypeInfo& typeInfo,
        const std::string& nodeType,
        const AstProperty& prop);
    void registerBinding(
        const std::shared_ptr<ui::Widget>& widget,
        std::string propertyName,
        std::string path,
        core::ValueType expectedType,
        std::function<void(const core::Value&)> apply);
    bool applyStyleProperty(
        ui::Widget& widget,
        const std::string& nodeType,
        const AstProperty& prop);
    bool attachChildren(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstNode& node,
        const ScopeBindings& scope);

    const ui::Theme& theme_;
    std::shared_ptr<ViewModel> viewModel_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap_;
    std::unordered_map<std::string, AstStyleDefinition> styleMap_;
    std::unordered_map<std::string, AstComponentDefinition> componentMap_;
    std::vector<std::string> componentStack_;
    std::vector<std::string> styleStack_;
    std::vector<detail::BindingDescriptor> bindings_;
    std::vector<std::string> structuralPaths_;
    std::vector<std::string> warnings_;
};

void registerBuiltinTypes();

} // namespace tinalux::markup
