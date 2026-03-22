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

#include "tinalux/core/Reflect.h"

namespace tinalux::ui {
class Widget;
}

namespace tinalux::markup {

class ModelNode {
public:
    using Object = std::unordered_map<std::string, ModelNode>;
    using Array = std::vector<ModelNode>;

    enum class Kind : std::uint8_t {
        Null,
        Scalar,
        Object,
        Array,
    };

    ModelNode() = default;
    explicit ModelNode(core::Value scalar);

    static ModelNode object(Object object);
    static ModelNode array(Array array);

    [[nodiscard]] Kind kind() const { return kind_; }
    [[nodiscard]] bool isNull() const { return kind_ == Kind::Null; }
    [[nodiscard]] bool isScalar() const { return kind_ == Kind::Scalar; }
    [[nodiscard]] bool isObject() const { return kind_ == Kind::Object; }
    [[nodiscard]] bool isArray() const { return kind_ == Kind::Array; }

    [[nodiscard]] const core::Value* scalar() const;
    [[nodiscard]] const Object* objectValue() const;
    [[nodiscard]] const Array* arrayValue() const;
    [[nodiscard]] const ModelNode* child(std::string_view name) const;

private:
    Kind kind_ = Kind::Null;
    std::variant<std::monostate, core::Value, Object, Array> data_;
};

class ViewModel : public std::enable_shared_from_this<ViewModel> {
public:
    using ListenerId = std::uint64_t;
    using ChangeCallback = std::function<void(const core::Value&)>;
    using InvalidationCallback = std::function<void()>;
    using Object = ModelNode::Object;
    using Array = ModelNode::Array;

    static std::shared_ptr<ViewModel> create();

    bool setNode(std::string_view path, ModelNode node);
    const ModelNode* findNode(std::string_view path) const;

    bool setArray(std::string_view path, Array array);
    bool setObject(std::string_view path, Object object);
    bool setValue(std::string_view path, const core::Value& value);
    const core::Value* findValue(std::string_view path) const;

    bool setString(std::string_view path, std::string value);
    bool setBool(std::string_view path, bool value);
    bool setInt(std::string_view path, int value);
    bool setFloat(std::string_view path, float value);
    bool setColor(std::string_view path, core::Color value);
    bool setEnum(std::string_view path, std::string name);

    ListenerId addListener(std::string_view path, ChangeCallback callback);
    ListenerId addInvalidationListener(std::string_view path, InvalidationCallback callback);
    void removeListener(ListenerId id);

private:
    struct ValueListenerEntry {
        std::string path;
        ChangeCallback callback;
    };

    struct InvalidationListenerEntry {
        std::string path;
        InvalidationCallback callback;
    };

    ModelNode root_ = ModelNode::object({});
    std::unordered_map<ListenerId, ValueListenerEntry> valueListeners_;
    std::unordered_map<ListenerId, InvalidationListenerEntry> invalidationListeners_;
    ListenerId nextListenerId_ = 1;
};

namespace detail {

struct BindingDescriptor {
    std::weak_ptr<ui::Widget> widget;
    std::string propertyName;
    std::vector<std::string> dependencyPaths;
    std::string writeBackPath;
    core::ValueType expectedType = core::ValueType::None;
    std::function<std::optional<core::Value>(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> evaluate;
    std::function<void(const core::Value&)> apply;
    std::function<bool(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> applyResolved;
};

} // namespace detail

} // namespace tinalux::markup
