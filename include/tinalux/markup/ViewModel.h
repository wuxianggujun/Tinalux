#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
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
    using Action = core::InteractionHandler;

    enum class Kind : std::uint8_t {
        Null,
        Scalar,
        Object,
        Array,
        Action,
    };

    ModelNode() = default;
    explicit ModelNode(core::Value scalar);
    explicit ModelNode(Action action);

    static ModelNode object(Object object);
    static ModelNode array(Array array);

    [[nodiscard]] Kind kind() const { return kind_; }
    [[nodiscard]] bool isNull() const { return kind_ == Kind::Null; }
    [[nodiscard]] bool isScalar() const { return kind_ == Kind::Scalar; }
    [[nodiscard]] bool isObject() const { return kind_ == Kind::Object; }
    [[nodiscard]] bool isArray() const { return kind_ == Kind::Array; }
    [[nodiscard]] bool isAction() const { return kind_ == Kind::Action; }

    [[nodiscard]] const core::Value* scalar() const;
    [[nodiscard]] const Object* objectValue() const;
    [[nodiscard]] const Array* arrayValue() const;
    [[nodiscard]] const Action* actionValue() const;
    [[nodiscard]] const ModelNode* child(std::string_view name) const;

private:
    Kind kind_ = Kind::Null;
    std::variant<std::monostate, core::Value, Object, Array, Action> data_;
};

namespace actions {

struct Binding {
    std::string path;
    ModelNode::Action action;
};

namespace detail {

template <typename T>
inline constexpr bool alwaysFalse = false;

template <typename T>
struct CallableTraits : CallableTraits<decltype(&T::operator())> {};

template <typename Return, typename... Args>
struct CallableTraits<Return(Args...)> {
    using ReturnType = Return;
    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t Index>
    using Arg = std::tuple_element_t<Index, std::tuple<Args...>>;
};

template <typename Return, typename... Args>
struct CallableTraits<Return (*)(Args...)> : CallableTraits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct CallableTraits<Return (Class::*)(Args...)> : CallableTraits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct CallableTraits<Return (Class::*)(Args...) const> : CallableTraits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct CallableTraits<Return (Class::*)(Args...) noexcept> : CallableTraits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct CallableTraits<Return (Class::*)(Args...) const noexcept> : CallableTraits<Return(Args...)> {};

template <typename Handler>
ModelNode::Action makeAction(Handler&& handler)
{
    using HandlerType = std::decay_t<Handler>;

    if constexpr (std::is_same_v<HandlerType, ModelNode::Action>) {
        return std::forward<Handler>(handler);
    } else {
        using Traits = CallableTraits<HandlerType>;
        static_assert(
            std::is_same_v<typename Traits::ReturnType, void>,
            "action handlers must return void");

        if constexpr (Traits::arity == 0) {
            return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value&) mutable {
                std::invoke(handler);
            };
        } else if constexpr (Traits::arity == 1) {
            using RawArg = typename Traits::template Arg<0>;
            using Arg = std::remove_cv_t<std::remove_reference_t<RawArg>>;

            if constexpr (std::is_same_v<Arg, core::Value>) {
                return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value& value) mutable {
                    std::invoke(handler, value);
                };
            } else if constexpr (std::is_same_v<Arg, bool>) {
                return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value& value) mutable {
                    if (value.type() != core::ValueType::Bool) {
                        return;
                    }
                    std::invoke(handler, value.asBool());
                };
            } else if constexpr (std::is_same_v<Arg, int>) {
                return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value& value) mutable {
                    if (value.type() != core::ValueType::Int) {
                        return;
                    }
                    std::invoke(handler, value.asInt());
                };
            } else if constexpr (std::is_same_v<Arg, float>) {
                return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value& value) mutable {
                    if (value.type() != core::ValueType::Float) {
                        return;
                    }
                    std::invoke(handler, value.asFloat());
                };
            } else if constexpr (std::is_same_v<Arg, std::string_view>) {
                return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value& value) mutable {
                    if (value.type() != core::ValueType::String) {
                        return;
                    }
                    const std::string& text = value.asString();
                    std::invoke(handler, std::string_view(text));
                };
            } else if constexpr (std::is_same_v<Arg, std::string>) {
                return [handler = HandlerType(std::forward<Handler>(handler))](const core::Value& value) mutable {
                    if (value.type() != core::ValueType::String) {
                        return;
                    }
                    if constexpr (std::is_reference_v<RawArg>) {
                        std::invoke(handler, value.asString());
                    } else {
                        std::invoke(handler, std::string(value.asString()));
                    }
                };
            } else {
                static_assert(
                    alwaysFalse<HandlerType>,
                    "unsupported action handler argument type; expected core::Value, bool, int, "
                    "float, std::string_view or std::string");
            }
        } else {
            static_assert(
                alwaysFalse<HandlerType>,
                "unsupported action handler signature; expected void() or void(T)");
        }
    }
}

} // namespace detail

template <typename Handler>
ModelNode::Action make(Handler&& handler)
{
    return detail::makeAction(std::forward<Handler>(handler));
}

template <typename Handler>
Binding bind(std::string_view path, Handler&& handler)
{
    return Binding {
        .path = std::string(path),
        .action = make(std::forward<Handler>(handler)),
    };
}

} // namespace actions

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
    bool setAction(std::string_view path, ModelNode::Action action);
    template <
        typename Handler,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<Handler>, ModelNode::Action>>>
    bool setAction(std::string_view path, Handler&& handler)
    {
        return setAction(path, actions::make(std::forward<Handler>(handler)));
    }
    bool setActions(std::initializer_list<actions::Binding> bindings);
    const ModelNode::Action* findAction(std::string_view path) const;

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

template <typename Signature>
class ActionSlot;

template <typename... Args>
class ActionSlot<void(Args...)> {
public:
    constexpr explicit ActionSlot(std::string_view path)
        : path_(path)
    {
    }

    [[nodiscard]] std::string_view path() const { return path_; }

    template <typename Handler>
    [[nodiscard]] actions::Binding bind(Handler&& handler) const
    {
        return actions::bind(path_, std::forward<Handler>(handler));
    }

    template <typename Handler>
    [[nodiscard]] actions::Binding operator()(Handler&& handler) const
    {
        return bind(std::forward<Handler>(handler));
    }

    template <typename Handler>
    bool connect(ViewModel& viewModel, Handler&& handler) const
    {
        return viewModel.setAction(path_, std::forward<Handler>(handler));
    }

    template <typename Handler>
    bool connect(const std::shared_ptr<ViewModel>& viewModel, Handler&& handler) const
    {
        return viewModel != nullptr
            && viewModel->setAction(path_, std::forward<Handler>(handler));
    }

private:
    std::string_view path_;
};

#define TINALUX_ACTION_SLOT(name, signature) \
    const ::tinalux::markup::ActionSlot<signature> name { #name }

#define TINALUX_ACTION_SLOT_PATH(name, path, signature) \
    const ::tinalux::markup::ActionSlot<signature> name { path }

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

struct InteractionBindingDescriptor {
    std::weak_ptr<ui::Widget> widget;
    std::string interactionName;
    std::function<const ModelNode*(
        const std::shared_ptr<ViewModel>&,
        const std::function<const ModelNode*(std::string_view)>&)> evaluateNode;
};

} // namespace detail

} // namespace tinalux::markup
