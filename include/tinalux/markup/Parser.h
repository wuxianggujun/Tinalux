#pragma once

#include <string>
#include <vector>

#include "tinalux/markup/Ast.h"
#include "tinalux/markup/Lexer.h"

namespace tinalux::markup {

struct DocumentParseResult {
    AstDocument document;
    std::vector<std::string> errors;
    bool ok() const { return errors.empty(); }
};

class Parser {
public:
    static DocumentParseResult parseDocument(
        std::string_view source,
        std::string_view baseDirectory = {});

private:
    explicit Parser(Lexer lexer, std::string baseDirectory);

    AstDocument parseDocumentInternal();
    void parseDirective(AstDocument& document);
    AstStyleDefinition parseStyleDefinition();
    AstComponentDefinition parseComponentDefinition();
    AstNode parseNode();
    AstNode parseControlNode();
    AstNode parseIfNode(int line, int column);
    AstNode parseForNode(int line, int column);
    std::vector<AstNode> parseControlBlockChildren(const char* context);
    AstNode parseElseIfBranch(int line, int column);
    AstNode parseElseBranch(int line, int column);
    AstProperty parseProperty(bool allowImplicitName = false);
    core::Value parseValue();
    core::Value parseValueDirective();
    void skipNodeBoundary();
    Token expect(TokenType type, const char* context);
    void error(const std::string& message);

    Lexer lexer_;
    Token current_;
    std::string baseDirectory_;
    std::vector<std::string> errors_;
};

} // namespace tinalux::markup
