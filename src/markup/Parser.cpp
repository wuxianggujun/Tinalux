#include "tinalux/markup/Parser.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace tinalux::markup {

namespace {

core::Color parseColorHex(const std::string& hex)
{
    std::uint32_t value = 0;
    std::from_chars(hex.data(), hex.data() + hex.size(), value, 16);
    if (hex.size() == 6) {
        // RRGGBB → 0xFFRRGGBB
        value = 0xFF000000u | value;
    }
    // 8 digits: AARRGGBB → already correct
    return core::Color(value);
}

std::string resolveResourcePath(
    const std::string& baseDirectory,
    const std::string& resourcePath)
{
    if (baseDirectory.empty()) {
        return resourcePath;
    }

    const std::filesystem::path candidate =
        std::filesystem::path(baseDirectory) / std::filesystem::path(resourcePath);
    return candidate.lexically_normal().generic_string();
}

bool isIdentifierText(const Token& token, std::string_view text)
{
    return token.type == TokenType::Identifier && token.text == text;
}

std::string escapeBindingStringLiteral(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
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
            escaped += ch;
            break;
        }
    }
    return escaped;
}

bool isTopLevelDirectiveToken(const Token& token)
{
    return isIdentifierText(token, "import")
        || isIdentifierText(token, "let")
        || isIdentifierText(token, "style")
        || isIdentifierText(token, "component");
}

bool isControlDirectiveToken(const Token& token)
{
    return isIdentifierText(token, "if")
        || isIdentifierText(token, "for")
        || isIdentifierText(token, "switch")
        || isIdentifierText(token, "elseif")
        || isIdentifierText(token, "else");
}

bool isConditionalBranchToken(const Token& token)
{
    return isIdentifierText(token, "elseif")
        || isIdentifierText(token, "else");
}

} // namespace

Parser::Parser(Lexer lexer, std::string baseDirectory)
    : lexer_(std::move(lexer))
    , baseDirectory_(std::move(baseDirectory))
{
    current_ = lexer_.next();
}

bool DocumentParseResult::ok() const
{
    for (const ParseDiagnostic& diagnostic : diagnostics) {
        if (diagnostic.isError()) {
            return false;
        }
    }
    return true;
}

DocumentParseResult Parser::parseDocument(
    std::string_view source,
    std::string_view baseDirectory)
{
    Lexer lexer(source);
    Parser parser(std::move(lexer), std::string(baseDirectory));

    DocumentParseResult result;
    result.document = parser.parseDocumentInternal();

    if (parser.current_.type != TokenType::EndOfFile) {
        parser.error("unexpected tokens after root node");
    }

    result.diagnostics = std::move(parser.diagnostics_);
    return result;
}

AstDocument Parser::parseDocumentInternal()
{
    AstDocument document;

    while (current_.type != TokenType::EndOfFile) {
        if (isTopLevelDirectiveToken(current_)) {
            parseDirective(document);
            continue;
        }

        if (!document.root) {
            document.root = parseNode();
            continue;
        }

        break;
    }

    return document;
}

void Parser::parseDirective(AstDocument& document)
{
    if (!isTopLevelDirectiveToken(current_)) {
        error("expected top-level directive");
        return;
    }

    const std::string directive = current_.text;
    current_ = lexer_.next();

    if (directive == "import") {
        if (current_.type != TokenType::StringLiteral) {
            error("expected string literal after 'import'");
            return;
        }
        document.imports.push_back(current_.text);
        current_ = lexer_.next();
        return;
    }

    if (directive == "let") {
        document.lets.push_back(parseProperty());
        return;
    }

    if (directive == "style") {
        document.styles.push_back(parseStyleDefinition());
        return;
    }

    if (directive == "component") {
        document.components.push_back(parseComponentDefinition());
        return;
    }

    error("unknown directive '" + directive + "'");
}

