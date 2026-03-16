#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "tinalux/platform/Window.h"

struct GLFWwindow;

#if defined(_WIN32)
#include <windows.h>
#endif

namespace tinalux::platform {

#if defined(__APPLE__)
class CocoaTextInputBridge;
class CocoaMetalLayerBridge;
#endif

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
    void setTextInputActive(bool active) override;
    void setTextInputCursorRect(const std::optional<core::Rect>& rect) override;

    void setEventCallback(EventCallback callback) override;
    GLGetProcFn glGetProcAddress() const override;
    bool vulkanSupported() const override;
    std::vector<std::string> requiredVulkanInstanceExtensions() const override;
    VulkanGetInstanceProcFn vulkanGetInstanceProcAddress() const override;
    bool vulkanPresentationSupported(void* instance, void* physicalDevice, uint32_t queueFamilyIndex) const override;
    void* createVulkanWindowSurface(void* instance) const override;
    void destroyVulkanWindowSurface(void* instance, void* surface) const override;
    bool prepareMetalLayer(
        void* device,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale) override;
    void* metalLayer() const override;
    void dispatchPlatformCompositionStart();
    void dispatchPlatformCompositionUpdate(
        const std::string& text,
        std::optional<std::size_t> caretUtf8Offset,
        std::optional<std::size_t> targetStartUtf8,
        std::optional<std::size_t> targetEndUtf8);
    void dispatchPlatformCompositionEnd();

private:
    void updateWindowMetrics();

    GLFWwindow* window_ = nullptr;
    EventCallback eventCallback_;
    int windowWidth_ = 0;
    int windowHeight_ = 0;
    int framebufferWidth_ = 0;
    int framebufferHeight_ = 0;
    float dpiScale_ = 1.0f;
    GraphicsAPI graphicsApi_ = GraphicsAPI::OpenGL;
    bool vsync_ = true;
    bool textInputActive_ = false;
    std::optional<core::Rect> imeCursorRect_;

    static void onFramebufferResize(GLFWwindow* w, int width, int height);
    static void onWindowClose(GLFWwindow* w);
    static void onWindowFocus(GLFWwindow* w, int focused);
    static void onKey(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void onMouseButton(GLFWwindow* w, int button, int action, int mods);
    static void onCursorPos(GLFWwindow* w, double x, double y);
    static void onCursorEnter(GLFWwindow* w, int entered);
    static void onScroll(GLFWwindow* w, double xoff, double yoff);
    static void onChar(GLFWwindow* w, unsigned int codepoint);
#if defined(__linux__)
    static void onX11TextInput(
        GLFWwindow* w,
        int event,
        const char* text,
        int caret,
        int targetStart,
        int targetEnd,
        void* userPointer);
#endif
#if defined(_WIN32)
    static LRESULT CALLBACK onNativeWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND nativeWindow_ = nullptr;
    WNDPROC originalWindowProc_ = nullptr;
    int suppressedImeCharCount_ = 0;
#endif
#if defined(__APPLE__)
    std::unique_ptr<CocoaTextInputBridge> cocoaTextInputBridge_;
    std::unique_ptr<CocoaMetalLayerBridge> cocoaMetalLayerBridge_;
#endif
};

}  // namespace tinalux::platform
