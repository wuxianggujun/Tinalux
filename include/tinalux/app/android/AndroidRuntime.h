#pragma once

#include <memory>
#include <optional>
#include <string>

#include "tinalux/app/Application.h"
#include "tinalux/core/Geometry.h"

namespace tinalux::app::android {

struct AndroidRuntimeConfig {
    ApplicationConfig application {};

    AndroidRuntimeConfig()
    {
        application.backend = rendering::Backend::OpenGL;
    }
};

struct AndroidTextInputState {
    bool active = false;
    std::optional<core::Rect> cursorRect;
};

class AndroidRuntime final {
public:
    AndroidRuntime();
    ~AndroidRuntime();

    AndroidRuntime(const AndroidRuntime&) = delete;
    AndroidRuntime& operator=(const AndroidRuntime&) = delete;

    void setConfig(AndroidRuntimeConfig config);
    const AndroidRuntimeConfig& config() const;
    void setPreferredBackend(rendering::Backend backend);

    bool attachWindow(void* nativeWindow, float dpiScale = 1.0f);
    void detachWindow();
    bool renderOnce();
    bool installDemoScene();
    bool dispatchPointerMove(double x, double y);
    bool dispatchPointerDown(double x, double y);
    bool dispatchPointerUp(double x, double y);
    AndroidTextInputState textInputState() const;
    bool dispatchKeyDown(int key, int modifiers = 0, bool repeat = false);
    bool dispatchKeyUp(int key, int modifiers = 0);
    bool dispatchTextInput(std::string text);
    bool dispatchCompositionUpdate(std::string text, std::optional<std::size_t> caretUtf8Offset);
    bool dispatchCompositionEnd();
    void setClipboardText(std::string text);
    std::string clipboardText() const;
    void setSuspended(bool suspended);
    void requestClose();

    bool ready() const;

private:
    void shutdown();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace tinalux::app::android
