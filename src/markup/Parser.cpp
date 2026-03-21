#include "tinalux/markup/Parser.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
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

} // namespace

Parser::Parser(Lexer lexer)
    : lexer_(std::move(lexer))
{
    current_ = lexer_.next();
}

ParseResult Parser::parse(std::string_view source)
{
    auto documentResult = parseDocument(source);

    ParseResult result;
    if (documentResult.document.root) {
        result.root = std::move(*documentResult.document.root);
    } else {
        result.errors.push_back("[0:0] missing root node");
    }

    for (auto& error : documentResult.errors) {
        result.errors.push_back(std::move(error));
    }

    return result;
}

DocumentParseResult Parser::parseDocument(std::string_view source)
{
    Lexer lexer(source);
    Parser parser(std::move(lexer));

    DocumentParseResult result;
    result.document = parser.parseDocumentInternal();

    if (parser.current_.type != TokenType::EndOfFile) {
        parser.error("unexpected tokens after root node");
    }

    result.errors = std::move(parser.errors_);
    return result;
}

AstDocument Parser::parseDocumentInternal()
{
    AstDocument document;

    while (current_.type == TokenType::At) {
        parseDirective(document);
    }

    if (current_.type != TokenType::EndOfFile) {
        document.root = parseNode();
    }

    return document;
}

void Parser::parseDirective(AstDocument& document)
{
    expect(TokenType::At, "directive");

    if (current_.type != TokenType::Identifier) {
        error("expected directive name after '@'");
        return;
    }

    const std::string directive = current_.text;
    current_ = lexer_.next();

    if (directive == "import") {
        if (current_.type != TokenType::StringLiteral) {
            error("expected string literal after '@import'");
            return;
        }
        document.imports.push_back(current_.text);
        current_ = lexer_.next();
        return;
    }

    if (directive == "style") {
        document.styles.push_back(parseStyleDefinition());
        return;
    }

    error("unknown directive '@" + directive + "'");
}

AstStyleDefinition Parser::parseStyleDefinition()
{
    AstStyleDefinition style;
    style.line = current_.line;
    style.column = current_.column;

    if (current_.type != TokenType::Identifier) {
        error("expected style name after '@style'");
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

AstNode Parser::parseNode()
{
    AstNode node;
    node.line = current_.line;
    node.column = current_.column;

    // expect identifier (type name)
    if (current_.type != TokenType::Identifier) {
        error("expected widget type name, got '" + current_.text + "'");
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
            node.properties.push_back(parseProperty());

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

AstProperty Parser::parseProperty()
{
    AstProperty prop;
    prop.line = current_.line;
    prop.column = current_.column;

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

    prop.value = parseValue();

    return prop;
}

core::Value Parser::parseValue()
{
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

Token Parser::expect(TokenType type, const char* context)
{
    if (current_.type != type) {
        std::ostringstream oss;
        oss << "expected token in " << context << " at line " << current_.line
            << ":" << current_.column << ", got '" << current_.text << "'";
        error(oss.str());
        return current_;
    }
    Token tok = current_;
    current_ = lexer_.next();
    return tok;
}

void Parser::error(const std::string& message)
{
    std::ostringstream oss;
    oss << "[" << current_.line << ":" << current_.column << "] " << message;
    errors_.push_back(oss.str());
}

} // namespace tinalux::markup
