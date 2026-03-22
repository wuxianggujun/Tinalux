#include <cstdlib>
#include <iostream>
#include <string>

#include "tinalux/markup/Parser.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void expectParseOk(
    const tinalux::markup::DocumentParseResult& result,
    const char* context)
{
    if (result.ok()) {
        return;
    }

    std::cerr << context << '\n';
    for (const auto& error : result.errors) {
        std::cerr << "error: " << error << '\n';
    }
    std::exit(1);
}

const tinalux::markup::AstProperty* findProperty(
    const tinalux::markup::AstNode& node,
    const char* name)
{
    for (const auto& property : node.properties) {
        if (property.name == name) {
            return &property;
        }
    }

    return nullptr;
}

} // namespace

int main()
{
    using namespace tinalux;

    {
        const std::string source = R"(
@import "components/shared.tui"
@style primaryAction: Button(backgroundColor: #FF336699, borderRadius: 12)
@component ActionChip(text: ${model.title}): Button(text: text)
VBox(id: "root") {
    @if(${model.showAdvanced}) {
        TextInput(id: "advancedQuery", text: ${model.query})
    },
    @for(item in ${model.items}) {
        Button(id: ${item.id}, text: ${item.label})
    }
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(source);
        expectParseOk(result, "control-flow document should parse");
        expect(result.document.imports.size() == 1, "parser should capture one import");
        expect(
            result.document.imports.front() == "components/shared.tui",
            "parser should preserve import path");
        expect(result.document.styles.size() == 1, "parser should capture one style");
        expect(
            result.document.styles.front().targetType == "Button",
            "style target type should be preserved");
        expect(
            result.document.styles.front().properties.size() == 2,
            "style properties should be parsed");
        expect(result.document.components.size() == 1, "parser should capture one component");
        expect(
            result.document.components.front().parameters.size() == 1,
            "component parameters should be parsed");
        expect(
            result.document.components.front().parameters.front().bindingPath.has_value()
                && *result.document.components.front().parameters.front().bindingPath == "model.title",
            "component parameter default binding should preserve binding path");

        expect(result.document.root.has_value(), "document root should exist");
        const markup::AstNode& root = *result.document.root;
        expect(root.isWidget(), "root should parse as a widget node");
        expect(root.typeName == "VBox", "root type should be VBox");
        expect(root.children.size() == 2, "root should contain if/for control nodes");

        const markup::AstNode& ifNode = root.children[0];
        expect(ifNode.isIfBlock(), "first child should be @if block");
        expect(
            ifNode.controlPath.has_value() && *ifNode.controlPath == "model.showAdvanced",
            "@if control path should preserve model binding");
        expect(ifNode.children.size() == 1, "@if should keep one child widget");
        const markup::AstProperty* ifTextProp = findProperty(ifNode.children.front(), "text");
        expect(ifTextProp != nullptr, "@if child should preserve text property");
        expect(
            ifTextProp->bindingPath.has_value() && *ifTextProp->bindingPath == "model.query",
            "@if child text binding should preserve path");

        const markup::AstNode& forNode = root.children[1];
        expect(forNode.isForBlock(), "second child should be @for block");
        expect(forNode.loopVariable == "item", "@for loop variable should be preserved");
        expect(
            forNode.controlPath.has_value() && *forNode.controlPath == "model.items",
            "@for control path should preserve collection binding");
        expect(forNode.children.size() == 1, "@for should keep one template child");
        const markup::AstNode& loopChild = forNode.children.front();
        expect(loopChild.isWidget(), "@for child should remain a widget node");
        const markup::AstProperty* idProp = findProperty(loopChild, "id");
        const markup::AstProperty* textProp = findProperty(loopChild, "text");
        expect(idProp != nullptr, "@for child should preserve id property");
        expect(textProp != nullptr, "@for child should preserve text property");
        expect(
            idProp->bindingPath.has_value() && *idProp->bindingPath == "item.id",
            "@for child id binding should preserve loop-scoped path");
        expect(
            textProp->bindingPath.has_value() && *textProp->bindingPath == "item.label",
            "@for child text binding should preserve loop-scoped path");
    }

    {
        const std::string invalidIfSource = R"(
VBox(id: "root") {
    @if(true) {
        Button(text: "broken")
    }
}
)";

        const markup::DocumentParseResult result =
            markup::Parser::parseDocument(invalidIfSource);
        expect(!result.ok(), "@if should reject non-binding conditions");
    }

    {
        const std::string invalidForSource = R"(
VBox(id: "root") {
    @for(item ${model.items}) {
        Button(text: "broken")
    }
}
)";

        const markup::DocumentParseResult result =
            markup::Parser::parseDocument(invalidForSource);
        expect(!result.ok(), "@for should reject missing 'in' keyword");
    }

    return 0;
}
