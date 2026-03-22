#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "tinalux/core/Geometry.h"

namespace tinalux::ui {
class Widget;
struct Theme;
}

namespace tinalux::core {

// ---------------------------------------------------------------------------
// Value — type-erased property value for the DSL/reflection system
// ---------------------------------------------------------------------------

enum class ValueType : std::uint8_t {
    None,
    Bool,
    Int,
    Float,
    String,
    Color,
    Enum, // stored as string, resolved by PropertyInfo setter
};

class Value {
public:
    Value() = default;

    explicit Value(bool v)
        : type_(ValueType::Bool)
        , data_(v)
    {
    }

    explicit Value(int v)
        : type_(ValueType::Int)
        , data_(v)
    {
    }

    explicit Value(float v)
        : type_(ValueType::Float)
        , data_(v)
    {
    }

    explicit Value(std::string v)
        : type_(ValueType::String)
        , data_(std::move(v))
    {
    }

    explicit Value(Color v)
        : type_(ValueType::Color)
        , data_(v)
    {
    }

    static Value enumValue(std::string name)
    {
        Value v;
        v.type_ = ValueType::Enum;
        v.data_ = std::move(name);
        return v;
    }

    [[nodiscard]] ValueType type() const { return type_; }
    [[nodiscard]] bool isNone() const { return type_ == ValueType::None; }

    [[nodiscard]] bool asBool() const { return std::get<bool>(data_); }
    [[nodiscard]] int asInt() const { return std::get<int>(data_); }
    [[nodiscard]] float asFloat() const { return std::get<float>(data_); }

    [[nodiscard]] float asNumber() const
    {
        if (type_ == ValueType::Float)
            return std::get<float>(data_);
        if (type_ == ValueType::Int)
            return static_cast<float>(std::get<int>(data_));
        return 0.0f;
    }

    [[nodiscard]] const std::string& asString() const { return std::get<std::string>(data_); }
    [[nodiscard]] Color asColor() const { return std::get<Color>(data_); }

private:
    ValueType type_ = ValueType::None;
    std::variant<std::monostate, bool, int, float, std::string, Color> data_;
};

// ---------------------------------------------------------------------------
// PropertyInfo — describes a single settable property on a widget type
// ---------------------------------------------------------------------------

struct PropertyInfo {
    std::string name;
    ValueType expectedType = ValueType::None;
    std::function<void(ui::Widget&, const Value&)> setter;
    std::function<std::optional<Value>(const ui::Widget&)> getter;
    int applicationOrder = 0;
};

struct StylePropertyInfo {
    std::string name;
    ValueType expectedType = ValueType::None;
    std::function<void(ui::Widget&, const Value&, const ui::Theme&)> setter;
};

using InteractionHandler = std::function<void(const Value&)>;
using ChildAttachmentHandler = std::function<void(ui::Widget&, std::shared_ptr<ui::Widget>)>;

struct InteractionInfo {
    std::string name;
    std::string boundProperty;
    ValueType payloadType = ValueType::None;
    std::function<void(ui::Widget&, InteractionHandler)> bind;
};

enum class ChildAttachmentPolicy : std::uint8_t {
    None,
    Single,
    Multiple,
};

struct ChildAttachmentInfo {
    ChildAttachmentPolicy policy = ChildAttachmentPolicy::None;
    ChildAttachmentHandler attach;

    [[nodiscard]] bool acceptsChildren() const
    {
        return policy != ChildAttachmentPolicy::None && static_cast<bool>(attach);
    }
};

// ---------------------------------------------------------------------------
// TypeInfo — describes a widget type for the registry
// ---------------------------------------------------------------------------

struct TypeInfo {
    std::string name;
    std::function<std::shared_ptr<ui::Widget>()> factory;
    std::vector<PropertyInfo> properties;
    std::vector<StylePropertyInfo> styleProperties;
    std::vector<InteractionInfo> interactions;
    ChildAttachmentInfo childAttachment;
    std::vector<std::string> markupPositionalProperties;

    const PropertyInfo* findProperty(std::string_view propName) const
    {
        for (const auto& p : properties) {
            if (p.name == propName)
                return &p;
        }
        return nullptr;
    }

    const StylePropertyInfo* findStyleProperty(std::string_view propName) const
    {
        for (const auto& p : styleProperties) {
            if (p.name == propName)
                return &p;
        }
        return nullptr;
    }

    const InteractionInfo* findInteraction(std::string_view interactionName) const
    {
        for (const auto& interaction : interactions) {
            if (interaction.name == interactionName) {
                return &interaction;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool acceptsChildren() const { return childAttachment.acceptsChildren(); }
};

// ---------------------------------------------------------------------------
// TypeRegistry — global registry of widget types
// ---------------------------------------------------------------------------

class TypeRegistry {
public:
    static TypeRegistry& instance();

    void registerType(TypeInfo info);
    const TypeInfo* findType(std::string_view name) const;
    void clear();

private:
    TypeRegistry() = default;

    struct StringHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view sv) const
        {
            return std::hash<std::string_view> {}(sv);
        }
        std::size_t operator()(const std::string& s) const
        {
            return std::hash<std::string_view> {}(s);
        }
    };

    std::unordered_map<std::string, TypeInfo, StringHash, std::equal_to<>> types_;
};

} // namespace tinalux::core
