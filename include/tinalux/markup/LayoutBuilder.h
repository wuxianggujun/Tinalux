#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tinalux/markup/Ast.h"

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
    std::vector<std::string> warnings;
};

class LayoutBuilder {
public:
    static BuildResult build(const AstNode& ast, const ui::Theme& theme);
    static BuildResult build(const AstDocument& document, const ui::Theme& theme);

private:
    explicit LayoutBuilder(const ui::Theme& theme);

    void registerStyles(const std::vector<AstStyleDefinition>& styles);
    std::shared_ptr<ui::Widget> buildNode(const AstNode& node);
    void applyNamedStyle(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstProperty& prop);
    void applyStandardProperty(
        const std::shared_ptr<ui::Widget>& widget,
        const core::TypeInfo& typeInfo,
        const std::string& nodeType,
        const AstProperty& prop);
    bool applyStyleProperty(
        ui::Widget& widget,
        const std::string& nodeType,
        const AstProperty& prop);
    bool attachChildren(
        const std::shared_ptr<ui::Widget>& widget,
        const std::string& nodeType,
        const AstNode& node);

    const ui::Theme& theme_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap_;
    std::unordered_map<std::string, AstStyleDefinition> styleMap_;
    std::vector<std::string> warnings_;
};

void registerBuiltinTypes();

} // namespace tinalux::markup
