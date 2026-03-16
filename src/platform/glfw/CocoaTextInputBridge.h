#pragma once

#include <memory>
#include <optional>

#include "tinalux/core/Geometry.h"

struct GLFWwindow;

namespace tinalux::platform {

class GLFWWindow;

class CocoaTextInputBridge {
public:
    virtual ~CocoaTextInputBridge();

    static std::unique_ptr<CocoaTextInputBridge> create(GLFWwindow* window, GLFWWindow& owner);

    virtual void setActive(
        bool active,
        const std::optional<core::Rect>& cursorRect,
        float dpiScale) = 0;
    virtual void setCursorRect(
        const std::optional<core::Rect>& cursorRect,
        float dpiScale) = 0;
};

}  // namespace tinalux::platform
