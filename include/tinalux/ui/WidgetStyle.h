#pragma once

#include <optional>

namespace tinalux::ui {

enum class WidgetState {
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Focused,
};

template<typename T>
struct StateStyle {
    T normal {};
    std::optional<T> hovered;
    std::optional<T> pressed;
    std::optional<T> disabled;
    std::optional<T> focused;

    const T& resolve(WidgetState state) const
    {
        switch (state) {
        case WidgetState::Hovered:
            return hovered ? *hovered : normal;
        case WidgetState::Pressed:
            if (pressed) {
                return *pressed;
            }
            if (hovered) {
                return *hovered;
            }
            return normal;
        case WidgetState::Disabled:
            return disabled ? *disabled : normal;
        case WidgetState::Focused:
            if (focused) {
                return *focused;
            }
            if (hovered) {
                return *hovered;
            }
            return normal;
        case WidgetState::Normal:
        default:
            return normal;
        }
    }
};

}  // namespace tinalux::ui
