#include "tinalux/app/Application.h"

#include <cmath>
#include <chrono>
#include <string>

#include "BackendPlan.h"
#include "LoopTiming.h"
#include "RuntimeHooks.h"
#include "UIContext.h"
#include "tinalux/core/Log.h"
#include "tinalux/core/events/Event.h"
#include "../rendering/FrameLifecycle.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/ResourceManager.h"

namespace tinalux::app {

namespace {

constexpr double kIdleWaitSeconds = 0.05;
constexpr auto kInteractiveSurfaceRecreateMinInterval = std::chrono::milliseconds(16);
constexpr auto kInteractiveSurfaceLossRetryWindow = std::chrono::milliseconds(32);
constexpr auto kRepeatedSurfaceFailureLogInterval = std::chrono::milliseconds(250);
constexpr std::size_t kPersistentRuntimeSurfaceFailureFallbackThreshold = 2;

enum class RenderFrameOutcome {
    Deferred,
    Presented,
};

enum class SurfaceFailureLogStage {
    RetryLater,
    SurfaceLost,
    CanvasUnavailable,
};

struct SurfaceFailureLogState {
    rendering::Backend backend = rendering::Backend::Auto;
    rendering::SurfaceFailureReason reason = rendering::SurfaceFailureReason::None;
    SurfaceFailureLogStage stage = SurfaceFailureLogStage::RetryLater;
    std::chrono::steady_clock::time_point lastLoggedAt {};
    std::size_t suppressedCount = 0;
};

struct RuntimeSurfaceFailureRecoveryState {
    rendering::Backend backend = rendering::Backend::Auto;
    std::size_t consecutiveFallbackEligibleFailures = 0;
};

float sanitizeDpiScale(float dpiScale)
{
    return std::isfinite(dpiScale) && dpiScale > 0.0f ? dpiScale : 1.0f;
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

std::chrono::steady_clock::time_point nowSteadyTime()
{
    const auto nowFn = detail::runtimeHooks().nowSteadyTime;
    return nowFn != nullptr ? nowFn() : std::chrono::steady_clock::now();
}

platform::WindowMetrics currentWindowMetrics(const platform::Window* window)
{
    return window != nullptr ? window->metrics() : platform::WindowMetrics {};
}

bool windowMetricsChanged(
    const platform::WindowMetrics& previous,
    const platform::WindowMetrics& current)
{
    return previous.windowWidth != current.windowWidth
        || previous.windowHeight != current.windowHeight
        || previous.framebufferWidth != current.framebufferWidth
        || previous.framebufferHeight != current.framebufferHeight
        || !nearlyEqual(
            sanitizeDpiScale(previous.dpiScale),
            sanitizeDpiScale(current.dpiScale));
}

bool framebufferSizeChanged(
    const platform::WindowMetrics& previous,
    const platform::WindowMetrics& current)
{
    return previous.framebufferWidth != current.framebufferWidth
        || previous.framebufferHeight != current.framebufferHeight;
}

bool shouldCoalesceInteractiveSurfaceRecreate(rendering::Backend backend)
{
    return backend == rendering::Backend::Vulkan || backend == rendering::Backend::Metal;
}

bool hasRecentInteractiveMetricsChange(
    const std::chrono::steady_clock::time_point& lastMetricsChangeAt,
    const std::chrono::steady_clock::time_point& now)
{
    return lastMetricsChangeAt != std::chrono::steady_clock::time_point {}
        && now - lastMetricsChangeAt < kInteractiveSurfaceLossRetryWindow;
}

bool isFallbackEligibleRuntimeSurfaceFailureReason(rendering::SurfaceFailureReason reason)
{
    switch (reason) {
    case rendering::SurfaceFailureReason::None:
    case rendering::SurfaceFailureReason::ZeroFramebufferSize:
    case rendering::SurfaceFailureReason::VulkanAcquireOutOfDate:
    case rendering::SurfaceFailureReason::VulkanPresentOutOfDate:
    case rendering::SurfaceFailureReason::MetalDrawableUnavailable:
        return false;
    default:
        return true;
    }
}

bool shouldPromoteFallbackAfterRuntimeSurfaceFailure(
    RuntimeSurfaceFailureRecoveryState& state,
    bool hasFallback,
    rendering::Backend backend,
    rendering::SurfaceFailureReason reason,
    bool recentInteractiveMetricsChange)
{
    if (!hasFallback
        || recentInteractiveMetricsChange
        || !isFallbackEligibleRuntimeSurfaceFailureReason(reason)) {
        state = {};
        return false;
    }

    if (state.backend != backend) {
        state.backend = backend;
        state.consecutiveFallbackEligibleFailures = 1;
        return false;
    }

    ++state.consecutiveFallbackEligibleFailures;
    return state.consecutiveFallbackEligibleFailures
        >= kPersistentRuntimeSurfaceFailureFallbackThreshold;
}

bool shouldSuppressRepeatedSurfaceFailureLog(
    SurfaceFailureLogState& state,
    rendering::Backend backend,
    rendering::SurfaceFailureReason reason,
    SurfaceFailureLogStage stage,
    const std::chrono::steady_clock::time_point& now)
{
    const bool sameFailure = state.backend == backend
        && state.reason == reason
        && state.stage == stage
        && state.lastLoggedAt != std::chrono::steady_clock::time_point {};
    if (!sameFailure) {
        state.suppressedCount = 0;
        return false;
    }
    if (now - state.lastLoggedAt >= kRepeatedSurfaceFailureLogInterval) {
        return false;
    }

    ++state.suppressedCount;
    return true;
}

void logSurfaceFailureEvent(
    SurfaceFailureLogState& state,
    rendering::Backend backend,
    rendering::SurfaceFailureReason reason,
    SurfaceFailureLogStage stage)
{
    const auto now = nowSteadyTime();
    if (shouldSuppressRepeatedSurfaceFailureLog(state, backend, reason, stage, now)) {
        return;
    }

    const std::size_t suppressedCount = state.backend == backend
            && state.reason == reason
            && state.stage == stage
        ? state.suppressedCount
        : 0;
    state.backend = backend;
    state.reason = reason;
    state.stage = stage;
    state.lastLoggedAt = now;
    state.suppressedCount = 0;

    const std::string repeatedSuffix = suppressedCount > 0
        ? std::string(", repeated=") + std::to_string(suppressedCount)
        : std::string {};
    if (stage == SurfaceFailureLogStage::RetryLater) {
        core::logDebugCat(
            "app",
            "Skipping frame because backend '{}' is temporarily unavailable (reason='{}'{})",
            rendering::backendName(backend),
            rendering::surfaceFailureReasonName(reason),
            repeatedSuffix);
        return;
    }

    if (stage == SurfaceFailureLogStage::SurfaceLost) {
        core::logWarnCat(
            "app",
            "Render surface became unavailable during frame preparation for backend '{}' (reason='{}'{})",
            rendering::backendName(backend),
            rendering::surfaceFailureReasonName(reason),
            repeatedSuffix);
        return;
    }

    if (stage == SurfaceFailureLogStage::CanvasUnavailable) {
        core::logWarnCat(
            "app",
            "Frame preparation reported ready, but backend '{}' returned an empty canvas (reason='{}'{})",
            rendering::backendName(backend),
            rendering::surfaceFailureReasonName(reason),
            repeatedSuffix);
        return;
    }

    core::logWarnCat(
        "app",
        "Backend '{}' reported surface failure reason='{}'{}",
        rendering::backendName(backend),
        rendering::surfaceFailureReasonName(reason),
        repeatedSuffix);
}

core::Rect scaleRect(const core::Rect& rect, float scale)
{
    return core::Rect::MakeLTRB(
        rect.left() * scale,
        rect.top() * scale,
        rect.right() * scale,
        rect.bottom() * scale);
}

std::optional<core::Rect> logicalToPhysicalRect(
    const std::optional<core::Rect>& rect,
    float dpiScale)
{
    if (!rect.has_value()) {
        return std::nullopt;
    }
    return scaleRect(*rect, sanitizeDpiScale(dpiScale));
}

void syncResourceManagerDevicePixelRatio(
    platform::Window* window,
    const platform::WindowMetrics& metrics,
    ui::DevicePixelRatioBindingId& bindingId,
    float& syncedDevicePixelRatio)
{
    const float targetRatio =
        window != nullptr ? sanitizeDpiScale(metrics.dpiScale) : 1.0f;
    const bool bindingStateMatches = window != nullptr ? bindingId != 0 : bindingId == 0;
    if (bindingStateMatches && nearlyEqual(syncedDevicePixelRatio, targetRatio)) {
        return;
    }

    ui::ResourceManager& resourceManager = ui::ResourceManager::instance();
    if (window == nullptr) {
        resourceManager.detachDevicePixelRatio(bindingId);
        bindingId = 0;
    } else if (bindingId == 0) {
        bindingId = resourceManager.attachDevicePixelRatio(targetRatio);
    } else {
        resourceManager.updateDevicePixelRatio(bindingId, targetRatio);
    }
    syncedDevicePixelRatio = targetRatio;
}

}  // namespace

struct Application::Impl {
    ApplicationConfig config;
    detail::BackendPlan backendPlan;
    std::unique_ptr<platform::Window> window;
    rendering::RenderContext context;
    rendering::RenderSurface surface;
    UIContext uiContext;
    ui::ClipboardBindingId clipboardBindingId = 0;
    ui::DevicePixelRatioBindingId resourceManagerBindingId = 0;
    bool skiaInitialized = false;
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    platform::WindowMetrics lastObservedWindowMetrics {};
    std::chrono::steady_clock::time_point lastWindowMetricsChangeAt {};
    bool pendingSurfaceRecreate = false;
    std::chrono::steady_clock::time_point lastSurfaceRecreateAt {};
    SurfaceFailureLogState surfaceFailureLogState {};
    RuntimeSurfaceFailureRecoveryState runtimeSurfaceFailureRecoveryState {};
    float syncedDevicePixelRatio = 0.0f;
    RenderFrameOutcome lastRenderFrameOutcome = RenderFrameOutcome::Deferred;
};

Application::Application()
    : impl_(std::make_unique<Impl>())
{
    // Window-backed swapchains do not guarantee preserved pixels across swaps,
    // so live application rendering must redraw the full scene.
    impl_->uiContext.setPartialRedrawEnabled(false);
}

Application::~Application()
{
    shutdown();
}

bool Application::init(const ApplicationConfig& config)
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
        "Initializing application title='{}' size={}x{} vsync={} backend={}",
        config.window.title != nullptr ? config.window.title : "Tinalux",
        config.window.width,
        config.window.height,
        config.window.vsync,
        rendering::backendName(config.backend));

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

