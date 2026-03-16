#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace tinalux::core {

enum class EventType {
    None,
    WindowClose,
    WindowResize,
    WindowFocus,
    KeyPress,
    KeyRelease,
    KeyRepeat,
    MouseButtonPress,
    MouseButtonRelease,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseScroll,
    TextInput,
    TextCompositionStart,
    TextCompositionUpdate,
    TextCompositionEnd,
};

class Event {
public:
    virtual ~Event() = default;
    virtual EventType type() const = 0;

    bool handled = false;
    bool stopPropagation = false;
};

class WindowCloseEvent final : public Event {
public:
    EventType type() const override { return EventType::WindowClose; }
};

class WindowResizeEvent final : public Event {
public:
    WindowResizeEvent(int w, int h)
        : width(w)
        , height(h)
    {
    }

    EventType type() const override { return EventType::WindowResize; }

    int width = 0;
    int height = 0;
};

class WindowFocusEvent final : public Event {
public:
    explicit WindowFocusEvent(bool focusedValue)
        : focused(focusedValue)
    {
    }

    EventType type() const override { return EventType::WindowFocus; }

    bool focused = false;
};

class KeyEvent : public Event {
public:
    KeyEvent(int keyValue, int scancodeValue, int modsValue, EventType t)
        : key(keyValue)
        , scancode(scancodeValue)
        , mods(modsValue)
        , type_(t)
    {
    }

    EventType type() const override { return type_; }

    int key = 0;
    int scancode = 0;
    int mods = 0;

private:
    EventType type_ = EventType::None;
};

class MouseMoveEvent final : public Event {
public:
    MouseMoveEvent(double xValue, double yValue)
        : x(xValue)
        , y(yValue)
    {
    }

    EventType type() const override { return EventType::MouseMove; }

    double x = 0.0;
    double y = 0.0;
};

class MouseCrossEvent final : public Event {
public:
    MouseCrossEvent(double xValue, double yValue, EventType t)
        : x(xValue)
        , y(yValue)
        , type_(t)
    {
    }

    EventType type() const override { return type_; }

    double x = 0.0;
    double y = 0.0;

private:
    EventType type_ = EventType::None;
};

class MouseButtonEvent : public Event {
public:
    MouseButtonEvent(int buttonValue, int modsValue, double xValue, double yValue, EventType t)
        : button(buttonValue)
        , mods(modsValue)
        , x(xValue)
        , y(yValue)
        , type_(t)
    {
    }

    EventType type() const override { return type_; }

    int button = 0;
    int mods = 0;
    double x = 0.0;
    double y = 0.0;

private:
    EventType type_ = EventType::None;
};

class MouseScrollEvent final : public Event {
public:
    MouseScrollEvent(double xOffsetValue, double yOffsetValue)
        : xOffset(xOffsetValue)
        , yOffset(yOffsetValue)
    {
    }

    EventType type() const override { return EventType::MouseScroll; }

    double xOffset = 0.0;
    double yOffset = 0.0;
};

class TextInputEvent final : public Event {
public:
    explicit TextInputEvent(uint32_t codepointValue)
        : codepoint(codepointValue)
        , text(encodeUtf8(codepointValue))
    {
    }

    explicit TextInputEvent(std::string textValue)
        : codepoint(0)
        , text(std::move(textValue))
    {
    }

    EventType type() const override { return EventType::TextInput; }

    uint32_t codepoint = 0;
    std::string text;

private:
    static std::string encodeUtf8(uint32_t codepointValue)
    {
        std::string encoded;
        if (codepointValue <= 0x7F) {
            encoded.push_back(static_cast<char>(codepointValue));
        } else if (codepointValue <= 0x7FF) {
            encoded.push_back(static_cast<char>(0xC0 | (codepointValue >> 6)));
            encoded.push_back(static_cast<char>(0x80 | (codepointValue & 0x3F)));
        } else if (codepointValue <= 0xFFFF) {
            encoded.push_back(static_cast<char>(0xE0 | (codepointValue >> 12)));
            encoded.push_back(static_cast<char>(0x80 | ((codepointValue >> 6) & 0x3F)));
            encoded.push_back(static_cast<char>(0x80 | (codepointValue & 0x3F)));
        } else if (codepointValue <= 0x10FFFF) {
            encoded.push_back(static_cast<char>(0xF0 | (codepointValue >> 18)));
            encoded.push_back(static_cast<char>(0x80 | ((codepointValue >> 12) & 0x3F)));
            encoded.push_back(static_cast<char>(0x80 | ((codepointValue >> 6) & 0x3F)));
            encoded.push_back(static_cast<char>(0x80 | (codepointValue & 0x3F)));
        }
        return encoded;
    }
};

class TextCompositionEvent final : public Event {
public:
    TextCompositionEvent(
        EventType typeValue,
        std::string textValue = {},
        std::optional<std::size_t> caretUtf8OffsetValue = std::nullopt,
        std::optional<std::size_t> targetStartUtf8Value = std::nullopt,
        std::optional<std::size_t> targetEndUtf8Value = std::nullopt,
        bool platformManagedValue = false)
        : type_(typeValue)
        , text(std::move(textValue))
        , caretUtf8Offset(std::move(caretUtf8OffsetValue))
        , targetStartUtf8(std::move(targetStartUtf8Value))
        , targetEndUtf8(std::move(targetEndUtf8Value))
        , platformManaged(platformManagedValue)
    {
    }

    EventType type() const override { return type_; }

    std::string text;
    std::optional<std::size_t> caretUtf8Offset;
    std::optional<std::size_t> targetStartUtf8;
    std::optional<std::size_t> targetEndUtf8;
    bool platformManaged = false;

private:
    EventType type_ = EventType::TextCompositionUpdate;
};

}  // namespace tinalux::core
