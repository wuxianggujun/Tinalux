#include "BindingExpression.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace tinalux::markup::detail {

namespace {

enum class TokenType : std::uint8_t {
    End,
    Error,
    Identifier,
    Number,
    String,
    Color,
    LeftParen,
    RightParen,
    Question,
    Colon,
    Plus,
    Minus,
    Star,
    Slash,
    Bang,
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    AndAnd,
    OrOr,
};

struct Token {
    TokenType type = TokenType::End;
    std::string text;
    int position = 0;
};

std::optional<core::Color> parseColorHex(std::string_view hex)
{
    std::uint32_t value = 0;
    const auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), value, 16);
    if (ec != std::errc {} || ptr != hex.data() + hex.size()) {
        return std::nullopt;
    }

    if (hex.size() == 6) {
        value = 0xFF000000u | value;
    } else if (hex.size() != 8) {
        return std::nullopt;
    }

    return core::Color(value);
}

bool truthyScalar(const core::Value& value)
{
    switch (value.type()) {
    case core::ValueType::Bool:
        return value.asBool();
    case core::ValueType::Int:
        return value.asInt() != 0;
    case core::ValueType::Float:
        return std::abs(value.asFloat()) > 0.0001f;
    case core::ValueType::String:
    case core::ValueType::Enum:
        return !value.asString().empty();
    case core::ValueType::Color:
        return value.asColor().value() != 0;
    case core::ValueType::None:
        return false;
    }

    return false;
}

bool truthyNode(const ModelNode* node)
{
    if (node == nullptr) {
        return false;
    }

    if (const core::Value* scalar = node->scalar()) {
        return truthyScalar(*scalar);
    }

    if (const auto* object = node->objectValue()) {
        return !object->empty();
    }

    if (const auto* array = node->arrayValue()) {
        return !array->empty();
    }

    return false;
}

struct EvaluatedValue {
    const ModelNode* node = nullptr;
    std::optional<core::Value> scalar;
};

class Scanner {
public:
    explicit Scanner(std::string_view source)
        : source_(source)
    {
    }

    Token next()
    {
        skipWhitespace();
        if (position_ >= source_.size()) {
            return Token {.type = TokenType::End, .position = static_cast<int>(position_)};
        }

        const char ch = source_[position_];
        const int tokenPosition = static_cast<int>(position_);

        if (ch == '(') {
            ++position_;
            return Token {.type = TokenType::LeftParen, .text = "(", .position = tokenPosition};
        }
        if (ch == ')') {
            ++position_;
            return Token {.type = TokenType::RightParen, .text = ")", .position = tokenPosition};
        }
        if (ch == '?') {
            ++position_;
            return Token {.type = TokenType::Question, .text = "?", .position = tokenPosition};
        }
        if (ch == ':') {
            ++position_;
            return Token {.type = TokenType::Colon, .text = ":", .position = tokenPosition};
        }
        if (ch == '+') {
            ++position_;
            return Token {.type = TokenType::Plus, .text = "+", .position = tokenPosition};
        }
        if (ch == '-') {
            ++position_;
            return Token {.type = TokenType::Minus, .text = "-", .position = tokenPosition};
        }
        if (ch == '*') {
            ++position_;
            return Token {.type = TokenType::Star, .text = "*", .position = tokenPosition};
        }
        if (ch == '/') {
            ++position_;
            return Token {.type = TokenType::Slash, .text = "/", .position = tokenPosition};
        }
        if (ch == '!') {
            ++position_;
            if (match('=')) {
                return Token {.type = TokenType::BangEqual, .text = "!=", .position = tokenPosition};
            }
            return Token {.type = TokenType::Bang, .text = "!", .position = tokenPosition};
        }
        if (ch == '=') {
            ++position_;
            if (match('=')) {
                return Token {.type = TokenType::EqualEqual, .text = "==", .position = tokenPosition};
            }
            return Token {.type = TokenType::Error, .text = "=", .position = tokenPosition};
        }
        if (ch == '<') {
            ++position_;
            if (match('=')) {
                return Token {.type = TokenType::LessEqual, .text = "<=", .position = tokenPosition};
            }
            return Token {.type = TokenType::Less, .text = "<", .position = tokenPosition};
        }
        if (ch == '>') {
            ++position_;
            if (match('=')) {
                return Token {.type = TokenType::GreaterEqual, .text = ">=", .position = tokenPosition};
            }
            return Token {.type = TokenType::Greater, .text = ">", .position = tokenPosition};
        }
        if (ch == '&') {
            ++position_;
            if (match('&')) {
                return Token {.type = TokenType::AndAnd, .text = "&&", .position = tokenPosition};
            }
            return Token {.type = TokenType::Error, .text = "&", .position = tokenPosition};
        }
        if (ch == '|') {
            ++position_;
            if (match('|')) {
                return Token {.type = TokenType::OrOr, .text = "||", .position = tokenPosition};
            }
            return Token {.type = TokenType::Error, .text = "|", .position = tokenPosition};
        }

        if (ch == '"') {
            return readString();
        }

        if (ch == '#') {
            return readColor();
        }

        if (std::isdigit(static_cast<unsigned char>(ch))
            || (ch == '.'
                && position_ + 1 < source_.size()
                && std::isdigit(static_cast<unsigned char>(source_[position_ + 1])))) {
            return readNumber();
        }

        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            return readIdentifier();
        }

