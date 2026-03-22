#include "tinalux/markup/Lexer.h"

#include <cctype>

namespace tinalux::markup {

Lexer::Lexer(std::string_view source)
    : source_(source)
{
}

Token Lexer::next()
{
    if (hasPeeked_) {
        hasPeeked_ = false;
        return std::move(peeked_);
    }

    skipWhitespaceAndComments();

    if (atEnd()) {
        return makeToken(TokenType::EndOfFile, "");
    }

    char ch = current();

    if (ch == '(') { advance(); return makeToken(TokenType::LeftParen, "("); }
    if (ch == ')') { advance(); return makeToken(TokenType::RightParen, ")"); }
    if (ch == '{') { advance(); return makeToken(TokenType::LeftBrace, "{"); }
    if (ch == '}') { advance(); return makeToken(TokenType::RightBrace, "}"); }
    if (ch == ':') { advance(); return makeToken(TokenType::Colon, ":"); }
    if (ch == ',') { advance(); return makeToken(TokenType::Comma, ","); }

    if (ch == '#') {
        return readColor();
    }

    if (ch == '$' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '{') {
        return readBinding();
    }

    if (ch == '"') {
        return readString();
    }

    if (std::isdigit(static_cast<unsigned char>(ch))
        || (ch == '-' && pos_ + 1 < source_.size()
            && std::isdigit(static_cast<unsigned char>(source_[pos_ + 1])))) {
        return readNumber();
    }

    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
        return readIdentifier();
    }

    advance();
    return makeToken(TokenType::Error, std::string(1, ch));
}

Token Lexer::peek()
{
    if (!hasPeeked_) {
        peeked_ = next();
        hasPeeked_ = true;
    }
    return peeked_;
}

void Lexer::skipWhitespaceAndComments()
{
    while (!atEnd()) {
        char ch = current();

        if (ch == ' ' || ch == '\t' || ch == '\r') {
            advance();
            continue;
        }

        if (ch == '\n') {
            advance();
            continue;
        }

        // line comment
        if (ch == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            while (!atEnd() && current() != '\n') {
                advance();
            }
            continue;
        }

        // block comment
        if (ch == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            advance();
            advance();
            while (!atEnd()) {
                if (current() == '*'
                    && pos_ + 1 < source_.size()
                    && source_[pos_ + 1] == '/') {
                    advance();
                    advance();
                    break;
                }
                advance();
            }
            continue;
        }

        break;
    }
}

Token Lexer::readIdentifier()
{
    int startLine = line_;
    int startCol = column_;
    std::string text;
    while (!atEnd()
        && (std::isalnum(static_cast<unsigned char>(current())) || current() == '_')) {
        text += advance();
    }
    Token tok;
    tok.type = TokenType::Identifier;
    tok.text = std::move(text);
    tok.line = startLine;
    tok.column = startCol;
    return tok;
}

Token Lexer::readNumber()
{
    int startLine = line_;
    int startCol = column_;
    std::string text;
    bool isFloat = false;

    if (current() == '-') {
        text += advance();
    }

    while (!atEnd() && std::isdigit(static_cast<unsigned char>(current()))) {
        text += advance();
    }

    if (!atEnd() && current() == '.' && pos_ + 1 < source_.size()
        && std::isdigit(static_cast<unsigned char>(source_[pos_ + 1]))) {
        isFloat = true;
        text += advance(); // '.'
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(current()))) {
            text += advance();
        }
    }

    Token tok;
    tok.type = isFloat ? TokenType::FloatLiteral : TokenType::IntLiteral;
    tok.text = std::move(text);
    tok.line = startLine;
    tok.column = startCol;
    return tok;
}

Token Lexer::readString()
{
    int startLine = line_;
    int startCol = column_;
    advance(); // skip opening "

    std::string text;
    while (!atEnd() && current() != '"') {
        if (current() == '\\' && pos_ + 1 < source_.size()) {
            advance(); // skip backslash
            char escaped = advance();
            switch (escaped) {
            case 'n': text += '\n'; break;
            case 't': text += '\t'; break;
            case '\\': text += '\\'; break;
            case '"': text += '"'; break;
            default: text += '\\'; text += escaped; break;
            }
        } else {
            text += advance();
        }
    }

    if (!atEnd()) {
        advance(); // skip closing "
    }

    Token tok;
    tok.type = TokenType::StringLiteral;
    tok.text = std::move(text);
    tok.line = startLine;
    tok.column = startCol;
    return tok;
}

Token Lexer::readColor()
{
    int startLine = line_;
    int startCol = column_;
    advance(); // skip '#'

    std::string hex;
    while (!atEnd() && std::isxdigit(static_cast<unsigned char>(current()))) {
        hex += advance();
    }

    if (hex.size() != 6 && hex.size() != 8) {
        Token tok;
        tok.type = TokenType::Error;
        tok.text = "#" + hex;
        tok.line = startLine;
        tok.column = startCol;
        return tok;
    }

    Token tok;
    tok.type = TokenType::ColorLiteral;
    tok.text = hex;
    tok.line = startLine;
    tok.column = startCol;
    return tok;
}

Token Lexer::readBinding()
{
    int startLine = line_;
    int startCol = column_;
    advance(); // skip '$'
    advance(); // skip '{'

    std::string text;
    while (!atEnd() && current() != '}') {
        text += advance();
    }

    if (atEnd()) {
        Token tok;
        tok.type = TokenType::Error;
        tok.text = "${" + text;
        tok.line = startLine;
        tok.column = startCol;
        return tok;
    }

    advance(); // skip '}'

    Token tok;
    tok.type = TokenType::BindingLiteral;
    tok.text = std::move(text);
    tok.line = startLine;
    tok.column = startCol;
    return tok;
}

Token Lexer::makeToken(TokenType type, std::string text) const
{
    Token tok;
    tok.type = type;
    tok.text = std::move(text);
    tok.line = line_;
    tok.column = column_;
    return tok;
}

Token Lexer::makeToken(TokenType type, char ch) const
{
    return makeToken(type, std::string(1, ch));
}

char Lexer::current() const
{
    return source_[pos_];
}

char Lexer::advance()
{
    char ch = source_[pos_++];
    if (ch == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return ch;
}

bool Lexer::atEnd() const
{
    return pos_ >= source_.size();
}

} // namespace tinalux::markup