AstStyleDefinition Parser::parseStyleDefinition()
{
    AstStyleDefinition style;
    style.line = current_.line;
    style.column = current_.column;

    if (current_.type != TokenType::Identifier) {
        error("expected style name after 'style'");
        return style;
    }

    style.name = current_.text;
    current_ = lexer_.next();

    expect(TokenType::Colon, "style definition");

    if (current_.type != TokenType::Identifier) {
        error("expected target widget type in style definition");
        return style;
    }

    style.targetType = current_.text;
    current_ = lexer_.next();

    if (current_.type == TokenType::LeftParen) {
        current_ = lexer_.next();

        while (current_.type != TokenType::RightParen
            && current_.type != TokenType::EndOfFile) {
            style.properties.push_back(parseProperty());

            if (current_.type == TokenType::Comma) {
                current_ = lexer_.next();
            }
        }

        expect(TokenType::RightParen, "style definition");
    } else {
        error("expected '(' after style target type");
    }

    return style;
}

AstComponentDefinition Parser::parseComponentDefinition()
{
    AstComponentDefinition component;
    component.line = current_.line;
    component.column = current_.column;

    if (current_.type != TokenType::Identifier) {
        error("expected component name after 'component'");
        return component;
    }

    component.name = current_.text;
    current_ = lexer_.next();

    if (current_.type == TokenType::LeftParen) {
        current_ = lexer_.next();

        while (current_.type != TokenType::RightParen
            && current_.type != TokenType::EndOfFile) {
            component.parameters.push_back(parseProperty());

            if (current_.type == TokenType::Comma) {
                current_ = lexer_.next();
            }
        }

        expect(TokenType::RightParen, "component parameter list");
    }

    expect(TokenType::Colon, "component definition");

    component.root = parseNode();
    return component;
}

AstNode Parser::parseNode()
{
    if (isControlDirectiveToken(current_)) {
        return parseControlNode();
    }

    AstNode node;
    node.kind = AstNodeKind::Widget;
    node.line = current_.line;
    node.column = current_.column;

    // expect identifier (type name)
    if (current_.type != TokenType::Identifier) {
        error("expected widget type name, got '" + current_.text + "'");
        skipNodeBoundary();
        return node;
    }

    node.typeName = current_.text;
    current_ = lexer_.next();

    // expect '('
    if (current_.type == TokenType::LeftParen) {
        current_ = lexer_.next();

        // parse property list
        while (current_.type != TokenType::RightParen
            && current_.type != TokenType::EndOfFile) {
            node.properties.push_back(parseProperty(true));

            if (current_.type == TokenType::Comma) {
                current_ = lexer_.next();
            }
        }

        expect(TokenType::RightParen, "property list");
    }

    // optional children block
    if (current_.type == TokenType::LeftBrace) {
        current_ = lexer_.next();

        while (current_.type != TokenType::RightBrace
            && current_.type != TokenType::EndOfFile) {
            node.children.push_back(parseNode());

            // optional comma between children
            if (current_.type == TokenType::Comma) {
                current_ = lexer_.next();
            }
        }

        expect(TokenType::RightBrace, "children block");
    }

    return node;
}

void Parser::skipNodeBoundary()
{
    while (current_.type != TokenType::Comma
        && current_.type != TokenType::RightBrace
        && current_.type != TokenType::EndOfFile) {
        current_ = lexer_.next();
    }
}

AstNode Parser::parseControlNode()
{
    if (!isControlDirectiveToken(current_)) {
        error("expected control directive");
        return {};
    }

    const Token directiveToken = current_;
    const std::string directive = directiveToken.text;
    current_ = lexer_.next();

    if (directive == "if") {
        return parseIfNode(directiveToken.line, directiveToken.column);
    }

    if (directive == "for") {
        return parseForNode(directiveToken.line, directiveToken.column);
    }

    if (directive == "switch") {
        return parseSwitchNode(directiveToken.line, directiveToken.column);
    }

    if (directive == "else" || directive == "elseif") {
        error("'" + directive + "' must appear immediately after an if block");
        return {};
    }

    error("unknown control directive '" + directive + "'");
    return {};
}