    rendering::initSkia();
    impl_->skiaInitialized = true;
    impl_->config = config;
    impl_->backendPlan.reset(config.backend);
    impl_->clipboardBindingId = ui::attachClipboardHandlers(
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

    const auto& candidates = impl_->backendPlan.candidates();
    for (std::size_t backendIndex = 0; backendIndex < candidates.size(); ++backendIndex) {
        const rendering::Backend candidateBackend = candidates[backendIndex];
        core::logInfoCat(
            "app",
            "Trying render backend '{}' for application startup",
            rendering::backendName(candidateBackend));
        if (tryInitializeBackend(config.window, candidateBackend, backendIndex)) {
            core::logInfoCat(
                "app",
                "Application initialized successfully with backend '{}'",
                rendering::backendName(impl_->context.backend()));
            syncTextInputState();
            return true;
        }
    }

    core::logErrorCat(
        "app",
        "Failed to initialize application for requested backend '{}'",
        rendering::backendName(config.backend));
    shutdown();
    return false;
}

void Application::suspendRendering()
{
    if (!impl_ || impl_->window == nullptr) {
        return;
    }

    core::logInfoCat(
        "app",
        "Suspending rendering state for backend '{}'",
        rendering::backendName(impl_->context.backend()));
    resetRenderState();
}

bool Application::resumeRendering(const platform::WindowConfig& windowConfig)
{
    if (!impl_ || !impl_->skiaInitialized) {
        return false;
    }
    if (impl_->window != nullptr && impl_->context) {
        return true;
    }

    impl_->config.window = windowConfig;
    impl_->backendPlan.reset(impl_->config.backend);

    const auto& candidates = impl_->backendPlan.candidates();
    for (std::size_t backendIndex = 0; backendIndex < candidates.size(); ++backendIndex) {
        const rendering::Backend candidateBackend = candidates[backendIndex];
        core::logInfoCat(
            "app",
            "Resuming rendering with backend '{}'",
            rendering::backendName(candidateBackend));
        if (tryInitializeBackend(windowConfig, candidateBackend, backendIndex)) {
            syncTextInputState();
            return true;
        }
    }

    core::logErrorCat(
        "app",
        "Failed to resume rendering for requested backend '{}'",
        rendering::backendName(impl_->config.backend));
    return false;
}

bool Application::renderingReady() const
{
    return impl_ != nullptr && impl_->window != nullptr && impl_->context;
}

bool Application::tryInitializeBackend(
    const platform::WindowConfig& windowConfig,
    rendering::Backend backend,
    std::size_t backendIndex)
{
    if (!impl_) {
        return false;
    }

    platform::WindowConfig candidateWindowConfig = windowConfig;
    candidateWindowConfig.graphicsApi = detail::graphicsApiForBackend(backend);

    std::unique_ptr<platform::Window> candidateWindow =
        detail::runtimeHooks().createWindow(candidateWindowConfig);
    if (!candidateWindow) {
        core::logWarnCat(
            "app",
            "Failed to create platform window for backend '{}'",
            rendering::backendName(backend));
        return false;
    }

    candidateWindow->setEventCallback([this](core::Event& event) { handleEvent(event); });

    if (backend == rendering::Backend::Vulkan && !candidateWindow->vulkanSupported()) {
        core::logWarnCat(
            "app",
            "Skipping backend '{}' because platform window reports Vulkan unsupported",
            rendering::backendName(backend));
        return false;
    }

    rendering::ContextConfig contextConfig {
        .backend = backend,
        .openGL = {
            .getProc = candidateWindow->glGetProcAddress(),
        },
        .vulkan = {
            .getInstanceProc = candidateWindow->vulkanGetInstanceProcAddress(),
            .instanceExtensions = candidateWindow->requiredVulkanInstanceExtensions(),
        },
    };
    rendering::RenderContext candidateContext =
        detail::runtimeHooks().createContext(contextConfig);
    if (!candidateContext) {
        core::logWarnCat(
            "app",
            "Backend '{}' failed to create a render context",
            rendering::backendName(backend));
        return false;
    }

    rendering::RenderSurface candidateSurface;
    int candidateSurfaceWidth = 0;
    int candidateSurfaceHeight = 0;
    const platform::WindowMetrics candidateMetrics = candidateWindow->metrics();
    if (candidateMetrics.framebufferWidth > 0
        && candidateMetrics.framebufferHeight > 0) {
        candidateSurface = detail::runtimeHooks().createWindowSurface(
            candidateContext,
            *candidateWindow);
        if (!candidateSurface) {
            core::logWarnCat(
                "app",
                "Backend '{}' failed to create an initial window surface",
                rendering::backendName(backend));
            return false;
        }
        candidateSurfaceWidth = candidateMetrics.framebufferWidth;
        candidateSurfaceHeight = candidateMetrics.framebufferHeight;
    }

    impl_->surface = std::move(candidateSurface);
    impl_->context = std::move(candidateContext);
    impl_->window = std::move(candidateWindow);
    impl_->surfaceWidth = candidateSurfaceWidth;
    impl_->surfaceHeight = candidateSurfaceHeight;
    impl_->lastObservedWindowMetrics = candidateMetrics;
    impl_->lastWindowMetricsChangeAt = {};
    impl_->pendingSurfaceRecreate = false;
    impl_->lastSurfaceRecreateAt =
        impl_->surface ? nowSteadyTime() : std::chrono::steady_clock::time_point {};
    impl_->surfaceFailureLogState = {};
    impl_->runtimeSurfaceFailureRecoveryState = {};
    impl_->backendPlan.activate(backendIndex);
    syncResourceManagerDevicePixelRatio(
        impl_->window.get(),
        candidateMetrics,
        impl_->resourceManagerBindingId,
        impl_->syncedDevicePixelRatio);
    return true;
}

bool Application::tryPromoteNextBackend()
{
    if (!impl_ || !impl_->backendPlan.hasFallback()) {
        return false;
    }

    const rendering::Backend currentBackend =
        impl_->context ? impl_->context.backend() : rendering::Backend::Auto;
    const platform::WindowConfig windowConfig = currentWindowConfigForRecovery();
    const auto& candidates = impl_->backendPlan.candidates();
    for (std::size_t backendIndex = impl_->backendPlan.nextFallbackIndex();
         backendIndex < candidates.size();
         ++backendIndex) {
        const rendering::Backend candidateBackend = candidates[backendIndex];
        core::logWarnCat(
            "app",
            "Attempting runtime backend fallback from '{}' to '{}'",
            rendering::backendName(currentBackend),
            rendering::backendName(candidateBackend));
        if (tryInitializeBackend(windowConfig, candidateBackend, backendIndex)) {
            core::logInfoCat(
                "app",
                "Runtime backend fallback succeeded: '{}'",
                rendering::backendName(candidateBackend));
            syncTextInputState();
            return true;
        }
    }

    core::logErrorCat(
        "app",
        "Runtime backend fallback failed, staying on '{}'",
        rendering::backendName(currentBackend));
    return false;
}

platform::WindowConfig Application::currentWindowConfigForRecovery() const
{
    platform::WindowConfig windowConfig = impl_ ? impl_->config.window : platform::WindowConfig {};
    if (impl_ == nullptr || impl_->window == nullptr) {
        return windowConfig;
    }

    const platform::WindowMetrics metrics = currentWindowMetrics(impl_->window.get());
    if (metrics.windowWidth > 0) {
        windowConfig.width = metrics.windowWidth;
    }
    if (metrics.windowHeight > 0) {
        windowConfig.height = metrics.windowHeight;
    }
    return windowConfig;
}

void Application::resetRenderState()
{
    if (!impl_) {
        return;
    }

    impl_->surface = {};
    impl_->context = {};
    impl_->window.reset();
    impl_->surfaceWidth = 0;
    impl_->surfaceHeight = 0;
    impl_->lastObservedWindowMetrics = {};
    impl_->lastWindowMetricsChangeAt = {};
    impl_->pendingSurfaceRecreate = false;
    impl_->lastSurfaceRecreateAt = {};
    impl_->surfaceFailureLogState = {};
    impl_->runtimeSurfaceFailureRecoveryState = {};
    impl_->backendPlan.reset(impl_->config.backend);
    syncResourceManagerDevicePixelRatio(
        nullptr,
        {},
        impl_->resourceManagerBindingId,
        impl_->syncedDevicePixelRatio);
    impl_->lastRenderFrameOutcome = RenderFrameOutcome::Deferred;
}

bool Application::pumpOnce()
{
    if (!impl_ || impl_->window == nullptr || !impl_->context) {
        return false;
    }

    if (impl_->window->shouldClose()) {
        return false;
    }

    const double nowSeconds = ui::animationNowSeconds();
    const detail::EventLoopDecision loopDecision = detail::chooseEventLoopDecision(
        impl_->uiContext.hasImmediateRenderWork(),
        impl_->uiContext.nextAnimationDelaySeconds(nowSeconds),
        kIdleWaitSeconds);

    if (loopDecision.mode == detail::EventLoopMode::Poll) {
        impl_->uiContext.notePollLoop();
        impl_->window->pollEvents();
    } else {
        impl_->uiContext.noteWaitLoop();
        impl_->window->waitEventsTimeout(loopDecision.waitSeconds);
    }
    const platform::WindowMetrics previousMetrics = impl_->lastObservedWindowMetrics;
    const platform::WindowMetrics currentMetrics = currentWindowMetrics(impl_->window.get());
    if (windowMetricsChanged(previousMetrics, currentMetrics)) {
        const auto metricsChangedAt = nowSteadyTime();
        if (framebufferSizeChanged(previousMetrics, currentMetrics) && impl_->surface) {
            impl_->pendingSurfaceRecreate = true;
        }
        impl_->lastWindowMetricsChangeAt = metricsChangedAt;
        impl_->uiContext.notifyWindowMetricsChanged();
        impl_->lastObservedWindowMetrics = currentMetrics;
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

        if (impl_->lastRenderFrameOutcome == RenderFrameOutcome::Presented) {
            impl_->uiContext.noteFrameRendered(fullRedraw, frameMs);
            impl_->uiContext.clearNeedsRedraw();
        } else {
            impl_->uiContext.noteFrameDeferred();
        }
    }

    return !impl_->window->shouldClose();
}

int Application::run()
{
    if (!impl_ || impl_->window == nullptr || !impl_->context) {
        return 1;
    }

    while (pumpOnce()) {
    }

    return 0;
}

void Application::shutdown()
{
    if (!impl_) {
        return;
    }

    ui::detachClipboardHandlers(impl_->clipboardBindingId);
    impl_->clipboardBindingId = 0;
    impl_->uiContext.shutdown();
    resetRenderState();
    impl_->backendPlan.reset(rendering::Backend::Auto);
    impl_->config = ApplicationConfig {};

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

    const float dpiScale = impl_->window != nullptr
        ? sanitizeDpiScale(impl_->window->metrics().dpiScale)
        : 1.0f;
    syncResourceManagerDevicePixelRatio(
        impl_->window.get(),
        currentWindowMetrics(impl_->window.get()),
        impl_->resourceManagerBindingId,
        impl_->syncedDevicePixelRatio);
    impl_->uiContext.handleEvent(event, [this] { requestClose(); }, dpiScale);
    syncTextInputState();
}

std::shared_ptr<ui::Widget> Application::buildWidgetTree(
    const std::function<std::shared_ptr<ui::Widget>()>& builder)
{
    if (!impl_) {
        return {};
    }

    return impl_->uiContext.buildWidgetTree(builder);
}

void Application::setRootWidget(std::shared_ptr<ui::Widget> root)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setRootWidget(std::move(root));
    syncTextInputState();
}

