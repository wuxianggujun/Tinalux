#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tinalux/core/Reflect.h"
#include "tinalux/markup/ViewModel.h"

namespace tinalux::markup::detail {

class BindingExpression {
public:
    using NodeResolver = std::function<const ModelNode*(std::string_view)>;

    enum class UnaryOperator : std::uint8_t {
        Negate,
        Not,
    };

    enum class BinaryOperator : std::uint8_t {
        Add,
        Subtract,
        Multiply,
        Divide,
        Equal,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        LogicalAnd,
        LogicalOr,
    };

    struct Node {
        enum class Kind : std::uint8_t {
            Literal,
            Path,
            Unary,
            Binary,
            Conditional,
        };

        Kind kind = Kind::Literal;
        core::Value literalValue;
        std::string path;
        UnaryOperator unaryOperator = UnaryOperator::Negate;
        BinaryOperator binaryOperator = BinaryOperator::Add;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        std::unique_ptr<Node> extra;
    };

    static std::shared_ptr<const BindingExpression> compile(
        std::string_view source,
        std::string* errorMessage);

    [[nodiscard]] const std::vector<std::string>& dependencies() const { return dependencies_; }
    [[nodiscard]] std::string_view source() const { return source_; }
    [[nodiscard]] bool isDirectPath() const { return directPath_.has_value(); }
    [[nodiscard]] std::string_view directPath() const
    {
        return directPath_.has_value() ? *directPath_ : std::string_view {};
    }

    [[nodiscard]] std::optional<core::Value> evaluateScalar(const NodeResolver& resolver) const;
    [[nodiscard]] bool evaluateTruthy(const NodeResolver& resolver) const;

private:
    BindingExpression(
        std::string source,
        std::unique_ptr<Node> root,
        std::vector<std::string> dependencies,
        std::optional<std::string> directPath);

    std::string source_;
    std::unique_ptr<Node> root_;
    std::vector<std::string> dependencies_;
    std::optional<std::string> directPath_;
};

} // namespace tinalux::markup::detail