AstNode Parser::parseIfNode(int line, int column)
{
    AstNode node;
    node.kind = AstNodeKind::IfBlock;
    node.line = line;
    node.column = column;

    expect(TokenType::LeftParen, "if directive");
    if (current_.type != TokenType::BindingLiteral) {
        error("expected binding expression inside if(...)");
        return node;
    }

    node.controlPath = current_.text;
    current_ = lexer_.next();
    expect(TokenType::RightParen, "if directive");
    node.children = parseControlBlockChildren("if body");

    while (isConditionalBranchToken(current_)) {
        const Token directiveToken = current_;
        const std::string directive = directiveToken.text;
        current_ = lexer_.next();

        if (directive == "elseif") {
            if (!node.conditionalBranches.empty()
                && !node.conditionalBranches.back().controlPath.has_value()) {
                error("elseif cannot appear after else");
                return node;
            }

            node.conditionalBranches.push_back(
                parseElseIfBranch(directiveToken.line, directiveToken.column));
            continue;
        }

        if (!node.conditionalBranches.empty()
            && !node.conditionalBranches.back().controlPath.has_value()) {
            error("duplicate else branch for if block");
            return node;
        }

        node.conditionalBranches.push_back(
            parseElseBranch(directiveToken.line, directiveToken.column));
        break;
    }

    return node;
}

AstNode Parser::parseForNode(int line, int column)
{
    AstNode node;
    node.kind = AstNodeKind::ForBlock;
    node.line = line;
    node.column = column;

    expect(TokenType::LeftParen, "for directive");
    if (current_.type != TokenType::Identifier) {
        error("expected loop variable name inside for(...)");
        return node;
    }

    node.loopVariable = current_.text;
    current_ = lexer_.next();

    if (current_.type == TokenType::Comma) {
        current_ = lexer_.next();
        if (current_.type != TokenType::Identifier) {
            error("expected index variable name after ',' inside for(...)");
            return node;
        }

        node.loopIndexVariable = current_.text;
        current_ = lexer_.next();
    }

    if (!isIdentifierText(current_, "in")) {
        error("expected 'in' inside for(...)");
        return node;
    }
    current_ = lexer_.next();

    if (current_.type != TokenType::BindingLiteral) {
        error("expected binding expression after 'in' inside for(...)");
        return node;
    }

    node.controlPath = current_.text;
    current_ = lexer_.next();
    expect(TokenType::RightParen, "for directive");
    node.children = parseControlBlockChildren("for body");
    return node;
}

AstNode Parser::parseSwitchNode(int line, int column)
{
    AstNode node;
    node.kind = AstNodeKind::IfBlock;
    node.line = line;
    node.column = column;

    expect(TokenType::LeftParen, "switch directive");
    if (current_.type != TokenType::BindingLiteral) {
        error("expected binding expression inside switch(...)");
        return node;
    }

    const std::string switchExpression = current_.text;
    current_ = lexer_.next();
    expect(TokenType::RightParen, "switch directive");
    expect(TokenType::LeftBrace, "switch body");

    bool hasCaseBranch = false;
    bool hasFallbackBranch = false;

    while (current_.type != TokenType::RightBrace
        && current_.type != TokenType::EndOfFile) {
        if (isIdentifierText(current_, "case")) {
            if (hasFallbackBranch) {
                error("case cannot appear after else in switch block");
                return node;
            }

            const Token directiveToken = current_;
            current_ = lexer_.next();
            AstNode branch =
                parseSwitchCaseBranch(switchExpression, directiveToken.line, directiveToken.column);
            if (!hasCaseBranch) {
                hasCaseBranch = true;
                node.controlPath = std::move(branch.controlPath);
                node.children = std::move(branch.children);
            } else {
                node.conditionalBranches.push_back(std::move(branch));
            }
        } else if (isIdentifierText(current_, "else")) {
            if (hasFallbackBranch) {
                error("duplicate else branch for switch block");
                return node;
            }
            if (!hasCaseBranch) {
                error("switch block requires at least one case branch before else");
                return node;
            }

            const Token directiveToken = current_;
            current_ = lexer_.next();
            node.conditionalBranches.push_back(
                parseElseBranch(directiveToken.line, directiveToken.column));
            hasFallbackBranch = true;
        } else {
            error("expected case(...) or else inside switch block");
            return node;
        }

        if (current_.type == TokenType::Comma) {
            current_ = lexer_.next();
        }
    }

    expect(TokenType::RightBrace, "switch body");
    if (!hasCaseBranch) {
        error("switch block requires at least one case branch");
    }
    return node;
}