void Application::setOverlayWidget(std::shared_ptr<ui::Widget> overlay)
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.setOverlayWidget(std::move(overlay));
    syncTextInputState();
}

void Application::clearOverlayWidget()
{
    if (!impl_) {
        return;
    }

    impl_->uiContext.clearOverlayWidget();
    syncTextInputState();
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

rendering::Backend Application::renderBackend() const
{
    return (impl_ && impl_->context) ? impl_->context.backend() : rendering::Backend::Auto;
}

void Application::setPreferredRenderBackend(rendering::Backend backend)
{
    if (!impl_ || impl_->config.backend == backend) {
        return;
    }

    impl_->config.backend = backend;
    impl_->backendPlan.reset(backend);
}

rendering::Backend Application::preferredRenderBackend() const
{
    return impl_ ? impl_->config.backend : rendering::Backend::Auto;
}

bool Application::renderFrame()
{
    if (!impl_ || impl_->window == nullptr || !impl_->context) {
        return true;
    }

    impl_->lastRenderFrameOutcome = RenderFrameOutcome::Deferred;

    const platform::WindowMetrics metrics = currentWindowMetrics(impl_->window.get());
    const int framebufferWidth = metrics.framebufferWidth;
    const int framebufferHeight = metrics.framebufferHeight;
    const float dpiScale = sanitizeDpiScale(metrics.dpiScale);
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        impl_->runtimeSurfaceFailureRecoveryState = {};
        return true;
    }

    const auto handleRuntimeSurfaceFailure = [this](rendering::SurfaceFailureReason failureReason) {
        impl_->uiContext.requestRedraw();
        impl_->surface = {};
        impl_->surfaceWidth = 0;
        impl_->surfaceHeight = 0;

        const auto now = nowSteadyTime();
        const bool recentInteractiveMetricsChange =
            shouldCoalesceInteractiveSurfaceRecreate(impl_->context.backend())
            && hasRecentInteractiveMetricsChange(
                impl_->lastWindowMetricsChangeAt,
                now);
        impl_->pendingSurfaceRecreate = recentInteractiveMetricsChange;

        if (shouldPromoteFallbackAfterRuntimeSurfaceFailure(
                impl_->runtimeSurfaceFailureRecoveryState,
                impl_->backendPlan.hasFallback(),
                impl_->context.backend(),
                failureReason,
                recentInteractiveMetricsChange)) {
            core::logWarnCat(
                "app",
                "Promoting fallback after {} consecutive runtime surface failures on backend '{}' (reason='{}')",
                impl_->runtimeSurfaceFailureRecoveryState.consecutiveFallbackEligibleFailures,
                rendering::backendName(impl_->context.backend()),
                rendering::surfaceFailureReasonName(failureReason));
            if (tryPromoteNextBackend()) {
                return true;
            }
        }

        syncTextInputState();
        return true;
    };

    const bool surfaceSizeChanged =
        impl_->surfaceWidth != framebufferWidth || impl_->surfaceHeight != framebufferHeight;
    if (!surfaceSizeChanged) {
        impl_->pendingSurfaceRecreate = false;
    }

    if (!impl_->surface || surfaceSizeChanged) {
        if (impl_->pendingSurfaceRecreate
            && shouldCoalesceInteractiveSurfaceRecreate(impl_->context.backend())
            && impl_->lastSurfaceRecreateAt != std::chrono::steady_clock::time_point {}) {
            const auto now = nowSteadyTime();
            if (now - impl_->lastSurfaceRecreateAt < kInteractiveSurfaceRecreateMinInterval) {
                impl_->uiContext.requestRedraw();
                syncTextInputState();
                return true;
            }
        }

        core::logInfoCat(
            "app",
            "Recreating window surface for framebuffer {}x{}",
            framebufferWidth,
            framebufferHeight);
        impl_->surface = detail::runtimeHooks().createWindowSurface(
            impl_->context,
            *impl_->window);
        if (!impl_->surface) {
            impl_->surfaceWidth = 0;
            impl_->surfaceHeight = 0;
            impl_->pendingSurfaceRecreate = false;
            impl_->runtimeSurfaceFailureRecoveryState = {};
            impl_->uiContext.requestRedraw();
            if (tryPromoteNextBackend()) {
                return true;
            }
            return true;
        }
        impl_->surfaceWidth = framebufferWidth;
        impl_->surfaceHeight = framebufferHeight;
        impl_->pendingSurfaceRecreate = false;
        impl_->lastSurfaceRecreateAt = nowSteadyTime();
    }

    const rendering::FramePrepareStatus framePrepareStatus =
        detail::runtimeHooks().prepareFrame(impl_->context, impl_->surface);
    if (framePrepareStatus == rendering::FramePrepareStatus::RetryLater) {
        const rendering::SurfaceFailureReason failureReason =
            rendering::lastSurfaceFailureReason(impl_->surface);
        logSurfaceFailureEvent(
            impl_->surfaceFailureLogState,
            impl_->context.backend(),
            failureReason,
            SurfaceFailureLogStage::RetryLater);
        impl_->runtimeSurfaceFailureRecoveryState = {};
        impl_->uiContext.requestRedraw();
        syncTextInputState();
        return true;
    }
    if (framePrepareStatus == rendering::FramePrepareStatus::SurfaceLost) {
        const rendering::SurfaceFailureReason failureReason =
            rendering::lastSurfaceFailureReason(impl_->surface);
        logSurfaceFailureEvent(
            impl_->surfaceFailureLogState,
            impl_->context.backend(),
            failureReason,
            SurfaceFailureLogStage::SurfaceLost);
        return handleRuntimeSurfaceFailure(failureReason);
    }

    rendering::Canvas canvas = impl_->surface.canvas();
    if (!canvas) {
        const rendering::SurfaceFailureReason failureReason =
            rendering::lastSurfaceFailureReason(impl_->surface);
        logSurfaceFailureEvent(
            impl_->surfaceFailureLogState,
            impl_->context.backend(),
            failureReason,
            SurfaceFailureLogStage::CanvasUnavailable);
        return handleRuntimeSurfaceFailure(failureReason);
    }
    impl_->surfaceFailureLogState = {};
    syncResourceManagerDevicePixelRatio(
        impl_->window.get(),
        metrics,
        impl_->resourceManagerBindingId,
        impl_->syncedDevicePixelRatio);
    const bool fullRedraw =
        impl_->uiContext.render(canvas, framebufferWidth, framebufferHeight, dpiScale);

    syncTextInputState();
    detail::runtimeHooks().flushFrame(impl_->context, impl_->surface);
    if (!impl_->surface) {
        const rendering::SurfaceFailureReason failureReason =
            rendering::lastSurfaceFailureReason(impl_->surface);
        return handleRuntimeSurfaceFailure(failureReason);
    }
    impl_->window->swapBuffers();
    impl_->runtimeSurfaceFailureRecoveryState = {};
    impl_->lastRenderFrameOutcome = RenderFrameOutcome::Presented;
    return fullRedraw;
}

void Application::syncTextInputState()
{
    if (!impl_ || impl_->window == nullptr) {
        return;
    }

    const bool targetTextInputActive = impl_->uiContext.textInputActive();
    if (impl_->window->textInputActive() != targetTextInputActive) {
        impl_->window->setTextInputActive(targetTextInputActive);
    }

    const std::optional<core::Rect> targetCursorRect = logicalToPhysicalRect(
        impl_->uiContext.imeCursorRect(),
        impl_->window->metrics().dpiScale);
    if (impl_->window->textInputCursorRect() != targetCursorRect) {
        impl_->window->setTextInputCursorRect(targetCursorRect);
    }
}

}  // namespace tinalux::app
