#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "tinalux/markup/Ast.h"
#include "tinalux/markup/Lexer.h"

namespace tinalux::markup {

enum class ParseDiagnosticSeverity : std::uint8_t {
    Warning,
    Error,
};

struct ParseDiagnostic {
    ParseDiagnosticSeverity severity = ParseDiagnosticSeverity::Error;
    std::string message;
    int line = 0;
    int column = 0;

    [[nodiscard]] bool isError() const
    {
        return severity == ParseDiagnosticSeverity::Error;
    }

    [[nodiscard]] std::string format() const
    {
        std::string formatted =
            severity == ParseDiagnosticSeverity::Error ? "error" : "warning";
        if (line > 0) {
            formatted += " [" + std::to_string(line) + ":" + std::to_string(column) + "]";
        }
        if (!message.empty()) {
            formatted += ": " + message;
        }
        return formatted;
    }
};

struct DocumentParseResult {
    AstDocument document;
    std::vector<ParseDiagnostic> diagnostics;
    bool ok() const;
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
    AstNode parseSwitchNode(int line, int column);
    AstNode parseSwitchCaseBranch(
        const std::string& switchExpression,
        int line,
        int column);
    std::vector<AstNode> parseControlBlockChildren(const char* context);
    AstNode parseElseIfBranch(int line, int column);
    AstNode parseElseBranch(int line, int column);
    AstProperty parseProperty(bool allowImplicitName = false);
    std::vector<AstProperty> parseObjectValueLiteral();
    void parseArrayValue(AstProperty& prop);
    std::optional<std::string> parseSwitchCaseExpression();
    core::Value parseValue();
    core::Value parseValueDirective();
    void skipNodeBoundary();
    Token expect(TokenType type, const char* context);
    void error(
        const std::string& message,
        int line = -1,
        int column = -1);

    Lexer lexer_;
    Token current_;
    std::string baseDirectory_;
    std::vector<ParseDiagnostic> diagnostics_;
};

} // namespace tinalux::markup