        ++position_;
        return Token {.type = TokenType::Error, .text = std::string(1, ch), .position = tokenPosition};
    }

private:
    void skipWhitespace()
    {
        while (position_ < source_.size()) {
            const char ch = source_[position_];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                break;
            }
            ++position_;
        }
    }

    bool match(char expected)
    {
        if (position_ >= source_.size() || source_[position_] != expected) {
            return false;
        }

        ++position_;
        return true;
    }

    Token readString()
    {
        const int tokenPosition = static_cast<int>(position_);
        ++position_;

        std::string text;
        while (position_ < source_.size() && source_[position_] != '"') {
            if (source_[position_] == '\\' && position_ + 1 < source_.size()) {
                ++position_;
                const char escaped = source_[position_++];
                switch (escaped) {
                case 'n':
                    text.push_back('\n');
                    break;
                case 't':
                    text.push_back('\t');
                    break;
                case '\\':
                    text.push_back('\\');
                    break;
                case '"':
                    text.push_back('"');
                    break;
                default:
                    text.push_back(escaped);
                    break;
                }
                continue;
            }

            text.push_back(source_[position_++]);
        }

        if (position_ >= source_.size()) {
            return Token {.type = TokenType::Error, .text = "\"", .position = tokenPosition};
        }

        ++position_;
        return Token {.type = TokenType::String, .text = std::move(text), .position = tokenPosition};
    }

    Token readColor()
    {
        const int tokenPosition = static_cast<int>(position_);
        ++position_;

        std::string text;
        while (position_ < source_.size()) {
            const char ch = source_[position_];
            if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                break;
            }
            text.push_back(ch);
            ++position_;
        }

        if ((text.size() != 6 && text.size() != 8) || !parseColorHex(text).has_value()) {
            return Token {.type = TokenType::Error, .text = "#" + text, .position = tokenPosition};
        }

        return Token {.type = TokenType::Color, .text = std::move(text), .position = tokenPosition};
    }

    Token readNumber()
    {
        const int tokenPosition = static_cast<int>(position_);
        std::string text;
        bool seenDot = false;

        while (position_ < source_.size()) {
            const char ch = source_[position_];
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                text.push_back(ch);
                ++position_;
                continue;
            }

            if (ch == '.' && !seenDot) {
                seenDot = true;
                text.push_back(ch);
                ++position_;
                continue;
            }

            break;
        }

        return Token {.type = TokenType::Number, .text = std::move(text), .position = tokenPosition};
    }

    Token readIdentifier()
    {
        const int tokenPosition = static_cast<int>(position_);
        std::string text;

        while (position_ < source_.size()) {
            const char ch = source_[position_];
            if (std::isalnum(static_cast<unsigned char>(ch))
                || ch == '_' || ch == '.') {
                text.push_back(ch);
                ++position_;
                continue;
            }

            break;
        }

        return Token {.type = TokenType::Identifier, .text = std::move(text), .position = tokenPosition};
    }

    std::string_view source_;
    std::size_t position_ = 0;
};

