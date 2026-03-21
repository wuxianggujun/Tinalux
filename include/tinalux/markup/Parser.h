#pragma once

#include <string>
#include <vector>

#include "tinalux/markup/Ast.h"
#include "tinalux/markup/Lexer.h"

namespace tinalux::markup {

struct ParseResult {
    AstNode root;
    std::vector<std::string> errors;
    bool ok() const { return errors.empty(); }
};

struct DocumentParseResult {
    AstDocument document;
    std::vector<std::string> errors;
    bool ok() const { return errors.empty(); }
};

class Parser {
public:
    static ParseResult parse(std::string_view source);
    static DocumentParseResult parseDocument(std::string_view source);

private:
    explicit Parser(Lexer lexer);

    AstDocument parseDocumentInternal();
    void parseDirective(AstDocument& document);
    AstStyleDefinition parseStyleDefinition();
    AstNode parseNode();
    AstProperty parseProperty();
    core::Value parseValue();
    Token expect(TokenType type, const char* context);
    void error(const std::string& message);

    Lexer lexer_;
    Token current_;
    std::vector<std::string> errors_;
};

} // namespace tinalux::markup
