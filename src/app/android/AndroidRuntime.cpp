#include "tinalux/app/android/AndroidRuntime.h"

#include <algorithm>
#include <utility>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/Log.h"
#include "tinalux/core/events/Event.h"

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
    std::string clipboardText;
    bool suspended = false;
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
    if (impl_->application.window() != nullptr) {
        impl_->application.window()->setClipboardText(impl_->clipboardText);
    }
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
    if (!impl_ || !ready() || impl_->suspended) {
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

bool AndroidRuntime::dispatchPointerMove(double x, double y)
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::MouseMoveEvent event(x, y);
    impl_->application.handleEvent(event);
    return true;
}

bool AndroidRuntime::dispatchPointerDown(double x, double y)
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::MouseMoveEvent moveEvent(x, y);
    impl_->application.handleEvent(moveEvent);

    core::MouseButtonEvent buttonEvent(
        core::mouse::kLeft,
        0,
        x,
        y,
        core::EventType::MouseButtonPress);
    impl_->application.handleEvent(buttonEvent);
    return true;
}

bool AndroidRuntime::dispatchPointerUp(double x, double y)
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::MouseMoveEvent moveEvent(x, y);
    impl_->application.handleEvent(moveEvent);

    core::MouseButtonEvent buttonEvent(
        core::mouse::kLeft,
        0,
        x,
        y,
        core::EventType::MouseButtonRelease);
    impl_->application.handleEvent(buttonEvent);
    return true;
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

bool AndroidRuntime::textInputActive() const
{
    return impl_ != nullptr
        && impl_->application.window() != nullptr
        && impl_->application.window()->textInputActive();
}

std::optional<core::Rect> AndroidRuntime::textInputCursorRect() const
{
    return impl_ != nullptr && impl_->application.window() != nullptr
        ? impl_->application.window()->textInputCursorRect()
        : std::nullopt;
}

bool AndroidRuntime::dispatchKeyDown(int key, int modifiers, bool repeat)
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::KeyEvent event(
        key,
        0,
        modifiers,
        repeat ? core::EventType::KeyRepeat : core::EventType::KeyPress);
    impl_->application.handleEvent(event);
    return true;
}

bool AndroidRuntime::dispatchKeyUp(int key, int modifiers)
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::KeyEvent event(key, 0, modifiers, core::EventType::KeyRelease);
    impl_->application.handleEvent(event);
    return true;
}

bool AndroidRuntime::dispatchTextInput(std::string text)
{
    if (!impl_ || !ready() || text.empty()) {
        return false;
    }

    core::TextInputEvent event(std::move(text));
    impl_->application.handleEvent(event);
    return true;
}

bool AndroidRuntime::dispatchCompositionStart()
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::TextCompositionEvent event(
        core::EventType::TextCompositionStart,
        {},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        true);
    impl_->application.handleEvent(event);
    return true;
}

bool AndroidRuntime::dispatchCompositionUpdate(
    std::string text,
    std::optional<std::size_t> caretUtf8Offset)
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::TextCompositionEvent event(
        core::EventType::TextCompositionUpdate,
        std::move(text),
        std::move(caretUtf8Offset),
        std::nullopt,
        std::nullopt,
        true);
    impl_->application.handleEvent(event);
    return true;
}

bool AndroidRuntime::dispatchCompositionEnd()
{
    if (!impl_ || !ready()) {
        return false;
    }

    core::TextCompositionEvent event(
        core::EventType::TextCompositionEnd,
        {},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        true);
    impl_->application.handleEvent(event);
    return true;
}

void AndroidRuntime::setClipboardText(std::string text)
{
    if (!impl_) {
        return;
    }

    impl_->clipboardText = std::move(text);
    if (impl_->application.window() != nullptr) {
        impl_->application.window()->setClipboardText(impl_->clipboardText);
    }
}

std::string AndroidRuntime::clipboardText() const
{
    if (!impl_) {
        return {};
    }

    return impl_->application.window() != nullptr
        ? impl_->application.window()->clipboardText()
        : impl_->clipboardText;
}

void AndroidRuntime::setSuspended(bool suspended)
{
    if (!impl_) {
        return;
    }

    if (impl_->suspended == suspended) {
        return;
    }

    impl_->suspended = suspended;
    core::logInfoCat(
        "app.android",
        "Android runtime lifecycle state changed to {}",
        suspended ? "suspended" : "resumed");
}

bool AndroidRuntime::suspended() const
{
    return impl_ != nullptr && impl_->suspended;
}

}  // namespace tinalux::app::android