class Parser {
public:
    explicit Parser(std::string_view source)
        : scanner_(source)
        , current_(scanner_.next())
    {
    }

    std::unique_ptr<BindingExpression::Node> parse(std::string* errorMessage)
    {
        auto expression = parseConditional(errorMessage);
        if (!expression) {
            return nullptr;
        }

        if (current_.type != TokenType::End) {
            if (errorMessage != nullptr) {
                *errorMessage = "unexpected token '" + current_.text + "'";
            }
            return nullptr;
        }

        return expression;
    }

private:
    std::unique_ptr<BindingExpression::Node> parseConditional(std::string* errorMessage)
    {
        auto condition = parseLogicalOr(errorMessage);
        if (!condition) {
            return nullptr;
        }

        if (current_.type != TokenType::Question) {
            return condition;
        }

        consume();
        auto whenTrue = parseConditional(errorMessage);
        if (!whenTrue) {
            return nullptr;
        }

        if (current_.type != TokenType::Colon) {
            if (errorMessage != nullptr) {
                *errorMessage = "expected ':' after conditional branch";
            }
            return nullptr;
        }

        consume();
        auto whenFalse = parseConditional(errorMessage);
        if (!whenFalse) {
            return nullptr;
        }

        auto node = std::make_unique<BindingExpression::Node>();
        node->kind = BindingExpression::Node::Kind::Conditional;
        node->left = std::move(condition);
        node->right = std::move(whenTrue);
        node->extra = std::move(whenFalse);
        return node;
    }

    std::unique_ptr<BindingExpression::Node> parseLogicalOr(std::string* errorMessage)
    {
        auto left = parseLogicalAnd(errorMessage);
        while (left && current_.type == TokenType::OrOr) {
            consume();
            auto right = parseLogicalAnd(errorMessage);
            if (!right) {
                return nullptr;
            }
            left = makeBinary(
                BindingExpression::BinaryOperator::LogicalOr,
                std::move(left),
                std::move(right));
        }
        return left;
    }

    std::unique_ptr<BindingExpression::Node> parseLogicalAnd(std::string* errorMessage)
    {
        auto left = parseEquality(errorMessage);
        while (left && current_.type == TokenType::AndAnd) {
            consume();
            auto right = parseEquality(errorMessage);
            if (!right) {
                return nullptr;
            }
            left = makeBinary(
                BindingExpression::BinaryOperator::LogicalAnd,
                std::move(left),
                std::move(right));
        }
        return left;
    }

    std::unique_ptr<BindingExpression::Node> parseEquality(std::string* errorMessage)
    {
        auto left = parseComparison(errorMessage);
        while (left
            && (current_.type == TokenType::EqualEqual || current_.type == TokenType::BangEqual)) {
            const TokenType op = current_.type;
            consume();
            auto right = parseComparison(errorMessage);
            if (!right) {
                return nullptr;
            }

            left = makeBinary(
                op == TokenType::EqualEqual
                    ? BindingExpression::BinaryOperator::Equal
                    : BindingExpression::BinaryOperator::NotEqual,
                std::move(left),
                std::move(right));
        }
        return left;
    }

