#include "tinalux/app/android/AndroidRuntime.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/Log.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/DemoScene.h"

namespace tinalux::app::android {

namespace {

float sanitizeAndroidDpiScale(float dpiScale)
{
    if (std::isfinite(dpiScale) && dpiScale > 0.0f) {
        return dpiScale;
    }

    core::logWarnCat(
        "app.android",
        "Android runtime received an invalid dpiScale ({:.3f}); falling back to 1.0",
        dpiScale);
    return 1.0f;
}

platform::WindowConfig makeWindowConfig(
    const platform::WindowConfig& baseConfig,
    void* nativeWindow,
    float dpiScale)
{
    platform::WindowConfig windowConfig = baseConfig;
    windowConfig.android = platform::WindowConfig::AndroidNativeWindowConfig {
        .nativeWindow = nativeWindow,
        .dpiScale = sanitizeAndroidDpiScale(dpiScale),
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
    void* attachedNativeWindow = nullptr;
    float attachedDpiScale = 1.0f;
    bool sessionActive = false;
    bool demoSceneInstalled = false;
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

void AndroidRuntime::setPreferredBackend(rendering::Backend backend)
{
    if (!impl_ || impl_->config.application.backend == backend) {
        return;
    }

    impl_->config.application.backend = backend;
    impl_->application.setRenderBackendPreference(backend);
    if (!impl_->sessionActive || impl_->attachedNativeWindow == nullptr) {
        return;
    }

    impl_->clipboardText = clipboardText();
    if (ready()) {
        impl_->application.suspendRendering();
    }
    if (!impl_->suspended) {
        platform::WindowConfig windowConfig = makeWindowConfig(
            impl_->config.application.window,
            impl_->attachedNativeWindow,
            impl_->attachedDpiScale);
        impl_->config.application.window = windowConfig;
        if (!impl_->application.resumeRendering(windowConfig)) {
            core::logErrorCat(
                "app.android",
                "Android runtime failed to switch backend to '{}'",
                rendering::backendName(backend));
            return;
        }
        impl_->application.setPlatformClipboardText(impl_->clipboardText);
    }
}

rendering::Backend AndroidRuntime::preferredBackend() const
{
    return impl_ != nullptr ? impl_->config.application.backend : rendering::Backend::OpenGL;
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

    impl_->attachedNativeWindow = nativeWindow;
    impl_->attachedDpiScale = sanitizeAndroidDpiScale(dpiScale);

    ApplicationConfig launchConfig = impl_->config.application;
    launchConfig.window = makeWindowConfig(
        launchConfig.window,
        impl_->attachedNativeWindow,
        impl_->attachedDpiScale);
    impl_->config.application.window = launchConfig.window;

    if (!impl_->sessionActive) {
        if (!impl_->application.init(launchConfig)) {
            core::logErrorCat(
                "app.android",
                "Android runtime failed to initialize the application");
            impl_->application.shutdown();
            return false;
        }
        impl_->sessionActive = true;
        impl_->demoSceneInstalled = false;
    } else if (!impl_->application.hasActiveRenderState()) {
        if (!impl_->application.resumeRendering(launchConfig.window)) {
            core::logErrorCat(
                "app.android",
                "Android runtime failed to resume rendering after attaching a window");
            return false;
        }
    }

    if (ready()) {
        impl_->application.setPlatformClipboardText(impl_->clipboardText);
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

    impl_->attachedNativeWindow = nullptr;
    impl_->clipboardText = clipboardText();

    if (ready()) {
        core::logInfoCat("app.android", "Android runtime detaching native window");
        impl_->application.suspendRendering();
    }
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
        impl_->sessionActive = false;
        impl_->demoSceneInstalled = false;
    }
    return keepRunning;
}

bool AndroidRuntime::installDemoScene()
{
    if (!impl_ || !ready()) {
        core::logErrorCat(
            "app.android",
            "InstallDemoScene requires an attached Android window");
        return false;
    }

    if (impl_->demoSceneInstalled) {
        return true;
    }

    const ui::Theme theme = ui::Theme::mobile();
    core::logInfoCat(
        "app.android",
        "Installing Android demo scene with theme background=#{:08x} primary=#{:08x}",
        static_cast<unsigned int>(theme.colors.background),
        static_cast<unsigned int>(theme.colors.primary));
    impl_->application.setTheme(theme);
    impl_->application.setRootWidget(impl_->application.buildWidgetTree([theme] {
        return ui::createDemoScene(theme);
    }));
    core::logInfoCat(
        "app.android",
        "Android demo scene installation finished");
    impl_->demoSceneInstalled = true;
    return true;
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
    if (!impl_) {
        return;
    }

    impl_->attachedNativeWindow = nullptr;
    impl_->sessionActive = false;
    impl_->demoSceneInstalled = false;
    impl_->suspended = false;
    impl_->application.shutdown();
}

bool AndroidRuntime::ready() const
{
    return impl_ != nullptr && impl_->sessionActive && impl_->application.hasActiveRenderState();
}

AndroidTextInputState AndroidRuntime::textInputState() const
{
    if (!impl_ || !ready()) {
        return {};
    }

    const bool active = impl_->application.platformTextInputActive();
    return AndroidTextInputState {
        .active = active,
        .cursorRect = active
            ? impl_->application.platformTextInputCursorRect()
            : std::nullopt,
    };
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
    if (ready()) {
        impl_->application.setPlatformClipboardText(impl_->clipboardText);
    }
}

std::string AndroidRuntime::clipboardText() const
{
    if (!impl_) {
        return {};
    }

    return ready()
        ? impl_->application.platformClipboardText()
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

    if (suspended) {
        impl_->clipboardText = clipboardText();
        if (ready()) {
            impl_->application.suspendRendering();
        }
        impl_->suspended = true;
    } else {
        if (impl_->sessionActive
            && !impl_->application.hasActiveRenderState()
            && impl_->attachedNativeWindow != nullptr) {
            platform::WindowConfig windowConfig = makeWindowConfig(
                impl_->config.application.window,
                impl_->attachedNativeWindow,
                impl_->attachedDpiScale);
            impl_->config.application.window = windowConfig;
            if (!impl_->application.resumeRendering(windowConfig)) {
                core::logErrorCat(
                    "app.android",
                    "Android runtime failed to resume rendering after suspend");
                impl_->suspended = true;
                return;
            }
            impl_->application.setPlatformClipboardText(impl_->clipboardText);
        }
        impl_->suspended = false;
    }

    core::logInfoCat(
        "app.android",
        "Android runtime lifecycle state changed to {}",
        suspended ? "suspended" : "resumed");
}

bool AndroidRuntime::suspended() const
{
    return impl_ != nullptr && impl_->suspended;
}

void AndroidRuntime::requestClose()
{
    if (impl_ != nullptr && ready()) {
        impl_->application.requestClose();
    }
}

}  // namespace tinalux::app::android
