#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "tinalux/core/Geometry.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::platform {

using GLGetProcFn = void* (*)(const char* name);
using VulkanGetInstanceProcFn = void* (*)(void* instance, const char* name);
using EventCallback = std::function<void(core::Event&)>;

enum class GraphicsAPI {
    OpenGL,
    None,
};

struct WindowConfig {
    int width = 960;
    int height = 640;
    const char* title = "Tinalux";
    bool resizable = true;
    bool vsync = true;
    GraphicsAPI graphicsApi = GraphicsAPI::OpenGL;
};

class Window {
public:
    virtual ~Window() = default;

    virtual bool shouldClose() const = 0;
    virtual void pollEvents() = 0;
    virtual void waitEventsTimeout(double timeoutSeconds) = 0;
    virtual void swapBuffers() = 0;
    virtual void requestClose() = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int framebufferWidth() const = 0;
    virtual int framebufferHeight() const = 0;
    virtual float dpiScale() const = 0;

    virtual void setClipboardText(const std::string& text) = 0;
    virtual std::string clipboardText() const = 0;
    virtual void setTextInputActive(bool active)
    {
        (void)active;
    }
    virtual void setTextInputCursorRect(const std::optional<core::Rect>& rect)
    {
        (void)rect;
    }

    virtual void setEventCallback(EventCallback callback) = 0;
    virtual GLGetProcFn glGetProcAddress() const = 0;
    virtual bool vulkanSupported() const
    {
        return false;
    }
    virtual std::vector<std::string> requiredVulkanInstanceExtensions() const
    {
        return {};
    }
    virtual VulkanGetInstanceProcFn vulkanGetInstanceProcAddress() const
    {
        return nullptr;
    }
    virtual bool vulkanPresentationSupported(void* instance, void* physicalDevice, uint32_t queueFamilyIndex) const
    {
        (void)instance;
        (void)physicalDevice;
        (void)queueFamilyIndex;
        return false;
    }
    virtual void* createVulkanWindowSurface(void* instance) const
    {
        (void)instance;
        return nullptr;
    }
    virtual void destroyVulkanWindowSurface(void* instance, void* surface) const
    {
        (void)instance;
        (void)surface;
    }
};

std::unique_ptr<Window> createWindow(const WindowConfig& config);

}  // namespace tinalux::platform
