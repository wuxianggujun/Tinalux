#include "tinalux/app/Application.h"

#include <chrono>
#include <string>

#include "UIContext.h"
#include "tinalux/core/Log.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Clipboard.h"

namespace tinalux::app {

struct Application::Impl {
    std::unique_ptr<platform::Window> window;
    rendering::RenderContext context;
    rendering::RenderSurface surface;
    UIContext uiContext;
    bool skiaInitialized = false;
    int surfaceWidth = 0;
    int surfaceHeight = 0;
};

Application::Application()
    : impl_(std::make_unique<Impl>())
{
}

Application::~Application()
{
    shutdown();
}

bool Application::init(const platform::WindowConfig& config)
{
    if (impl_->window != nullptr) {
        return true;
    }

    if (!core::isLogInitialized()) {
        core::initLog();
    }

    impl_->uiContext.initializeFromEnvironment();

    core::logInfoCat(
        "app",
        "Initializing application title='{}' size={}x{} vsync={}",
        config.title != nullptr ? config.title : "Tinalux",
        config.width,
        config.height,
        config.vsync);

    const PerfLogConfig perfConfig = impl_->uiContext.perfLogConfig();
    if (perfConfig.enabled) {
        core::logInfoCat(
            "perf",
            "Periodic performance summary enabled every {} rendered frames",
            perfConfig.frameInterval);
    }

    const DebugHudConfig hudConfig = impl_->uiContext.debugHudConfig();
    if (hudConfig.enabled) {
        core::logInfoCat(
            "app",
            "Debug HUD enabled (highlight_dirty_region={} scale={:.2f})",
            hudConfig.highlightDirtyRegion,
            hudConfig.scale);
    }

    impl_->window = platform::createWindow(config);
    if (!impl_->window) {
        core::logErrorCat("app", "Failed to create platform window");
        return false;
    }

    impl_->window->setEventCallback([this](core::Event& event) { handleEvent(event); });
    ui::setClipboardHandlers(
        [this] {
            return impl_ != nullptr && impl_->window != nullptr
                ? impl_->window->clipboardText()
                : std::string {};
        },
        [this](const std::string& text) {
            if (impl_ != nullptr && impl_->window != nullptr) {
                impl_->window->setClipboardText(text);
            }
        });

    rendering::initSkia();
    impl_->skiaInitialized = true;

    impl_->context = rendering::createGLContext(impl_->window->glGetProcAddress());
    if (!impl_->context) {
        core::logErrorCat("app", "Failed to create Skia GL context");
        shutdown();
        return false;
    }

    core::logInfoCat("app", "Application initialized successfully");
    return true;
}

int Application::run()
{
    if (!impl_ || impl_->window == nullptr || !impl_->context) {
        return 1;
    }

    while (!impl_->window->shouldClose()) {
        if (!impl_->uiContext.shouldRender()) {
            impl_->uiContext.noteWaitLoop();
            impl_->window->waitEventsTimeout(0.008);
        } else {
            impl_->uiContext.notePollLoop();
            impl_->window->pollEvents();
        }

        if (impl_->uiContext.tickAnimations(ui::animationNowSeconds())) {
            impl_->uiContext.noteAnimationTickUpdated();
        }
        if (impl_->uiContext.tickAsyncResources()) {
            impl_->uiContext.noteAnimationTickUpdated();
        }

        if (impl_->uiContext.shouldRender()) {
            using clock = std::chrono::steady_clock;
            const auto frameStart = clock::now();
            const bool fullRedraw = renderFrame();
            const auto frameEnd = clock::now();
            const double frameMs =
                std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

            impl_->uiContext.noteFrameRendered(fullRedraw, frameMs);
            impl_->uiContext.clearNeedsRedraw();
        } else {
            impl_->uiContext.noteFrameSkipped();
        }
    }

    return 0;
}

void Application::shutdown()
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.shutdown();

    impl_->surface = {};
    impl_->context = {};
    impl_->window.reset();
    impl_->surfaceWidth = 0;
    impl_->surfaceHeight = 0;

    if (impl_->skiaInitialized) {
        rendering::shutdownSkia();
        impl_->skiaInitialized = false;
    }
}

void Application::handleEvent(core::Event& event)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.handleEvent(event, [this] { requestClose(); });
}

void Application::setRootWidget(std::shared_ptr<ui::Widget> root)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setRootWidget(std::move(root));
}

void Application::setOverlayWidget(std::shared_ptr<ui::Widget> overlay)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setOverlayWidget(std::move(overlay));
}

void Application::clearOverlayWidget()
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.clearOverlayWidget();
}

platform::Window* Application::window() const
{
    return impl_ ? impl_->window.get() : nullptr;
}

ui::AnimationSink& Application::animationSink()
{
    return impl_->uiContext.animationSink();
}

void Application::requestClose()
{
    if (impl_ && impl_->window != nullptr) {
        core::logInfoCat("app", "Application requested close");
        impl_->window->requestClose();
    }
}

FrameStats Application::frameStats() const
{
    return impl_ ? impl_->uiContext.frameStats() : FrameStats {};
}

void Application::resetFrameStats()
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.resetFrameStats();
}

void Application::setTheme(ui::Theme theme)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setTheme(theme);
}

ui::Theme Application::theme() const
{
    return impl_ ? impl_->uiContext.theme() : ui::Theme::dark();
}

void Application::setPerfLogConfig(PerfLogConfig config)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setPerfLogConfig(config);
}

PerfLogConfig Application::perfLogConfig() const
{
    return impl_ ? impl_->uiContext.perfLogConfig() : PerfLogConfig {};
}

void Application::setDebugHudConfig(DebugHudConfig config)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setDebugHudConfig(config);
}

DebugHudConfig Application::debugHudConfig() const
{
    return impl_ ? impl_->uiContext.debugHudConfig() : DebugHudConfig {};
}

bool Application::renderFrame()
{
    if (!impl_ || impl_->window == nullptr || !impl_->context) {
        return true;
    }

    const int framebufferWidth = impl_->window->framebufferWidth();
    const int framebufferHeight = impl_->window->framebufferHeight();
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return true;
    }

    if (!impl_->surface
        || impl_->surfaceWidth != framebufferWidth
        || impl_->surfaceHeight != framebufferHeight) {
        core::logInfoCat(
            "app",
            "Recreating window surface for framebuffer {}x{}",
            framebufferWidth,
            framebufferHeight);
        impl_->surface = rendering::createWindowSurface(
            impl_->context,
            framebufferWidth,
            framebufferHeight);
        if (!impl_->surface) {
            impl_->surfaceWidth = 0;
            impl_->surfaceHeight = 0;
            return true;
        }
        impl_->surfaceWidth = framebufferWidth;
        impl_->surfaceHeight = framebufferHeight;
    }

    rendering::Canvas canvas = impl_->surface.canvas();
    const bool fullRedraw =
        impl_->uiContext.render(canvas, framebufferWidth, framebufferHeight);

    rendering::flushFrame(impl_->context);
    impl_->window->swapBuffers();
    return fullRedraw;
}

}  // namespace tinalux::app
