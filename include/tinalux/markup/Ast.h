#pragma once

#include <optional>
#include <string>
#include <vector>

#include "tinalux/core/Reflect.h"

namespace tinalux::markup {

struct AstProperty {
    std::string name;
    core::Value value;
    int line = 0;
    int column = 0;
};

struct AstNode {
    std::string typeName;
    std::vector<AstProperty> properties;
    std::vector<AstNode> children;
    int line = 0;
    int column = 0;
};

struct AstStyleDefinition {
    std::string name;
    std::string targetType;
    std::vector<AstProperty> properties;
    int line = 0;
    int column = 0;
};

struct AstDocument {
    std::vector<std::string> imports;
    std::vector<AstStyleDefinition> styles;
    std::optional<AstNode> root;
};

} // namespace tinalux::markup
