#pragma once

#include "tinalux/platform/Window.h"

struct GLFWwindow;

namespace tinalux::platform {

class GLFWWindow final : public Window {
public:
    explicit GLFWWindow(const WindowConfig& config);
    ~GLFWWindow() override;

    GLFWWindow(const GLFWWindow&) = delete;
    GLFWWindow& operator=(const GLFWWindow&) = delete;

    bool valid() const;

    bool shouldClose() const override;
    void pollEvents() override;
    void waitEventsTimeout(double timeoutSeconds) override;
    void swapBuffers() override;
    void requestClose() override;

    int width() const override;
    int height() const override;
    int framebufferWidth() const override;
    int framebufferHeight() const override;
    float dpiScale() const override;

    void setClipboardText(const std::string& text) override;
    std::string clipboardText() const override;

    void setEventCallback(EventCallback callback) override;
    GLGetProcFn glGetProcAddress() const override;

private:
    void updateWindowMetrics();

    GLFWwindow* window_ = nullptr;
    EventCallback eventCallback_;
    int windowWidth_ = 0;
    int windowHeight_ = 0;
    int framebufferWidth_ = 0;
    int framebufferHeight_ = 0;
    float dpiScale_ = 1.0f;

    static void onFramebufferResize(GLFWwindow* w, int width, int height);
    static void onWindowClose(GLFWwindow* w);
    static void onWindowFocus(GLFWwindow* w, int focused);
    static void onKey(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void onMouseButton(GLFWwindow* w, int button, int action, int mods);
    static void onCursorPos(GLFWwindow* w, double x, double y);
    static void onCursorEnter(GLFWwindow* w, int entered);
    static void onScroll(GLFWwindow* w, double xoff, double yoff);
    static void onChar(GLFWwindow* w, unsigned int codepoint);
};

}  // namespace tinalux::platform
