#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "tinalux/core/Reflect.h"

namespace tinalux::markup {

enum class AstNodeKind : std::uint8_t {
    Widget,
    IfBlock,
    ForBlock,
};

struct AstProperty {
    std::string name;
    core::Value value;
    std::vector<AstProperty> objectProperties;
    std::optional<std::string> bindingPath;
    bool objectValue = false;
    bool implicitName = false;
    int line = 0;
    int column = 0;

    bool hasObjectValue() const { return objectValue; }
    bool hasBinding() const { return bindingPath.has_value(); }
    bool hasImplicitName() const { return implicitName; }
};

struct AstNode {
    AstNodeKind kind = AstNodeKind::Widget;
    std::string typeName;
    std::vector<AstProperty> properties;
    std::vector<AstNode> children;
    std::vector<AstNode> conditionalBranches;
    std::optional<std::string> controlPath;
    std::string loopVariable;
    std::optional<std::string> loopIndexVariable;
    int line = 0;
    int column = 0;

    [[nodiscard]] bool isWidget() const { return kind == AstNodeKind::Widget; }
    [[nodiscard]] bool isIfBlock() const { return kind == AstNodeKind::IfBlock; }
    [[nodiscard]] bool isForBlock() const { return kind == AstNodeKind::ForBlock; }
};

struct AstStyleDefinition {
    std::string name;
    std::string targetType;
    std::vector<AstProperty> properties;
    int line = 0;
    int column = 0;
};

struct AstComponentDefinition {
    std::string name;
    std::vector<AstProperty> parameters;
    AstNode root;
    int line = 0;
    int column = 0;
};

struct AstDocument {
    std::vector<std::string> imports;
    std::vector<AstProperty> lets;
    std::vector<AstStyleDefinition> styles;
    std::vector<AstComponentDefinition> components;
    std::optional<AstNode> root;
};

} // namespace tinalux::markup
