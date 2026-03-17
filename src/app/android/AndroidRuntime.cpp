#include "tinalux/app/android/AndroidRuntime.h"

#include <algorithm>
#include <utility>

#include "tinalux/core/Log.h"

namespace tinalux::app::android {

namespace {

platform::WindowConfig makeWindowConfig(
    const platform::WindowConfig& baseConfig,
    void* nativeWindow,
    float dpiScale)
{
    platform::WindowConfig windowConfig = baseConfig;
    windowConfig.android = platform::WindowConfig::AndroidNativeWindowConfig {
        .nativeWindow = nativeWindow,
        .dpiScale = std::max(dpiScale, 0.1f),
    };
    if (windowConfig.title == nullptr) {
        windowConfig.title = "Tinalux Android";
    }
    return windowConfig;
}

}  // namespace

struct AndroidRuntime::Impl {
    AndroidRuntimeConfig config;
    Application application;
};

AndroidRuntime::AndroidRuntime()
    : impl_(std::make_unique<Impl>())
{
}

AndroidRuntime::~AndroidRuntime()
{
    shutdown();
}

void AndroidRuntime::setConfig(AndroidRuntimeConfig config)
{
    if (!impl_) {
        return;
    }

    if (ready()) {
        core::logInfoCat(
            "app.android",
            "Android runtime config changed while active; restarting application session");
        detachWindow();
    }

    impl_->config = std::move(config);
}

const AndroidRuntimeConfig& AndroidRuntime::config() const
{
    return impl_->config;
}

bool AndroidRuntime::attachWindow(void* nativeWindow, float dpiScale)
{
    if (!impl_) {
        return false;
    }

    if (nativeWindow == nullptr) {
        core::logErrorCat(
            "app.android",
            "Android runtime attach failed because nativeWindow is null");
        return false;
    }

    if (ready()) {
        core::logInfoCat(
            "app.android",
            "Android runtime received a new window; recycling the previous application session");
        detachWindow();
    }

    ApplicationConfig launchConfig = impl_->config.application;
    launchConfig.window = makeWindowConfig(launchConfig.window, nativeWindow, dpiScale);
    if (!impl_->application.init(launchConfig)) {
        core::logErrorCat(
            "app.android",
            "Android runtime failed to initialize the application");
        impl_->application.shutdown();
        return false;
    }

    core::logInfoCat(
        "app.android",
        "Android runtime attached a native window and initialized backend '{}'",
        rendering::backendName(impl_->application.renderBackend()));
    return true;
}

void AndroidRuntime::detachWindow()
{
    if (!impl_) {
        return;
    }

    if (!ready()) {
        return;
    }

    core::logInfoCat("app.android", "Android runtime detaching native window");
    impl_->application.shutdown();
}

bool AndroidRuntime::renderOnce()
{
    if (!impl_ || !ready()) {
        return false;
    }

    const bool keepRunning = impl_->application.pumpOnce();
    if (!keepRunning) {
        core::logInfoCat(
            "app.android",
            "Android runtime stopped because the application requested close");
        impl_->application.shutdown();
    }
    return keepRunning;
}

void AndroidRuntime::shutdown()
{
    detachWindow();
}

Application* AndroidRuntime::application()
{
    return ready() ? &impl_->application : nullptr;
}

const Application* AndroidRuntime::application() const
{
    return ready() ? &impl_->application : nullptr;
}

bool AndroidRuntime::ready() const
{
    return impl_ != nullptr && impl_->application.window() != nullptr;
}

}  // namespace tinalux::app::android
