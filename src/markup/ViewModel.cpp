#include "tinalux/markup/ViewModel.h"

#include <sstream>

namespace tinalux::markup {

namespace {

std::string normalizeBindingPath(std::string_view path)
{
    const auto trim = [](std::string_view value) {
        std::size_t start = 0;
        while (start < value.size()
            && (value[start] == ' ' || value[start] == '\t'
                || value[start] == '\r' || value[start] == '\n')) {
            ++start;
        }

        std::size_t end = value.size();
        while (end > start
            && (value[end - 1] == ' ' || value[end - 1] == '\t'
                || value[end - 1] == '\r' || value[end - 1] == '\n')) {
            --end;
        }

        return value.substr(start, end - start);
    };

    std::string_view normalized = trim(path);
    constexpr std::string_view kModelPrefix = "model.";
    if (normalized.substr(0, kModelPrefix.size()) == kModelPrefix) {
        normalized.remove_prefix(kModelPrefix.size());
    }

    return std::string(normalized);
}

bool valueEquals(const core::Value& lhs, const core::Value& rhs)
{
    if (lhs.type() != rhs.type()) {
        return false;
    }

    switch (lhs.type()) {
    case core::ValueType::None:
        return true;
    case core::ValueType::Bool:
        return lhs.asBool() == rhs.asBool();
    case core::ValueType::Int:
        return lhs.asInt() == rhs.asInt();
    case core::ValueType::Float:
        return lhs.asFloat() == rhs.asFloat();
    case core::ValueType::String:
    case core::ValueType::Enum:
        return lhs.asString() == rhs.asString();
    case core::ValueType::Color:
        return lhs.asColor() == rhs.asColor();
    }

    return false;
}

} // namespace

std::shared_ptr<ViewModel> ViewModel::create()
{
    return std::make_shared<ViewModel>();
}

bool ViewModel::setValue(std::string_view path, const core::Value& value)
{
    const std::string normalizedPath = normalizeBindingPath(path);
    if (normalizedPath.empty()) {
        return false;
    }

    const auto existing = values_.find(normalizedPath);
    if (existing != values_.end() && valueEquals(existing->second, value)) {
        return false;
    }

    values_[normalizedPath] = value;

    std::vector<ChangeCallback> callbacks;
    callbacks.reserve(listeners_.size());
    for (const auto& [listenerId, listener] : listeners_) {
        (void)listenerId;
        if (listener.path == normalizedPath && listener.callback) {
            callbacks.push_back(listener.callback);
        }
    }

    const core::Value& storedValue = values_.find(normalizedPath)->second;
    for (const auto& callback : callbacks) {
        callback(storedValue);
    }

    return true;
}

const core::Value* ViewModel::findValue(std::string_view path) const
{
    const std::string normalizedPath = normalizeBindingPath(path);
    const auto it = values_.find(normalizedPath);
    return it != values_.end() ? &it->second : nullptr;
}

bool ViewModel::setString(std::string_view path, std::string value)
{
    return setValue(path, core::Value(std::move(value)));
}

bool ViewModel::setBool(std::string_view path, bool value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setInt(std::string_view path, int value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setFloat(std::string_view path, float value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setColor(std::string_view path, core::Color value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setEnum(std::string_view path, std::string name)
{
    return setValue(path, core::Value::enumValue(std::move(name)));
}

ViewModel::ListenerId ViewModel::addListener(
    std::string_view path,
    ChangeCallback callback)
{
    const ListenerId listenerId = nextListenerId_++;
    listeners_[listenerId] = ListenerEntry {
        .path = normalizeBindingPath(path),
        .callback = std::move(callback),
    };
    return listenerId;
}

void ViewModel::removeListener(ListenerId id)
{
    listeners_.erase(id);
}

} // namespace tinalux::markup
