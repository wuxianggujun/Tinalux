#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tinalux::markup {

enum class TokenType : std::uint8_t {
    Identifier,
    StringLiteral,
    IntLiteral,
    FloatLiteral,
    ColorLiteral,
    BindingLiteral,

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Colon,
    Comma,

    EndOfFile,
    Error,
};

struct Token {
    TokenType type = TokenType::EndOfFile;
    std::string text;
    int line = 1;
    int column = 1;
};

} // namespace tinalux::markup
