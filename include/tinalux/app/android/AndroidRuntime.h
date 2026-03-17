#pragma once

#include <memory>

#include "tinalux/app/Application.h"

namespace tinalux::app::android {

struct AndroidRuntimeConfig {
    ApplicationConfig application {};
};

class AndroidRuntime final {
public:
    AndroidRuntime();
    ~AndroidRuntime();

    AndroidRuntime(const AndroidRuntime&) = delete;
    AndroidRuntime& operator=(const AndroidRuntime&) = delete;

    void setConfig(AndroidRuntimeConfig config);
    const AndroidRuntimeConfig& config() const;

    bool attachWindow(void* nativeWindow, float dpiScale = 1.0f);
    void detachWindow();
    bool renderOnce();
    void shutdown();

    Application* application();
    const Application* application() const;
    bool ready() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace tinalux::app::android