std::vector<AstNode> Parser::parseControlBlockChildren(const char* context)
{
    std::vector<AstNode> children;
    expect(TokenType::LeftBrace, context);

    while (current_.type != TokenType::RightBrace
        && current_.type != TokenType::EndOfFile) {
        children.push_back(parseNode());
        if (current_.type == TokenType::Comma) {
            current_ = lexer_.next();
        }
    }

    expect(TokenType::RightBrace, context);
    return children;
}

AstNode Parser::parseElseIfBranch(int line, int column)
{
    AstNode node;
    node.kind = AstNodeKind::IfBlock;
    node.line = line;
    node.column = column;

    expect(TokenType::LeftParen, "elseif directive");
    if (current_.type != TokenType::BindingLiteral) {
        error("expected binding expression inside elseif(...)");
        return node;
    }

    node.controlPath = current_.text;
    current_ = lexer_.next();
    expect(TokenType::RightParen, "elseif directive");
    node.children = parseControlBlockChildren("elseif body");
    return node;
}

AstNode Parser::parseElseBranch(int line, int column)
{
    AstNode node;
    node.kind = AstNodeKind::IfBlock;
    node.line = line;
    node.column = column;
    node.children = parseControlBlockChildren("else body");
    return node;
}

AstNode Parser::parseSwitchCaseBranch(
    const std::string& switchExpression,
    int line,
    int column)
{
    AstNode node;
    node.kind = AstNodeKind::IfBlock;
    node.line = line;
    node.column = column;

    expect(TokenType::LeftParen, "switch case");
    const std::optional<std::string> caseExpression = parseSwitchCaseExpression();
    if (!caseExpression.has_value()) {
        return node;
    }

    expect(TokenType::RightParen, "switch case");
    node.controlPath =
        "(" + switchExpression + ") == (" + *caseExpression + ")";
    node.children = parseControlBlockChildren("switch case body");
    return node;
}

AstProperty Parser::parseProperty(bool allowImplicitName)
{
    AstProperty prop;
    prop.line = current_.line;
    prop.column = current_.column;

    if (allowImplicitName) {
        const bool isNamedProperty = current_.type == TokenType::Identifier
            && lexer_.peek().type == TokenType::Colon;
        if (!isNamedProperty) {
            prop.implicitName = true;

            if (current_.type == TokenType::BindingLiteral) {
                prop.bindingPath = current_.text;
                current_ = lexer_.next();
                return prop;
            }

            if (current_.type == TokenType::LeftBrace) {
                prop.objectValue = true;
                prop.objectProperties = parseObjectValueLiteral();
                return prop;
            }

            if (current_.type == TokenType::LeftBracket) {
                parseArrayValue(prop);
                return prop;
            }

            prop.value = parseValue();
            return prop;
        }
    }

    if (current_.type != TokenType::Identifier) {
        error("expected property name, got '" + current_.text + "'");
        // skip to comma or right paren for recovery
        while (current_.type != TokenType::Comma
            && current_.type != TokenType::RightParen
            && current_.type != TokenType::EndOfFile) {
            current_ = lexer_.next();
        }
        return prop;
    }

    prop.name = current_.text;
    current_ = lexer_.next();

    expect(TokenType::Colon, "property");

    if (current_.type == TokenType::LeftBrace) {
        prop.objectValue = true;
        prop.objectProperties = parseObjectValueLiteral();
        return prop;
    }

    if (current_.type == TokenType::LeftBracket) {
        parseArrayValue(prop);
        return prop;
    }

    if (prop.name == "id"
        && (current_.type == TokenType::BindingLiteral || current_.type == TokenType::StringLiteral)) {
        error("property 'id' expects a bare identifier");
    }

    if (current_.type == TokenType::BindingLiteral) {
        prop.bindingPath = current_.text;
        current_ = lexer_.next();
        return prop;
    }

    prop.value = parseValue();

    return prop;
}

std::vector<AstProperty> Parser::parseObjectValueLiteral()
{
    std::vector<AstProperty> objectProperties;
    expect(TokenType::LeftBrace, "object property value");

    while (current_.type != TokenType::RightBrace
        && current_.type != TokenType::EndOfFile) {
        objectProperties.push_back(parseProperty());

        if (current_.type == TokenType::Comma) {
            current_ = lexer_.next();
        }
    }

    expect(TokenType::RightBrace, "object property value");
    return objectProperties;
}

