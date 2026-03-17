#pragma once

#include <string>

#include "tinalux/platform/Window.h"

struct ANativeWindow;

typedef void* EGLDisplay;
typedef void* EGLSurface;
typedef void* EGLContext;
typedef void* EGLConfig;

namespace tinalux::platform {

class AndroidWindow final : public Window {
public:
    explicit AndroidWindow(const WindowConfig& config);
    ~AndroidWindow() override;

    AndroidWindow(const AndroidWindow&) = delete;
    AndroidWindow& operator=(const AndroidWindow&) = delete;

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
    void setTextInputActive(bool active) override;
    bool textInputActive() const override;
    void setTextInputCursorRect(const std::optional<core::Rect>& rect) override;
    std::optional<core::Rect> textInputCursorRect() const override;
    void setEventCallback(EventCallback callback) override;
    GLGetProcFn glGetProcAddress() const override;

private:
    bool initialize(const WindowConfig& config);
    bool initializeEgl(const WindowConfig& config);
    void destroyEgl();
    void refreshWindowMetrics();

    static void* eglProcAddress(const char* name);

    ANativeWindow* nativeWindow_ = nullptr;
    EventCallback eventCallback_;
    std::string clipboardText_;
    int fallbackWidth_ = 1;
    int fallbackHeight_ = 1;
    int windowWidth_ = 0;
    int windowHeight_ = 0;
    float dpiScale_ = 1.0f;
    GraphicsAPI graphicsApi_ = GraphicsAPI::OpenGL;
    bool shouldClose_ = false;
    bool textInputActive_ = false;
    std::optional<core::Rect> imeCursorRect_;

    EGLDisplay display_ = nullptr;
    EGLSurface surface_ = nullptr;
    EGLContext context_ = nullptr;
    EGLConfig config_ = nullptr;
    bool eglReady_ = false;
};

}  // namespace tinalux::platform
