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

const tinalux::markup::AstProperty* findObjectProperty(
    const tinalux::markup::AstProperty& property,
    const char* name)
{
    for (const auto& child : property.objectProperties) {
        if (child.name == name) {
            return &child;
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
        const std::string expressionSource = R"(
VBox(id: "root") {
    Button(
        id: "cta",
        text: ${model.prefix + model.suffix},
        style: { borderRadius: ${model.radius * model.scale} }
    ),
    @if(${model.count > 0 && model.enabled}) {
        Button(id: "active", text: "Active")
    }
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(expressionSource);
        expectParseOk(result, "expression document should parse");
        expect(result.document.root.has_value(), "expression document root should exist");

        const markup::AstNode& root = *result.document.root;
        expect(root.children.size() == 2, "expression document should keep widget and @if nodes");

        const markup::AstNode& buttonNode = root.children.front();
        const markup::AstProperty* textProp = findProperty(buttonNode, "text");
        const markup::AstProperty* styleProp = findProperty(buttonNode, "style");
        expect(textProp != nullptr, "expression button should preserve text property");
        expect(styleProp != nullptr, "expression button should preserve style property");
        expect(
            textProp->bindingPath.has_value()
                && *textProp->bindingPath == "model.prefix + model.suffix",
            "parser should preserve raw string expression text");
        expect(styleProp->hasObjectValue(), "style property should preserve nested object");

        const markup::AstProperty* borderRadiusProp = findObjectProperty(*styleProp, "borderRadius");
        expect(borderRadiusProp != nullptr, "style object should preserve expression child property");
        expect(
            borderRadiusProp->bindingPath.has_value()
                && *borderRadiusProp->bindingPath == "model.radius * model.scale",
            "parser should preserve raw arithmetic expression text");

        const markup::AstNode& ifNode = root.children.back();
        expect(ifNode.isIfBlock(), "expression document second child should stay @if");
        expect(
            ifNode.controlPath.has_value()
                && *ifNode.controlPath == "model.count > 0 && model.enabled",
            "parser should preserve raw conditional expression text");
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
        const std::string source = R"(
VBox(id: "root") {
    @if(${model.primary}) {
        Button(id: "primary", text: "Primary")
    } @elseif(${model.secondary}) {
        Button(id: "secondary", text: "Secondary")
    } @else {
        Button(id: "fallback", text: "Fallback")
    }
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(source);
        expect(result.ok(), "@elseif/@else markup should parse");
        expect(result.document.root.has_value(), "@elseif/@else parse should keep root");

        const markup::AstNode& root = *result.document.root;
        expect(root.children.size() == 1, "control-flow chain should remain a single root child");

        const markup::AstNode& ifNode = root.children.front();
        expect(ifNode.isIfBlock(), "@if/@elseif/@else chain should parse as if block");
        expect(
            ifNode.controlPath.has_value() && *ifNode.controlPath == "model.primary",
            "primary branch should preserve control path");
        expect(ifNode.conditionalBranches.size() == 2, "if block should keep elseif and else branches");
        expect(
            ifNode.conditionalBranches.front().controlPath.has_value()
                && *ifNode.conditionalBranches.front().controlPath == "model.secondary",
            "@elseif branch should preserve control path");
        expect(
            !ifNode.conditionalBranches.back().controlPath.has_value(),
            "@else branch should not require a control path");
        expect(
            ifNode.conditionalBranches.front().children.size() == 1,
            "@elseif branch should keep one child widget");
        expect(
            ifNode.conditionalBranches.back().children.size() == 1,
            "@else branch should keep one child widget");
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