void Parser::parseArrayValue(AstProperty& prop)
{
    prop.arrayValue = true;
    prop.arrayItems.clear();
    expect(TokenType::LeftBracket, "array property value");

    while (current_.type != TokenType::RightBracket
        && current_.type != TokenType::EndOfFile) {
        prop.arrayItems.push_back(parseProperty(true));

        if (current_.type == TokenType::Comma) {
            current_ = lexer_.next();
        }
    }

    expect(TokenType::RightBracket, "array property value");
}

std::optional<std::string> Parser::parseSwitchCaseExpression()
{
    Token tok = current_;
    switch (tok.type) {
    case TokenType::BindingLiteral:
        current_ = lexer_.next();
        return tok.text;

    case TokenType::StringLiteral:
        current_ = lexer_.next();
        return "\"" + escapeBindingStringLiteral(tok.text) + "\"";

    case TokenType::IntLiteral:
    case TokenType::FloatLiteral:
        current_ = lexer_.next();
        return tok.text;

    case TokenType::ColorLiteral:
        current_ = lexer_.next();
        return "#" + tok.text;

    case TokenType::Identifier:
        current_ = lexer_.next();
        if (tok.text == "true" || tok.text == "false") {
            return tok.text;
        }
        return "\"" + escapeBindingStringLiteral(tok.text) + "\"";

    default:
        error("expected switch case value");
        return std::nullopt;
    }
}

core::Value Parser::parseValue()
{
    if (isIdentifierText(current_, "res") && lexer_.peek().type == TokenType::LeftParen) {
        return parseValueDirective();
    }

    Token tok = current_;
    current_ = lexer_.next();

    switch (tok.type) {
    case TokenType::StringLiteral:
        return core::Value(tok.text);

    case TokenType::IntLiteral: {
        int val = 0;
        std::from_chars(tok.text.data(), tok.text.data() + tok.text.size(), val);
        return core::Value(val);
    }

    case TokenType::FloatLiteral: {
        float val = 0.0f;
        // std::from_chars for float may not be available on all compilers,
        // fall back to strtof
        val = std::strtof(tok.text.c_str(), nullptr);
        return core::Value(val);
    }

    case TokenType::ColorLiteral:
        return core::Value(parseColorHex(tok.text));

    case TokenType::Identifier:
        if (tok.text == "true") return core::Value(true);
        if (tok.text == "false") return core::Value(false);
        return core::Value::enumValue(tok.text);

    default:
        error("expected value, got '" + tok.text + "'");
        return core::Value();
    }
}

core::Value Parser::parseValueDirective()
{
    if (!isIdentifierText(current_, "res")) {
        error("expected value directive");
        return core::Value();
    }

    const std::string directive = current_.text;
    current_ = lexer_.next();

    if (directive != "res") {
        error("unknown value directive '" + directive + "'");
        return core::Value();
    }

    expect(TokenType::LeftParen, "resource directive");

    if (current_.type != TokenType::StringLiteral) {
        error("expected string literal inside res(...)");
        return core::Value();
    }

    const std::string resourcePath = current_.text;
    current_ = lexer_.next();

    expect(TokenType::RightParen, "resource directive");
    return core::Value(resolveResourcePath(baseDirectory_, resourcePath));
}

Token Parser::expect(TokenType type, const char* context)
{
    if (current_.type != type) {
        std::ostringstream oss;
        oss << "expected token in " << context << ", got '" << current_.text << "'";
        error(oss.str(), current_.line, current_.column);
        return current_;
    }
    Token tok = current_;
    current_ = lexer_.next();
    return tok;
}

void Parser::error(const std::string& message, int line, int column)
{
    ParseDiagnostic diagnostic;
    diagnostic.severity = ParseDiagnosticSeverity::Error;
    diagnostic.message = message;
    diagnostic.line = line >= 0 ? line : current_.line;
    diagnostic.column = column >= 0 ? column : current_.column;
    diagnostics_.push_back(std::move(diagnostic));
}

} // namespace tinalux::markup
