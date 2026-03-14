#pragma once

#include <functional>
#include <memory>
#include <string>

namespace tinalux::core {
class Event;
}

namespace tinalux::platform {

using GLGetProcFn = void* (*)(const char* name);
using EventCallback = std::function<void(core::Event&)>;

struct WindowConfig {
    int width = 960;
    int height = 640;
    const char* title = "Tinalux";
    bool resizable = true;
    bool vsync = true;
};

class Window {
public:
    virtual ~Window() = default;

    virtual bool shouldClose() const = 0;
    virtual void pollEvents() = 0;
    virtual void swapBuffers() = 0;
    virtual void requestClose() = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int framebufferWidth() const = 0;
    virtual int framebufferHeight() const = 0;
    virtual float dpiScale() const = 0;

    virtual void setClipboardText(const std::string& text) = 0;
    virtual std::string clipboardText() const = 0;

    virtual void setEventCallback(EventCallback callback) = 0;
    virtual GLGetProcFn glGetProcAddress() const = 0;
};

std::unique_ptr<Window> createWindow(const WindowConfig& config);

}  // namespace tinalux::platform
