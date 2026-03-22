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
let gap: 12
let accent: #FF336699
let ctaText: ${model.ctaText}
VBox(id: root) {
    Button(text: ctaText, style: { backgroundColor: accent, borderRadius: gap })
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(source);
        expectParseOk(result, "let document should parse");
        expect(result.document.lets.size() == 3, "parser should capture top-level let definitions");
        expect(
            result.document.lets[0].name == "gap"
                && result.document.lets[0].value.type() == core::ValueType::Int
                && result.document.lets[0].value.asInt() == 12,
            "int let should preserve scalar value");
        expect(
            result.document.lets[1].name == "accent"
                && result.document.lets[1].value.type() == core::ValueType::Color
                && result.document.lets[1].value.asColor().value() == 0xFF336699u,
            "color let should preserve color literal value");
        expect(
            result.document.lets[2].name == "ctaText"
                && result.document.lets[2].bindingPath.has_value()
                && *result.document.lets[2].bindingPath == "model.ctaText",
            "binding let should preserve raw binding expression");
    }

    {
        const std::string indexedLoopSource = R"(
/* top-level comment */
VBox(id: root) {
    /* nested comment */
    for(item, index in ${model.items}) {
        Label(text: ${index == 0 ? "First" : item.label})
    }
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(indexedLoopSource);
        expectParseOk(result, "indexed for-loop document should parse");
        expect(result.document.root.has_value(), "indexed for-loop document root should exist");

        const markup::AstNode& root = *result.document.root;
        expect(root.children.size() == 1, "indexed for-loop document should keep one control node");

        const markup::AstNode& forNode = root.children.front();
        expect(forNode.isForBlock(), "indexed loop child should parse as for block");
        expect(forNode.loopVariable == "item", "indexed for-loop should preserve item variable");
        expect(
            forNode.loopIndexVariable.has_value() && *forNode.loopIndexVariable == "index",
            "indexed for-loop should preserve index variable");
        expect(
            forNode.controlPath.has_value() && *forNode.controlPath == "model.items",
            "indexed for-loop should preserve collection binding");
        expect(forNode.children.size() == 1, "indexed for-loop should keep one template child");

        const markup::AstProperty* textProp = findProperty(forNode.children.front(), "text");
        expect(textProp != nullptr, "indexed for-loop child should preserve text property");
        expect(
            textProp->bindingPath.has_value()
                && *textProp->bindingPath == "index == 0 ? \"First\" : item.label",
            "indexed for-loop binding should preserve raw expression text");
    }

    {
        const std::string arraySource = R"(
let pickerItems: ["Zero", "One", "Two"]
VBox(id: root) {
    Dropdown(id: choice, items: ["North", "South", "East"])
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(arraySource);
        expectParseOk(result, "array literal document should parse");
        expect(result.document.lets.size() == 1, "array literal document should capture let definitions");
        expect(result.document.lets.front().hasArrayValue(), "let array literal should preserve array flag");
        expect(result.document.lets.front().arrayValues.size() == 3, "let array literal should preserve element count");
        expect(
            result.document.lets.front().arrayValues[1].type() == core::ValueType::String
                && result.document.lets.front().arrayValues[1].asString() == "One",
            "let array literal should preserve string elements");
        expect(result.document.root.has_value(), "array literal document root should exist");
        const markup::AstProperty* itemsProp = findProperty(*result.document.root, "items");
        expect(itemsProp == nullptr, "root VBox should not capture child dropdown items");
        expect(result.document.root->children.size() == 1, "array literal document should keep dropdown child");
        itemsProp = findProperty(result.document.root->children.front(), "items");
        expect(itemsProp != nullptr, "dropdown should preserve items property");
        expect(itemsProp->hasArrayValue(), "dropdown items should preserve array flag");
        expect(itemsProp->arrayValues.size() == 3, "dropdown items should preserve element count");
        expect(
            itemsProp->arrayValues[2].type() == core::ValueType::String
                && itemsProp->arrayValues[2].asString() == "East",
            "dropdown items should preserve array element payloads");
    }

    {
        const std::string source = R"(
import "components/shared.tui"
style primaryAction: Button(backgroundColor: #FF336699, borderRadius: 12)
component ActionChip(text: ${model.title}): Button(text: text)
VBox(id: root) {
    if(${model.showAdvanced}) {
        TextInput(id: advancedQuery, text: ${model.query})
    },
    for(item in ${model.items}) {
        Button(text: ${item.label})
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
        expect(ifNode.isIfBlock(), "first child should be if block");
        expect(
            ifNode.controlPath.has_value() && *ifNode.controlPath == "model.showAdvanced",
            "if control path should preserve model binding");
        expect(ifNode.children.size() == 1, "if should keep one child widget");
        const markup::AstProperty* ifTextProp = findProperty(ifNode.children.front(), "text");
        expect(ifTextProp != nullptr, "if child should preserve text property");
        expect(
            ifTextProp->bindingPath.has_value() && *ifTextProp->bindingPath == "model.query",
            "if child text binding should preserve path");

        const markup::AstNode& forNode = root.children[1];
        expect(forNode.isForBlock(), "second child should be for block");
        expect(forNode.loopVariable == "item", "for loop variable should be preserved");
        expect(
            forNode.controlPath.has_value() && *forNode.controlPath == "model.items",
            "for control path should preserve collection binding");
        expect(forNode.children.size() == 1, "for should keep one template child");
        const markup::AstNode& loopChild = forNode.children.front();
        expect(loopChild.isWidget(), "for child should remain a widget node");
        const markup::AstProperty* textProp = findProperty(loopChild, "text");
        expect(textProp != nullptr, "for child should preserve text property");
        expect(
            textProp->bindingPath.has_value() && *textProp->bindingPath == "item.label",
            "for child text binding should preserve loop-scoped path");
    }

    {
        const std::string expressionSource = R"(
VBox(id: root) {
    Button(
        id: cta,
        text: ${model.prefix + model.suffix},
        style: { borderRadius: ${model.radius * model.scale} }
    ),
    if(${model.count > 0 && model.enabled}) {
        Button(id: active, text: "Active")
    }
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(expressionSource);
        expectParseOk(result, "expression document should parse");
        expect(result.document.root.has_value(), "expression document root should exist");

        const markup::AstNode& root = *result.document.root;
        expect(root.children.size() == 2, "expression document should keep widget and if nodes");

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
        expect(ifNode.isIfBlock(), "expression document second child should stay if");
        expect(
            ifNode.controlPath.has_value()
                && *ifNode.controlPath == "model.count > 0 && model.enabled",
            "parser should preserve raw conditional expression text");
    }

    {
        const std::string interpolatedStringSource = R"(
VBox(id: root) {
    TextInput(id: summary, text: "共 ${model.count} 条记录"),
    TextInput(id: escaped, text: "字面量 \${model.count}")
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(interpolatedStringSource);
        expectParseOk(result, "interpolated string document should parse");
        expect(result.document.root.has_value(), "interpolated string document root should exist");

        const markup::AstNode& root = *result.document.root;
        expect(root.children.size() == 2, "interpolated string document should keep both widgets");

        const markup::AstProperty* summaryTextProp = findProperty(root.children.front(), "text");
        expect(summaryTextProp != nullptr, "summary TextInput should preserve text property");
        expect(
            summaryTextProp->bindingPath.has_value()
                && *summaryTextProp->bindingPath == "\"共 \" + (model.count) + \" 条记录\"",
            "parser should lower interpolated strings into binding expressions");

        const markup::AstProperty* escapedTextProp = findProperty(root.children.back(), "text");
        expect(escapedTextProp != nullptr, "escaped TextInput should preserve text property");
        expect(
            !escapedTextProp->bindingPath.has_value()
                && escapedTextProp->value.type() == core::ValueType::String
                && escapedTextProp->value.asString() == "字面量 ${model.count}",
            "parser should keep escaped interpolation markers as plain string text");
    }

    {
        const std::string shorthandSource = R"(
component SearchField(placeholder: "Search", currentText: ""): TextInput(placeholder, currentText)
VBox(id: root, 12, 8) {
    Label("Dashboard"),
    Button("Deploy", res("icons/deploy.png")),
    Checkbox("Remember me", true),
    SearchField("Filter", ${model.ctaText})
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(shorthandSource);
        expectParseOk(result, "anonymous property syntax should parse");
        expect(result.document.root.has_value(), "shorthand document root should exist");

        const markup::AstNode& root = *result.document.root;
        expect(root.properties.size() == 3, "VBox shorthand should preserve id and two positional args");
        expect(
            root.properties.front().name == "id"
                && root.properties.front().value.type() == core::ValueType::Enum
                && root.properties.front().value.asString() == "root",
            "id shorthand should preserve bare identifier value");
        expect(
            root.properties[1].hasImplicitName() && root.properties[2].hasImplicitName(),
            "VBox shorthand should preserve multiple positional arguments");
        expect(root.children.size() == 4, "shorthand document should keep four child widgets");

        const markup::AstNode& labelNode = root.children[0];
        expect(labelNode.properties.size() == 1, "Label shorthand should create one property");
        expect(
            labelNode.properties.front().hasImplicitName(),
            "Label shorthand should preserve implicit property marker");
        expect(
            labelNode.properties.front().name.empty(),
            "Label shorthand property should remain unnamed in AST");
        expect(
            labelNode.properties.front().value.type() == core::ValueType::String
                && labelNode.properties.front().value.asString() == "Dashboard",
            "Label shorthand should preserve string literal value");

        const markup::AstNode& buttonNode = root.children[1];
        expect(buttonNode.properties.size() == 2, "Button shorthand should keep two positional properties");
        expect(
            buttonNode.properties.front().value.type() == core::ValueType::String
                && buttonNode.properties.front().value.asString() == "Deploy",
            "Button shorthand should preserve first positional string");
        expect(
            buttonNode.properties.back().hasImplicitName(),
            "Button shorthand should preserve second positional property");

        const markup::AstNode& checkboxNode = root.children[2];
        expect(checkboxNode.properties.size() == 2, "Checkbox shorthand should preserve two positional properties");
        expect(
            checkboxNode.properties.front().hasImplicitName(),
            "Checkbox shorthand should preserve leading anonymous property");
        expect(
            checkboxNode.properties.back().hasImplicitName()
                && checkboxNode.properties.back().value.type() == core::ValueType::Bool
                && checkboxNode.properties.back().value.asBool(),
            "Checkbox shorthand should preserve trailing bool positional argument");

        const markup::AstNode& componentNode = root.children[3];
        expect(componentNode.properties.size() == 2, "component shorthand should preserve two positional arguments");
        expect(
            componentNode.properties.front().hasImplicitName(),
            "component shorthand should preserve first positional component argument");
        expect(
            componentNode.properties.back().hasBinding()
                && *componentNode.properties.back().bindingPath == "model.ctaText",
            "component shorthand should preserve bound positional component argument");
    }

    {
        const std::string invalidIfSource = R"(
VBox(id: root) {
    if(true) {
        Button(text: "broken")
    }
}
)";

        const markup::DocumentParseResult result =
            markup::Parser::parseDocument(invalidIfSource);
        expect(!result.ok(), "if should reject non-binding conditions");
    }

    {
        const std::string legacyDirectiveSource = R"(
@if(${model.primary}) {
    Button(text: "broken")
}
)";

        const markup::DocumentParseResult result =
            markup::Parser::parseDocument(legacyDirectiveSource);
        expect(!result.ok(), "legacy @ directive syntax should be rejected");
    }

    {
        const std::string stringIdSource = R"(
VBox(id: "legacyRoot") {
    Button(text: "broken")
}
)";

        const markup::DocumentParseResult result =
            markup::Parser::parseDocument(stringIdSource);
        expect(!result.ok(), "id should reject string literal syntax");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    if(${model.primary}) {
        Button(id: primary, text: "Primary")
    } elseif(${model.secondary}) {
        Button(id: secondary, text: "Secondary")
    } else {
        Button(id: fallback, text: "Fallback")
    }
}
)";

        const markup::DocumentParseResult result = markup::Parser::parseDocument(source);
        expect(result.ok(), "elseif/else markup should parse");
        expect(result.document.root.has_value(), "elseif/else parse should keep root");

        const markup::AstNode& root = *result.document.root;
        expect(root.children.size() == 1, "control-flow chain should remain a single root child");

        const markup::AstNode& ifNode = root.children.front();
        expect(ifNode.isIfBlock(), "if/elseif/else chain should parse as if block");
        expect(
            ifNode.controlPath.has_value() && *ifNode.controlPath == "model.primary",
            "primary branch should preserve control path");
        expect(ifNode.conditionalBranches.size() == 2, "if block should keep elseif and else branches");
        expect(
            ifNode.conditionalBranches.front().controlPath.has_value()
                && *ifNode.conditionalBranches.front().controlPath == "model.secondary",
            "elseif branch should preserve control path");
        expect(
            !ifNode.conditionalBranches.back().controlPath.has_value(),
            "else branch should not require a control path");
        expect(
            ifNode.conditionalBranches.front().children.size() == 1,
            "elseif branch should keep one child widget");
        expect(
            ifNode.conditionalBranches.back().children.size() == 1,
            "else branch should keep one child widget");
    }

    {
        const std::string invalidForSource = R"(
VBox(id: root) {
    for(item ${model.items}) {
        Button(text: "broken")
    }
}
)";

        const markup::DocumentParseResult result =
            markup::Parser::parseDocument(invalidForSource);
        expect(!result.ok(), "for should reject missing 'in' keyword");
    }

    return 0;
}
