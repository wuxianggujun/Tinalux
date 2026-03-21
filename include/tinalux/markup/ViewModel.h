#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tinalux/core/Reflect.h"

namespace tinalux::ui {
class Widget;
}

namespace tinalux::markup {

class ViewModel : public std::enable_shared_from_this<ViewModel> {
public:
    using ListenerId = std::uint64_t;
    using ChangeCallback = std::function<void(const core::Value&)>;

    static std::shared_ptr<ViewModel> create();

    bool setValue(std::string_view path, const core::Value& value);
    const core::Value* findValue(std::string_view path) const;

    bool setString(std::string_view path, std::string value);
    bool setBool(std::string_view path, bool value);
    bool setInt(std::string_view path, int value);
    bool setFloat(std::string_view path, float value);
    bool setColor(std::string_view path, core::Color value);
    bool setEnum(std::string_view path, std::string name);

    ListenerId addListener(std::string_view path, ChangeCallback callback);
    void removeListener(ListenerId id);

private:
    struct ListenerEntry {
        std::string path;
        ChangeCallback callback;
    };

    std::unordered_map<std::string, core::Value> values_;
    std::unordered_map<ListenerId, ListenerEntry> listeners_;
    ListenerId nextListenerId_ = 1;
};

namespace detail {

struct BindingDescriptor {
    std::weak_ptr<ui::Widget> widget;
    std::string propertyName;
    std::string path;
    core::ValueType expectedType = core::ValueType::None;
    std::function<void(const core::Value&)> apply;
};

} // namespace detail

} // namespace tinalux::markup