    std::unique_ptr<BindingExpression::Node> parseComparison(std::string* errorMessage)
    {
        auto left = parseTerm(errorMessage);
        while (left
            && (current_.type == TokenType::Less
                || current_.type == TokenType::LessEqual
                || current_.type == TokenType::Greater
                || current_.type == TokenType::GreaterEqual)) {
            const TokenType op = current_.type;
            consume();
            auto right = parseTerm(errorMessage);
            if (!right) {
                return nullptr;
            }

            BindingExpression::BinaryOperator binaryOperator =
                BindingExpression::BinaryOperator::Less;
            if (op == TokenType::LessEqual) {
                binaryOperator = BindingExpression::BinaryOperator::LessEqual;
            } else if (op == TokenType::Greater) {
                binaryOperator = BindingExpression::BinaryOperator::Greater;
            } else if (op == TokenType::GreaterEqual) {
                binaryOperator = BindingExpression::BinaryOperator::GreaterEqual;
            }

            left = makeBinary(binaryOperator, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<BindingExpression::Node> parseTerm(std::string* errorMessage)
    {
        auto left = parseFactor(errorMessage);
        while (left
            && (current_.type == TokenType::Plus || current_.type == TokenType::Minus)) {
            const TokenType op = current_.type;
            consume();
            auto right = parseFactor(errorMessage);
            if (!right) {
                return nullptr;
            }

            left = makeBinary(
                op == TokenType::Plus
                    ? BindingExpression::BinaryOperator::Add
                    : BindingExpression::BinaryOperator::Subtract,
                std::move(left),
                std::move(right));
        }
        return left;
    }

    std::unique_ptr<BindingExpression::Node> parseFactor(std::string* errorMessage)
    {
        auto left = parseUnary(errorMessage);
        while (left
            && (current_.type == TokenType::Star || current_.type == TokenType::Slash)) {
            const TokenType op = current_.type;
            consume();
            auto right = parseUnary(errorMessage);
            if (!right) {
                return nullptr;
            }

            left = makeBinary(
                op == TokenType::Star
                    ? BindingExpression::BinaryOperator::Multiply
                    : BindingExpression::BinaryOperator::Divide,
                std::move(left),
                std::move(right));
        }
        return left;
    }

    std::unique_ptr<BindingExpression::Node> parseUnary(std::string* errorMessage)
    {
        if (current_.type == TokenType::Bang || current_.type == TokenType::Minus) {
            const TokenType op = current_.type;
            consume();
            auto operand = parseUnary(errorMessage);
            if (!operand) {
                return nullptr;
            }

            auto node = std::make_unique<BindingExpression::Node>();
            node->kind = BindingExpression::Node::Kind::Unary;
            node->unaryOperator = op == TokenType::Bang
                ? BindingExpression::UnaryOperator::Not
                : BindingExpression::UnaryOperator::Negate;
            node->left = std::move(operand);
            return node;
        }

        return parsePrimary(errorMessage);
    }

    std::unique_ptr<BindingExpression::Node> parsePrimary(std::string* errorMessage)
    {
        if (current_.type == TokenType::Identifier) {
            auto node = std::make_unique<BindingExpression::Node>();
            node->kind = BindingExpression::Node::Kind::Path;
            node->path = current_.text;
            if (current_.text == "true") {
                node->kind = BindingExpression::Node::Kind::Literal;
                node->literalValue = core::Value(true);
                node->path.clear();
            } else if (current_.text == "false") {
                node->kind = BindingExpression::Node::Kind::Literal;
                node->literalValue = core::Value(false);
                node->path.clear();
            }

            consume();
            return node;
        }

        if (current_.type == TokenType::Number) {
            auto node = std::make_unique<BindingExpression::Node>();
            node->kind = BindingExpression::Node::Kind::Literal;
            if (current_.text.find('.') == std::string::npos) {
                node->literalValue = core::Value(std::stoi(current_.text));
            } else {
                node->literalValue = core::Value(std::stof(current_.text));
            }
            consume();
            return node;
        }

        if (current_.type == TokenType::Color) {
            auto node = std::make_unique<BindingExpression::Node>();
            node->kind = BindingExpression::Node::Kind::Literal;
            const std::optional<core::Color> color = parseColorHex(current_.text);
            if (!color.has_value()) {
                if (errorMessage != nullptr) {
                    *errorMessage = "invalid color literal '#" + current_.text + "'";
                }
                return nullptr;
            }
            node->literalValue = core::Value(*color);
            consume();
            return node;
        }

        if (current_.type == TokenType::String) {
            auto node = std::make_unique<BindingExpression::Node>();
            node->kind = BindingExpression::Node::Kind::Literal;
            node->literalValue = core::Value(current_.text);
            consume();
            return node;
        }

        if (current_.type == TokenType::LeftParen) {
            consume();
            auto node = parseConditional(errorMessage);
            if (!node) {
                return nullptr;
            }
            if (current_.type != TokenType::RightParen) {
                if (errorMessage != nullptr) {
                    *errorMessage = "expected ')'";
                }
                return nullptr;
            }
            consume();
            return node;
        }

        if (errorMessage != nullptr) {
            *errorMessage = "unexpected token '" + current_.text + "'";
        }
        return nullptr;
    }

    static std::unique_ptr<BindingExpression::Node> makeBinary(
        BindingExpression::BinaryOperator op,
        std::unique_ptr<BindingExpression::Node> left,
        std::unique_ptr<BindingExpression::Node> right)
    {
        auto node = std::make_unique<BindingExpression::Node>();
        node->kind = BindingExpression::Node::Kind::Binary;
        node->binaryOperator = op;
        node->left = std::move(left);
        node->right = std::move(right);
        return node;
    }

    void consume()
    {
        current_ = scanner_.next();
    }

    Scanner scanner_;
    Token current_;
};

void collectDependencies(
    const BindingExpression::Node* node,
    std::vector<std::string>& dependencies,
    std::unordered_set<std::string>& seen)
{
    if (node == nullptr) {
        return;
    }

    if (node->kind == BindingExpression::Node::Kind::Path) {
        if (seen.insert(node->path).second) {
            dependencies.push_back(node->path);
        }
        return;
    }

    collectDependencies(node->left.get(), dependencies, seen);
    collectDependencies(node->right.get(), dependencies, seen);
    collectDependencies(node->extra.get(), dependencies, seen);
}

std::optional<float> numericValue(const EvaluatedValue& value)
{
    if (!value.scalar.has_value()) {
        return std::nullopt;
    }

    if (value.scalar->type() == core::ValueType::Int) {
        return static_cast<float>(value.scalar->asInt());
    }
    if (value.scalar->type() == core::ValueType::Float) {
        return value.scalar->asFloat();
    }

    return std::nullopt;
}

bool isIntValue(const EvaluatedValue& value)
{
    return value.scalar.has_value() && value.scalar->type() == core::ValueType::Int;
}

std::optional<std::string> stringValue(const EvaluatedValue& value)
{
    if (!value.scalar.has_value()) {
        return std::nullopt;
    }

    if (value.scalar->type() == core::ValueType::String
        || value.scalar->type() == core::ValueType::Enum) {
        return value.scalar->asString();
    }

    return std::nullopt;
}

bool looseEquals(const EvaluatedValue& lhs, const EvaluatedValue& rhs)
{
    const std::optional<float> lhsNumber = numericValue(lhs);
    const std::optional<float> rhsNumber = numericValue(rhs);
    if (lhsNumber.has_value() && rhsNumber.has_value()) {
        return std::abs(*lhsNumber - *rhsNumber) <= 0.0001f;
    }

    if (!lhs.scalar.has_value() || !rhs.scalar.has_value()) {
        return truthyNode(lhs.node) == truthyNode(rhs.node);
    }

    if (lhs.scalar->type() != rhs.scalar->type()) {
        return false;
    }

    switch (lhs.scalar->type()) {
    case core::ValueType::None:
        return true;
    case core::ValueType::Bool:
        return lhs.scalar->asBool() == rhs.scalar->asBool();
    case core::ValueType::Int:
        return lhs.scalar->asInt() == rhs.scalar->asInt();
    case core::ValueType::Float:
        return std::abs(lhs.scalar->asFloat() - rhs.scalar->asFloat()) <= 0.0001f;
    case core::ValueType::String:
    case core::ValueType::Enum:
        return lhs.scalar->asString() == rhs.scalar->asString();
    case core::ValueType::Color:
        return lhs.scalar->asColor() == rhs.scalar->asColor();
    }

    return false;
}

EvaluatedValue evaluateNode(
    const BindingExpression::Node* node,
    const BindingExpression::NodeResolver& resolver);

bool truthyValue(const EvaluatedValue& value);

std::optional<core::Value> evaluateUnary(
    BindingExpression::UnaryOperator op,
    const EvaluatedValue& operand)
{
    if (op == BindingExpression::UnaryOperator::Not) {
        return core::Value(!truthyValue(operand));
    }

    const std::optional<float> numeric = numericValue(operand);
    if (!numeric.has_value()) {
        return std::nullopt;
    }

    if (isIntValue(operand)) {
        return core::Value(-operand.scalar->asInt());
    }

    return core::Value(-*numeric);
}

bool truthyValue(const EvaluatedValue& value)
{
    if (value.scalar.has_value()) {
        return truthyScalar(*value.scalar);
    }

    return truthyNode(value.node);
}

std::optional<core::Value> evaluateBinary(
    BindingExpression::BinaryOperator op,
    const EvaluatedValue& lhs,
    const EvaluatedValue& rhs)
{
    if (op == BindingExpression::BinaryOperator::LogicalAnd) {
        return core::Value(truthyValue(lhs) && truthyValue(rhs));
    }
    if (op == BindingExpression::BinaryOperator::LogicalOr) {
        return core::Value(truthyValue(lhs) || truthyValue(rhs));
    }

    if (op == BindingExpression::BinaryOperator::Equal) {
        return core::Value(looseEquals(lhs, rhs));
    }
    if (op == BindingExpression::BinaryOperator::NotEqual) {
        return core::Value(!looseEquals(lhs, rhs));
    }

    if (op == BindingExpression::BinaryOperator::Add) {
        const std::optional<std::string> lhsText = stringValue(lhs);
        const std::optional<std::string> rhsText = stringValue(rhs);
        if (lhsText.has_value() || rhsText.has_value()) {
            if (!lhsText.has_value() || !rhsText.has_value()) {
                return std::nullopt;
            }
            return core::Value(*lhsText + *rhsText);
        }
    }

    const std::optional<float> lhsNumber = numericValue(lhs);
    const std::optional<float> rhsNumber = numericValue(rhs);
    if (!lhsNumber.has_value() || !rhsNumber.has_value()) {
        return std::nullopt;
    }

    switch (op) {
    case BindingExpression::BinaryOperator::Add:
        if (isIntValue(lhs) && isIntValue(rhs)) {
            return core::Value(lhs.scalar->asInt() + rhs.scalar->asInt());
        }
        return core::Value(*lhsNumber + *rhsNumber);
    case BindingExpression::BinaryOperator::Subtract:
        if (isIntValue(lhs) && isIntValue(rhs)) {
            return core::Value(lhs.scalar->asInt() - rhs.scalar->asInt());
        }
        return core::Value(*lhsNumber - *rhsNumber);
    case BindingExpression::BinaryOperator::Multiply:
        if (isIntValue(lhs) && isIntValue(rhs)) {
            return core::Value(lhs.scalar->asInt() * rhs.scalar->asInt());
        }
        return core::Value(*lhsNumber * *rhsNumber);
    case BindingExpression::BinaryOperator::Divide:
        if (std::abs(*rhsNumber) <= 0.0001f) {
            return std::nullopt;
        }
        return core::Value(*lhsNumber / *rhsNumber);
    case BindingExpression::BinaryOperator::Less:
        return core::Value(*lhsNumber < *rhsNumber);
    case BindingExpression::BinaryOperator::LessEqual:
        return core::Value(*lhsNumber <= *rhsNumber);
    case BindingExpression::BinaryOperator::Greater:
        return core::Value(*lhsNumber > *rhsNumber);
    case BindingExpression::BinaryOperator::GreaterEqual:
        return core::Value(*lhsNumber >= *rhsNumber);
    case BindingExpression::BinaryOperator::Equal:
    case BindingExpression::BinaryOperator::NotEqual:
    case BindingExpression::BinaryOperator::LogicalAnd:
    case BindingExpression::BinaryOperator::LogicalOr:
        break;
    }

    return std::nullopt;
}

EvaluatedValue evaluateNode(
    const BindingExpression::Node* node,
    const BindingExpression::NodeResolver& resolver)
{
    if (node == nullptr) {
        return {};
    }

    switch (node->kind) {
    case BindingExpression::Node::Kind::Literal:
        return EvaluatedValue {.scalar = node->literalValue};
    case BindingExpression::Node::Kind::Path: {
        const ModelNode* resolvedNode = resolver ? resolver(node->path) : nullptr;
        return EvaluatedValue {
            .node = resolvedNode,
            .scalar = resolvedNode != nullptr && resolvedNode->scalar() != nullptr
                ? std::optional<core::Value>(*resolvedNode->scalar())
                : std::nullopt,
        };
    }
    case BindingExpression::Node::Kind::Unary: {
        const EvaluatedValue operand = evaluateNode(node->left.get(), resolver);
        return EvaluatedValue {.scalar = evaluateUnary(node->unaryOperator, operand)};
    }
    case BindingExpression::Node::Kind::Binary: {
        const EvaluatedValue left = evaluateNode(node->left.get(), resolver);
        const EvaluatedValue right = evaluateNode(node->right.get(), resolver);
        return EvaluatedValue {.scalar = evaluateBinary(node->binaryOperator, left, right)};
    }
    case BindingExpression::Node::Kind::Conditional: {
        const EvaluatedValue condition = evaluateNode(node->left.get(), resolver);
        const BindingExpression::Node* branch = truthyValue(condition)
            ? node->right.get()
            : node->extra.get();
        return evaluateNode(branch, resolver);
    }
    }

    return {};
}

} // namespace

BindingExpression::BindingExpression(
    std::string source,
    std::unique_ptr<Node> root,
    std::vector<std::string> dependencies,
    std::optional<std::string> directPath)
    : source_(std::move(source))
    , root_(std::move(root))
    , dependencies_(std::move(dependencies))
    , directPath_(std::move(directPath))
{
}

std::shared_ptr<const BindingExpression> BindingExpression::compile(
    std::string_view source,
    std::string* errorMessage)
{
    Parser parser(source);
    auto root = parser.parse(errorMessage);
    if (!root) {
        return {};
    }

    std::vector<std::string> dependencies;
    std::unordered_set<std::string> seenDependencies;
    collectDependencies(root.get(), dependencies, seenDependencies);

    std::optional<std::string> directPath;
    if (root->kind == Node::Kind::Path) {
        directPath = root->path;
    }

    return std::shared_ptr<const BindingExpression>(new BindingExpression(
        std::string(source),
        std::move(root),
        std::move(dependencies),
        std::move(directPath)));
}

std::optional<core::Value> BindingExpression::evaluateScalar(const NodeResolver& resolver) const
{
    const EvaluatedValue value = evaluateNode(root_.get(), resolver);
    return value.scalar;
}

bool BindingExpression::evaluateTruthy(const NodeResolver& resolver) const
{
    return truthyValue(evaluateNode(root_.get(), resolver));
}

} // namespace tinalux::markup::detail
