#pragma once

#include <string>
#include <string_view>

#include "tinalux/markup/Token.h"

namespace tinalux::markup {

class Lexer {
public:
    explicit Lexer(std::string_view source);

    Token next();
    Token peek();

private:
    void skipWhitespaceAndComments();
    Token readIdentifier();
    Token readNumber();
    Token readString();
    Token readColor();
    Token makeToken(TokenType type, std::string text) const;
    Token makeToken(TokenType type, char ch) const;
    char current() const;
    char advance();
    bool atEnd() const;

    std::string source_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    bool hasPeeked_ = false;
    Token peeked_;
};

} // namespace tinalux::markup
