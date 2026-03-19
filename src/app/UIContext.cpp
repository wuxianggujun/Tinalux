#include "UIContext.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "../ui/RuntimeState.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/Log.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ResourceLoader.h"
#include "tinalux/ui/ThemeManager.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::app {

namespace {

constexpr float kGeometryTolerance = 0.001f;
constexpr std::size_t kMaxPartialRedrawRegions = 8;
constexpr float kMaxPartialRedrawCoverage = 0.3f;

float sanitizeDpiScale(float dpiScale)
{
    return std::isfinite(dpiScale) && dpiScale > 0.0f ? dpiScale : 1.0f;
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= kGeometryTolerance;
}

void appendRedrawRegion(std::vector<core::Rect>& regions, const core::Rect& candidate)
{
    if (candidate.isEmpty()) {
        return;
    }

    regions.push_back(candidate);

    bool merged = true;
    while (merged) {
        merged = false;
        for (std::size_t index = 0; index < regions.size() && !merged; ++index) {
            for (std::size_t other = index + 1; other < regions.size(); ++other) {
                if (!regions[index].intersects(regions[other])) {
                    continue;
                }

                regions[index].join(regions[other]);
                regions.erase(regions.begin() + static_cast<std::ptrdiff_t>(other));
                merged = true;
                break;
            }
        }
    }
}

bool shouldPromoteToFullRedraw(
    const std::vector<core::Rect>& regions,
    const core::Size& logicalSize)
{
    if (regions.empty()) {
        return false;
    }

    if (regions.size() > kMaxPartialRedrawRegions) {
        return true;
    }

    const float logicalWidth = std::max(0.0f, logicalSize.width());
    const float logicalHeight = std::max(0.0f, logicalSize.height());
    const float fullArea = logicalWidth * logicalHeight;
    if (fullArea <= 0.0f) {
        return false;
    }

    float coveredArea = 0.0f;
    for (const auto& region : regions) {
        const float left = std::clamp(region.left(), 0.0f, logicalWidth);
        const float top = std::clamp(region.top(), 0.0f, logicalHeight);
        const float right = std::clamp(region.right(), 0.0f, logicalWidth);
        const float bottom = std::clamp(region.bottom(), 0.0f, logicalHeight);
        if (right <= left || bottom <= top) {
            continue;
        }

        coveredArea += (right - left) * (bottom - top);
        if (coveredArea >= fullArea * kMaxPartialRedrawCoverage) {
            return true;
        }
    }

    return false;
}

core::Size logicalFramebufferSize(
    int framebufferWidth,
    int framebufferHeight,
    float dpiScale)
{
    const float sanitizedScale = sanitizeDpiScale(dpiScale);
    return core::Size::Make(
        static_cast<float>(framebufferWidth) / sanitizedScale,
        static_cast<float>(framebufferHeight) / sanitizedScale);
}

void normalizePointerEventCoordinates(core::Event& event, float dpiScale)
{
    const double inverseScale = 1.0 / static_cast<double>(sanitizeDpiScale(dpiScale));
    switch (event.type()) {
    case core::EventType::MouseMove: {
        auto& mouseEvent = static_cast<core::MouseMoveEvent&>(event);
        mouseEvent.x *= inverseScale;
        mouseEvent.y *= inverseScale;
        break;
    }
    case core::EventType::MouseEnter:
    case core::EventType::MouseLeave: {
        auto& mouseEvent = static_cast<core::MouseCrossEvent&>(event);
        mouseEvent.x *= inverseScale;
        mouseEvent.y *= inverseScale;
        break;
    }
    case core::EventType::MouseButtonPress:
    case core::EventType::MouseButtonRelease: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        mouseEvent.x *= inverseScale;
        mouseEvent.y *= inverseScale;
        break;
    }
    default:
        break;
    }
}

bool isKeyboardEvent(core::EventType type)
{
    return type == core::EventType::KeyPress
        || type == core::EventType::KeyRelease
        || type == core::EventType::KeyRepeat
        || type == core::EventType::TextInput
        || type == core::EventType::TextCompositionStart
        || type == core::EventType::TextCompositionUpdate
        || type == core::EventType::TextCompositionEnd;
}

std::vector<std::shared_ptr<ui::Widget>> buildEventPath(
    const std::shared_ptr<ui::Widget>& target)
{
    std::vector<std::shared_ptr<ui::Widget>> path;
    for (std::shared_ptr<ui::Widget> current = target;
         current != nullptr;
         current = current->parent() != nullptr ? current->parent()->sharedHandle() : nullptr) {
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

ui::Widget* findFocusableWidget(ui::Widget* target)
{
    for (ui::Widget* current = target; current != nullptr; current = current->parent()) {
        if (current->focusable()) {
            return current;
        }
    }
    return nullptr;
}

void collectFocusableWidgets(ui::Widget* widget, std::vector<ui::Widget*>& out)
{
    if (widget == nullptr || !widget->visible()) {
        return;
    }

    if (widget->focusable()) {
        out.push_back(widget);
    }

    if (auto* container = dynamic_cast<ui::Container*>(widget); container != nullptr) {
        for (const auto& child : container->children()) {
            collectFocusableWidgets(child.get(), out);
        }
    }
}

bool dispatchAlongPath(core::Event& event, const std::shared_ptr<ui::Widget>& target)
{
    if (target == nullptr) {
        return false;
    }

    const std::vector<std::shared_ptr<ui::Widget>> path = buildEventPath(target);
    if (path.empty()) {
        return false;
    }

    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        if (path[i]->onEventCapture(event)) {
            event.handled = true;
            return true;
        }
        if (event.handled || event.stopPropagation) {
            return true;
        }
    }

    event.handled = path.back()->onEvent(event) || event.handled;
    if (event.handled || event.stopPropagation) {
        return true;
    }

    for (std::size_t i = path.size() - 1; i > 0; --i) {
        event.handled = path[i - 1]->onEvent(event) || event.handled;
        if (event.handled || event.stopPropagation) {
            return true;
        }
    }

    return event.handled;
}

void sendMouseCrossEvent(
    core::EventType type,
    const std::shared_ptr<ui::Widget>& widget,
    float x,
    float y)
{
    if (widget == nullptr) {
        return;
    }

    core::MouseCrossEvent event(x, y, type);
    widget->onEvent(event);
}

std::string toLowerCopy(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::optional<bool> parseBool(std::string value)
{
    value = toLowerCopy(std::move(value));
    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }
    return std::nullopt;
}

std::optional<std::uint64_t> parseUint64(std::string value)
{
    try {
        std::size_t parsedCharacters = 0;
        const unsigned long long parsed = std::stoull(value, &parsedCharacters, 10);
        if (parsedCharacters != value.size()) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(parsed);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> getEnvString(const char* name)
{
    if (name == nullptr) {
        return std::nullopt;
    }

    const char* value = std::getenv(name);
    if (value == nullptr) {
        return std::nullopt;
    }

    return std::string(value);
}

PerfLogConfig sanitizePerfLogConfig(PerfLogConfig config)
{
    config.frameInterval = std::max<std::uint64_t>(1, config.frameInterval);
    return config;
}

DebugHudConfig sanitizeDebugHudConfig(DebugHudConfig config)
{
    config.scale = std::clamp(config.scale, 0.5f, 2.5f);
    return config;
}

PerfLogConfig applyPerfLogEnvironmentOverrides(PerfLogConfig config)
{
    if (const auto value = getEnvString("TINALUX_APP_PERF_LOG")) {
        if (const auto parsed = parseBool(*value)) {
            config.enabled = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_APP_PERF_LOG_INTERVAL_FRAMES")) {
        if (const auto parsed = parseUint64(*value)) {
            config.frameInterval = *parsed;
        }
    }

    return sanitizePerfLogConfig(config);
}

DebugHudConfig applyDebugHudEnvironmentOverrides(DebugHudConfig config)
{
    if (const auto value = getEnvString("TINALUX_APP_DEBUG_HUD")) {
        if (const auto parsed = parseBool(*value)) {
            config.enabled = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_APP_DEBUG_HUD_DIRTY_REGION")) {
        if (const auto parsed = parseBool(*value)) {
            config.highlightDirtyRegion = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_APP_DEBUG_HUD_SCALE")) {
        try {
            std::size_t parsedCharacters = 0;
            const float parsed = std::stof(*value, &parsedCharacters);
            if (parsedCharacters == value->size()) {
                config.scale = parsed;
            }
        } catch (...) {
        }
    }

    return sanitizeDebugHudConfig(config);
}

}  // namespace

UIContext::UIContext()
    : runtimeState_(std::make_unique<ui::RuntimeState>())
{
    ui::ThemeManager& themeManager = ui::ThemeManager::instance();
    runtimeState_->theme = themeManager.currentTheme();
    themeListenerId_ = themeManager.addThemeChangeListener([this](const ui::Theme& theme) {
        if (runtimeState_ == nullptr) {
            return;
        }

        runtimeState_->theme = theme;
        ++runtimeState_->themeGeneration;
        needsRedraw_ = true;
    });
    bindThemeRuntime();
}

UIContext::~UIContext()
{
    shutdown();
    ui::ThemeManager& themeManager = ui::ThemeManager::instance();
    themeManager.removeThemeChangeListener(themeListenerId_);
}

void UIContext::initializeFromEnvironment()
{
    resetForStartup();

    if (!perfLogConfiguredExplicitly_) {
        perfLogConfig_ = applyPerfLogEnvironmentOverrides(perfLogConfig_);
    } else {
        perfLogConfig_ = sanitizePerfLogConfig(perfLogConfig_);
    }

    if (!debugHudConfiguredExplicitly_) {
        debugHudConfig_ = applyDebugHudEnvironmentOverrides(debugHudConfig_);
    } else {
        debugHudConfig_ = sanitizeDebugHudConfig(debugHudConfig_);
    }

    ui::ThemeManager& themeManager = ui::ThemeManager::instance();
    if (const std::string savedTheme = themeManager.loadThemePreference(); !savedTheme.empty()) {
        themeManager.switchTheme(savedTheme, false);
    }
}

void UIContext::shutdown()
{
    if (shutdown_) {
        return;
    }
    shutdown_ = true;

    ui::ScopedRuntimeState runtimeScope(*runtimeState_);

    if (perfLogConfig_.enabled && perfLogIntervalStats_.renderedFrames > 0) {
        logPeriodicPerfSummary("shutdown");
    }

    if (frameStats_.totalFrames > 0) {
        core::logInfoCat(
            "app",
            "Frame stats: total={} full={} partial={} skipped={} wait_loops={} poll_loops={} avg_ms={:.3f} max_ms={:.3f}",
            frameStats_.totalFrames,
            frameStats_.fullRedrawFrames,
            frameStats_.partialRedrawFrames,
            frameStats_.skippedFrames,
            frameStats_.waitEventLoops,
            frameStats_.pollEventLoops,
            frameStats_.averageFrameMs,
            frameStats_.maxFrameMs);
        core::flushLog();
    }

    unbindThemeRuntime();
    runtimeState_->animationScheduler.clear();

    if (const auto focused = lockWidget(focusedWidget_)) {
        focused->setFocused(false);
    }

    rootWidget_.reset();
    overlayWidget_.reset();
    focusedWidget_.reset();
    hoveredWidget_.reset();
    mouseCaptureWidget_.reset();
    needsRedraw_ = true;
    layoutWidth_ = 0;
    layoutHeight_ = 0;
    lastDpiScale_ = 1.0f;
    frameStats_ = {};
    perfLogIntervalStats_ = {};
}

void UIContext::handleEvent(
    core::Event& event,
    const std::function<void()>& requestClose,
    float dpiScale)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    normalizePointerEventCoordinates(event, dpiScale);

    switch (event.type()) {
    case core::EventType::WindowClose:
        core::logInfoCat("app", "Handling WindowClose event");
        if (requestClose) {
            requestClose();
        }
        event.handled = true;
        break;
    case core::EventType::WindowResize:
        core::logDebugCat("app", "Handling WindowResize event");
        needsRedraw_ = true;
        break;
    case core::EventType::WindowFocus: {
        const auto& focusEvent = static_cast<const core::WindowFocusEvent&>(event);
        core::logDebugCat("app", "Handling WindowFocus event focused={}", focusEvent.focused);
        if (!focusEvent.focused) {
            if (const auto hovered = lockWidget(hoveredWidget_)) {
                const core::Rect hoveredBounds = hovered->globalBounds();
                sendMouseCrossEvent(
                    core::EventType::MouseLeave,
                    hovered,
                    hoveredBounds.centerX(),
                    hoveredBounds.centerY());
            }
            setFocus(nullptr);
            hoveredWidget_.reset();
            mouseCaptureWidget_.reset();
        }
        break;
    }
    case core::EventType::KeyPress: {
        auto& keyEvent = static_cast<core::KeyEvent&>(event);
        if (keyEvent.key == core::keys::kEscape) {
            if (requestClose) {
                requestClose();
            }
            event.handled = true;
        } else if (keyEvent.key == core::keys::kTab) {
            advanceFocus((keyEvent.mods & core::mods::kShift) != 0);
            event.handled = true;
        }
        break;
    }
    default:
        break;
    }

    if (!event.handled) {
        dispatchEvent(event);
    }

    if ((rootWidget_ != nullptr && rootWidget_->isDirty())
        || (overlayWidget_ != nullptr && overlayWidget_->isDirty())) {
        needsRedraw_ = true;
    }
}

void UIContext::setRootWidget(std::shared_ptr<ui::Widget> root)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    runtimeState_->animationScheduler.clear();

    if (const auto focused = lockWidget(focusedWidget_)) {
        focused->setFocused(false);
    }

    rootWidget_ = std::move(root);
    focusedWidget_.reset();
    hoveredWidget_.reset();
    mouseCaptureWidget_.reset();
    needsRedraw_ = true;
    layoutWidth_ = 0;
    layoutHeight_ = 0;
    core::logInfoCat(
        "app",
        "Root widget set to {}",
        rootWidget_ != nullptr
            ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(rootWidget_.get()))
            : std::string("null"));
}

std::shared_ptr<ui::Widget> UIContext::buildWidgetTree(
    const std::function<std::shared_ptr<ui::Widget>()>& builder)
{
    if (runtimeState_ == nullptr || !builder) {
        return {};
    }

    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    return builder();
}

void UIContext::setOverlayWidget(std::shared_ptr<ui::Widget> overlay)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);

    if (const auto hovered = lockWidget(hoveredWidget_)) {
        const core::Rect hoveredBounds = hovered->globalBounds();
        sendMouseCrossEvent(
            core::EventType::MouseLeave,
            hovered,
            hoveredBounds.centerX(),
            hoveredBounds.centerY());
    }

    setFocus(nullptr);
    hoveredWidget_.reset();
    mouseCaptureWidget_.reset();
    overlayWidget_ = std::move(overlay);
    if (overlayWidget_ != nullptr) {
        overlayWidget_->markLayoutDirty();
    }
    needsRedraw_ = true;

    core::logInfoCat(
        "app",
        "Overlay widget set to {}",
        overlayWidget_ != nullptr
            ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(overlayWidget_.get()))
            : std::string("null"));
}

void UIContext::clearOverlayWidget()
{
    setOverlayWidget(nullptr);
}

ui::AnimationSink& UIContext::animationSink()
{
    return runtimeState_->animationScheduler;
}

bool UIContext::textInputActive()
{
    if (runtimeState_ == nullptr) {
        return false;
    }

    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    if (const auto focused = lockWidget(focusedWidget_)) {
        return focused->wantsTextInput();
    }
    return false;
}

std::optional<core::Rect> UIContext::imeCursorRect()
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    const auto focused = lockWidget(focusedWidget_);
    if (focused == nullptr) {
        return std::nullopt;
    }
    return focused->imeCursorRect();
}

FrameStats UIContext::frameStats() const
{
    return frameStats_;
}

void UIContext::resetFrameStats()
{
    frameStats_ = {};
}

void UIContext::setTheme(ui::Theme theme)
{
    ui::ThemeManager::instance().setTheme(theme, false);
    core::logInfoCat(
        "app",
        "Theme updated background=#{:08x} primary=#{:08x}",
        static_cast<unsigned int>(theme.colors.background),
        static_cast<unsigned int>(theme.colors.primary));
}

ui::Theme UIContext::theme() const
{
    return runtimeState_ != nullptr
        ? runtimeState_->theme
        : ui::ThemeManager::instance().currentTheme();
}

void UIContext::setPartialRedrawEnabled(bool enabled)
{
    if (partialRedrawEnabled_ == enabled) {
        return;
    }

    partialRedrawEnabled_ = enabled;
    needsRedraw_ = true;
}

bool UIContext::partialRedrawEnabled() const
{
    return partialRedrawEnabled_;
}

void UIContext::setPerfLogConfig(PerfLogConfig config)
{
    perfLogConfig_ = sanitizePerfLogConfig(config);
    perfLogConfiguredExplicitly_ = true;
    perfLogIntervalStats_ = {};
    core::logInfoCat(
        "perf",
        "Periodic performance summary {} (interval={} rendered frames)",
        perfLogConfig_.enabled ? "enabled" : "disabled",
        perfLogConfig_.frameInterval);
}

PerfLogConfig UIContext::perfLogConfig() const
{
    return perfLogConfig_;
}

void UIContext::setDebugHudConfig(DebugHudConfig config)
{
    debugHudConfig_ = sanitizeDebugHudConfig(config);
    debugHudConfiguredExplicitly_ = true;
    needsRedraw_ = true;
    core::logInfoCat(
        "app",
        "Debug HUD {} (highlight_dirty_region={} scale={:.2f})",
        debugHudConfig_.enabled ? "enabled" : "disabled",
        debugHudConfig_.highlightDirtyRegion,
        debugHudConfig_.scale);
}

DebugHudConfig UIContext::debugHudConfig() const
{
    return debugHudConfig_;
}

void UIContext::noteWaitLoop()
{
    ++frameStats_.waitEventLoops;
    ++perfLogIntervalStats_.waitEventLoops;
}

void UIContext::notePollLoop()
{
    ++frameStats_.pollEventLoops;
    ++perfLogIntervalStats_.pollEventLoops;
}

void UIContext::noteAnimationTickUpdated()
{
    needsRedraw_ = true;
}

void UIContext::noteFrameRendered(bool fullRedraw, double frameMs)
{
    ++frameStats_.totalFrames;
    if (fullRedraw) {
        ++frameStats_.fullRedrawFrames;
    } else {
        ++frameStats_.partialRedrawFrames;
    }
    frameStats_.lastFrameMs = frameMs;
    frameStats_.maxFrameMs = std::max(frameStats_.maxFrameMs, frameMs);
    const double frameCount = static_cast<double>(frameStats_.totalFrames);
    frameStats_.averageFrameMs += (frameMs - frameStats_.averageFrameMs) / frameCount;

    ++perfLogIntervalStats_.renderedFrames;
    if (fullRedraw) {
        ++perfLogIntervalStats_.fullRedrawFrames;
    } else {
        ++perfLogIntervalStats_.partialRedrawFrames;
    }
    perfLogIntervalStats_.totalFrameMs += frameMs;
    perfLogIntervalStats_.lastFrameMs = frameMs;
    perfLogIntervalStats_.maxFrameMs =
        std::max(perfLogIntervalStats_.maxFrameMs, frameMs);

    needsRedraw_ = false;
    maybeLogPeriodicPerfSummary();
}

void UIContext::noteFrameSkipped()
{
    ++frameStats_.skippedFrames;
    ++perfLogIntervalStats_.skippedFrames;
}

bool UIContext::tickAnimations(double nowSeconds)
{
    return runtimeState_ != nullptr && runtimeState_->animationScheduler.tick(nowSeconds);
}

bool UIContext::tickAsyncResources()
{
    if (runtimeState_ == nullptr) {
        return false;
    }

    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    return ui::ResourceLoader::instance().pumpCallbacks();
}

bool UIContext::hasActiveAnimations() const
{
    return runtimeState_ != nullptr && runtimeState_->animationScheduler.hasActiveAnimations();
}

std::optional<double> UIContext::nextAnimationDelaySeconds(double nowSeconds) const
{
    if (runtimeState_ == nullptr) {
        return std::nullopt;
    }
    return runtimeState_->animationScheduler.nextWakeDelaySeconds(nowSeconds);
}

bool UIContext::shouldRender() const
{
    return hasImmediateRenderWork() || hasActiveAnimations();
}

bool UIContext::hasImmediateRenderWork() const
{
    return needsRedraw_
        || (rootWidget_ != nullptr && rootWidget_->isDirty())
        || (overlayWidget_ != nullptr && overlayWidget_->isDirty());
}

bool UIContext::render(
    rendering::Canvas& canvas,
    int framebufferWidth,
    int framebufferHeight,
    float dpiScale)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    if (!canvas || framebufferWidth <= 0 || framebufferHeight <= 0) {
        return true;
    }

    const float deviceScale = sanitizeDpiScale(dpiScale);
    runtimeState_->devicePixelRatio = deviceScale;
    const core::Size logicalSize =
        logicalFramebufferSize(framebufferWidth, framebufferHeight, deviceScale);
    const bool logicalSizeChanged = !nearlyEqual(layoutWidth_, logicalSize.width())
        || !nearlyEqual(layoutHeight_, logicalSize.height());
    const bool dpiChanged = !nearlyEqual(lastDpiScale_, deviceScale);

    const bool hasScene = rootWidget_ != nullptr || overlayWidget_ != nullptr;
    if (!hasScene && !loggedEmptyScene_) {
        core::logWarnCat(
            "app",
            "UI render has no root or overlay widget; only the clear color will be visible");
        loggedEmptyScene_ = true;
        loggedSceneReady_ = false;
    } else if (hasScene && !loggedSceneReady_) {
        core::logInfoCat(
            "app",
            "UI render scene is ready root={} overlay={}",
            rootWidget_ != nullptr
                ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(rootWidget_.get()))
                : std::string("null"),
            overlayWidget_ != nullptr
                ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(overlayWidget_.get()))
                : std::string("null"));
        loggedSceneReady_ = true;
        loggedEmptyScene_ = false;
    }

    constexpr core::Color kClearColor = core::colorRGB(18, 20, 28);
    const bool baseFullRedraw = !partialRedrawEnabled_
        || (rootWidget_ == nullptr && overlayWidget_ == nullptr)
        || (rootWidget_ != nullptr
            && (rootWidget_->isLayoutDirty() || !rootWidget_->hasDirtyRegion()))
        || (overlayWidget_ != nullptr
            && (overlayWidget_->isLayoutDirty() || !overlayWidget_->hasDirtyRegion()))
        || logicalSizeChanged
        || dpiChanged;
    std::vector<core::Rect> redrawRegions;
    core::Rect redrawRegion = core::Rect::MakeEmpty();
    if (!baseFullRedraw) {
        if (rootWidget_ != nullptr && rootWidget_->hasDirtyRegion()) {
            std::vector<core::Rect> rootRegions;
            rootWidget_->collectDirtyDrawRegions(rootRegions);
            for (const auto& region : rootRegions) {
                appendRedrawRegion(redrawRegions, region);
            }
        }
        if (overlayWidget_ != nullptr && overlayWidget_->hasDirtyRegion()) {
            std::vector<core::Rect> overlayRegions;
            overlayWidget_->collectDirtyDrawRegions(overlayRegions);
            for (const auto& region : overlayRegions) {
                appendRedrawRegion(redrawRegions, region);
            }
        }
    }
    if (debugHudConfig_.enabled) {
        const core::Rect hudBounds =
            debugHudBounds(logicalSize.width(), logicalSize.height());
        appendRedrawRegion(redrawRegions, hudBounds);
    }
    const bool fullRedraw = baseFullRedraw
        || shouldPromoteToFullRedraw(redrawRegions, logicalSize);
    for (const auto& region : redrawRegions) {
        if (redrawRegion.isEmpty()) {
            redrawRegion = region;
        } else {
            redrawRegion.join(region);
        }
    }

    if (fullRedraw) {
        canvas.clear(kClearColor);
    }
    canvas.save();
    // 布局统一使用逻辑像素，最终在这里一次性映射到物理像素。
    canvas.scale(deviceScale, deviceScale);
    if (!fullRedraw) {
        for (const auto& region : redrawRegions) {
            canvas.clearRect(region, kClearColor);
        }
    }

    const auto fullBounds = core::Rect::MakeXYWH(
        0.0f,
        0.0f,
        logicalSize.width(),
        logicalSize.height());
    const auto tightConstraints =
        ui::Constraints::tight(logicalSize.width(), logicalSize.height());

    if (rootWidget_ != nullptr) {
        if (rootWidget_->isLayoutDirty() || logicalSizeChanged) {
            rootWidget_->measure(tightConstraints);
            rootWidget_->arrange(fullBounds);
        }
    }
    if (overlayWidget_ != nullptr) {
        if (overlayWidget_->isLayoutDirty() || logicalSizeChanged) {
            overlayWidget_->measure(tightConstraints);
            overlayWidget_->arrange(fullBounds);
        }
    }
    layoutWidth_ = logicalSize.width();
    layoutHeight_ = logicalSize.height();
    lastDpiScale_ = deviceScale;

    if (rootWidget_ != nullptr) {
        if (fullRedraw) {
            rootWidget_->draw(canvas);
        } else {
            for (const auto& region : redrawRegions) {
                canvas.save();
                canvas.clipRect(region);
                rootWidget_->drawPartial(canvas, region);
                canvas.restore();
            }
        }
    }
    if (overlayWidget_ != nullptr) {
        if (fullRedraw) {
            overlayWidget_->draw(canvas);
        } else {
            for (const auto& region : redrawRegions) {
                canvas.save();
                canvas.clipRect(region);
                overlayWidget_->drawPartial(canvas, region);
                canvas.restore();
            }
        }
    }

    if (debugHudConfig_.enabled) {
        drawDebugHud(
            canvas,
            logicalSize.width(),
            logicalSize.height(),
            fullRedraw,
            redrawRegion);
    }
    canvas.restore();

    return fullRedraw;
}

void UIContext::clearNeedsRedraw()
{
    needsRedraw_ = false;
}

ui::Widget* UIContext::hitTestTopLevel(float x, float y) const
{
    if (overlayWidget_ != nullptr) {
        if (ui::Widget* target = overlayWidget_->hitTestGlobal(x, y); target != nullptr) {
            return target;
        }
    }

    return rootWidget_ != nullptr ? rootWidget_->hitTestGlobal(x, y) : nullptr;
}

void UIContext::collectActiveFocusables(std::vector<ui::Widget*>& out) const
{
    if (overlayWidget_ != nullptr) {
        collectFocusableWidgets(overlayWidget_.get(), out);
        if (!out.empty()) {
            return;
        }
    }

    collectFocusableWidgets(rootWidget_.get(), out);
}

void UIContext::dispatchEvent(core::Event& event)
{
    if (rootWidget_ == nullptr && overlayWidget_ == nullptr) {
        return;
    }

    switch (event.type()) {
    case core::EventType::MouseEnter: {
        auto& mouseEvent = static_cast<core::MouseCrossEvent&>(event);
        const auto hovered = lockWidget(hoveredWidget_);
        auto target = lockActiveWidget(hitTestTopLevel(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        if (target != hovered) {
            sendMouseCrossEvent(
                core::EventType::MouseLeave,
                hovered,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
            target = lockActiveWidget(target.get());
            hoveredWidget_ = target != nullptr ? target->weakHandle() : std::weak_ptr<ui::Widget> {};
            sendMouseCrossEvent(
                core::EventType::MouseEnter,
                target,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }
        return;
    }
    case core::EventType::MouseLeave: {
        auto& mouseEvent = static_cast<core::MouseCrossEvent&>(event);
        const auto hovered = lockWidget(hoveredWidget_);
        sendMouseCrossEvent(
            core::EventType::MouseLeave,
            hovered,
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        hoveredWidget_.reset();
        return;
    }
    case core::EventType::MouseMove: {
        auto& mouseEvent = static_cast<core::MouseMoveEvent&>(event);
        const auto hovered = lockWidget(hoveredWidget_);
        auto target = lockActiveWidget(hitTestTopLevel(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));

        if (target != hovered) {
            sendMouseCrossEvent(
                core::EventType::MouseLeave,
                hovered,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
            target = lockActiveWidget(target.get());
            hoveredWidget_ = target != nullptr ? target->weakHandle() : std::weak_ptr<ui::Widget> {};
            sendMouseCrossEvent(
                core::EventType::MouseEnter,
                target,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }

        const auto mouseCapture = lockWidget(mouseCaptureWidget_);
        dispatchAlongPath(
            event,
            mouseCapture != nullptr
                ? mouseCapture
                : target);
        return;
    }
    case core::EventType::MouseButtonPress: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        ui::Widget* target = hitTestTopLevel(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        mouseCaptureWidget_ = target != nullptr ? target->weakHandle() : std::weak_ptr<ui::Widget> {};
        setFocus(
            findFocusableWidget(target) != nullptr
                ? findFocusableWidget(target)->sharedHandle()
                : std::shared_ptr<ui::Widget> {});
        dispatchAlongPath(
            event,
            target != nullptr ? target->sharedHandle() : std::shared_ptr<ui::Widget> {});
        return;
    }
    case core::EventType::MouseButtonRelease: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        auto target = lockWidget(mouseCaptureWidget_);
        if (target == nullptr) {
            if (ui::Widget* hit = hitTestTopLevel(
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
                hit != nullptr) {
                target = lockActiveWidget(hit);
            }
        }
        dispatchAlongPath(event, target);
        mouseCaptureWidget_.reset();
        return;
    }
    case core::EventType::MouseScroll: {
        auto target = lockWidget(hoveredWidget_);
        if (target == nullptr) {
            target = overlayWidget_ != nullptr ? overlayWidget_ : rootWidget_;
        }
        dispatchAlongPath(event, target);
        return;
    }
    default:
        break;
    }

    if (isKeyboardEvent(event.type())) {
        dispatchAlongPath(event, lockWidget(focusedWidget_));
    }
}

void UIContext::setFocus(const std::shared_ptr<ui::Widget>& widget)
{
    const auto previousFocused = lockWidget(focusedWidget_);
    if (previousFocused == widget) {
        return;
    }

    if (previousFocused != nullptr) {
        previousFocused->setFocused(false);
    }

    focusedWidget_ = widget;
    if (widget != nullptr) {
        widget->setFocused(true);
    }

    needsRedraw_ = true;
    core::logDebugCat(
        "app",
        "Focus changed from {} to {}",
        previousFocused != nullptr
            ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(previousFocused.get()))
            : std::string("null"),
        widget != nullptr
            ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(widget.get()))
            : std::string("null"));
}

void UIContext::advanceFocus(bool reverse)
{
    std::vector<ui::Widget*> focusables;
    collectActiveFocusables(focusables);
    if (focusables.empty()) {
        setFocus(nullptr);
        return;
    }

    const auto focused = lockWidget(focusedWidget_);
    if (focused == nullptr) {
        setFocus((reverse ? focusables.back() : focusables.front())->sharedHandle());
        return;
    }

    const auto it = std::find(focusables.begin(), focusables.end(), focused.get());
    if (it == focusables.end()) {
        setFocus((reverse ? focusables.back() : focusables.front())->sharedHandle());
        return;
    }

    const std::ptrdiff_t currentIndex = std::distance(focusables.begin(), it);
    const std::ptrdiff_t count = static_cast<std::ptrdiff_t>(focusables.size());
    const std::ptrdiff_t nextIndex = reverse
        ? (currentIndex - 1 + count) % count
        : (currentIndex + 1) % count;
    setFocus(focusables[static_cast<std::size_t>(nextIndex)]->sharedHandle());
}

std::shared_ptr<ui::Widget> UIContext::lockActiveWidget(ui::Widget* widget) const
{
    if (widget == nullptr) {
        return {};
    }

    const auto locked = widget->sharedHandle();
    if (locked == nullptr || !isInActiveTree(locked.get())) {
        return {};
    }
    return locked;
}

std::shared_ptr<ui::Widget> UIContext::lockWidget(
    const std::weak_ptr<ui::Widget>& widget) const
{
    const auto locked = widget.lock();
    if (locked == nullptr || !isInActiveTree(locked.get())) {
        return {};
    }
    return locked;
}

bool UIContext::isInActiveTree(const ui::Widget* widget) const
{
    if (widget == nullptr) {
        return false;
    }

    for (const ui::Widget* current = widget; current != nullptr; current = current->parent()) {
        if (current == overlayWidget_.get() || current == rootWidget_.get()) {
            return true;
        }
    }

    return false;
}

void UIContext::maybeLogPeriodicPerfSummary()
{
    if (!perfLogConfig_.enabled
        || perfLogIntervalStats_.renderedFrames < perfLogConfig_.frameInterval) {
        return;
    }

    logPeriodicPerfSummary("interval");
}

void UIContext::logPeriodicPerfSummary(const char* reason)
{
    if (perfLogIntervalStats_.renderedFrames == 0) {
        return;
    }

    const double averageFrameMs = perfLogIntervalStats_.totalFrameMs
        / static_cast<double>(perfLogIntervalStats_.renderedFrames);
    core::logInfoCat(
        "perf",
        "Frame summary [{}]: frames={} full={} partial={} skipped={} wait_loops={} poll_loops={} avg_ms={:.3f} max_ms={:.3f} last_ms={:.3f}",
        reason,
        perfLogIntervalStats_.renderedFrames,
        perfLogIntervalStats_.fullRedrawFrames,
        perfLogIntervalStats_.partialRedrawFrames,
        perfLogIntervalStats_.skippedFrames,
        perfLogIntervalStats_.waitEventLoops,
        perfLogIntervalStats_.pollEventLoops,
        averageFrameMs,
        perfLogIntervalStats_.maxFrameMs,
        perfLogIntervalStats_.lastFrameMs);
    perfLogIntervalStats_ = {};
}

void UIContext::bindThemeRuntime()
{
    if (runtimeState_ == nullptr || themeRuntimeBindingId_ != 0) {
        return;
    }

    themeRuntimeBindingId_ = ui::ThemeManager::instance().attachRuntime(
        &runtimeState_->animationScheduler,
        [this] { needsRedraw_ = true; });
}

void UIContext::unbindThemeRuntime()
{
    if (themeRuntimeBindingId_ == 0) {
        return;
    }

    ui::ThemeManager::instance().detachRuntime(themeRuntimeBindingId_);
    themeRuntimeBindingId_ = 0;
}

void UIContext::resetForStartup()
{
    shutdown_ = false;
    bindThemeRuntime();

    if (runtimeState_ != nullptr) {
        ui::ScopedRuntimeState runtimeScope(*runtimeState_);
        runtimeState_->animationScheduler.clear();
        runtimeState_->theme = ui::ThemeManager::instance().currentTheme();
        runtimeState_->devicePixelRatio = 1.0f;
        ++runtimeState_->themeGeneration;
        if (const auto focused = lockWidget(focusedWidget_)) {
            focused->setFocused(false);
        }
    }

    rootWidget_.reset();
    overlayWidget_.reset();
    focusedWidget_.reset();
    hoveredWidget_.reset();
    mouseCaptureWidget_.reset();
    needsRedraw_ = true;
    layoutWidth_ = 0.0f;
    layoutHeight_ = 0.0f;
    lastDpiScale_ = 1.0f;
    frameStats_ = {};
    perfLogIntervalStats_ = {};
    loggedEmptyScene_ = false;
    loggedSceneReady_ = false;
}

core::Rect UIContext::debugHudBounds(float logicalWidth, float logicalHeight) const
{
    const float scale = debugHudConfig_.scale;
    const float width = 280.0f * scale;
    const float height = 122.0f * scale;
    const float margin = 16.0f * scale;
    return core::Rect::MakeXYWH(
        std::max(0.0f, logicalWidth - width - margin),
        margin,
        width,
        std::min(height, std::max(0.0f, logicalHeight - margin * 2.0f)));
}

void UIContext::drawDebugHud(
    rendering::Canvas& canvas,
    float logicalWidth,
    float logicalHeight,
    bool fullRedraw,
    const core::Rect& dirtyRegion)
{
    if (!canvas) {
        return;
    }

    const DebugHudConfig config = debugHudConfig_;
    const core::Rect hudBounds = debugHudBounds(logicalWidth, logicalHeight);
    if (hudBounds.isEmpty()) {
        return;
    }

    if (config.highlightDirtyRegion && !fullRedraw && !dirtyRegion.isEmpty()) {
        canvas.drawRect(
            dirtyRegion,
            core::colorARGB(220, 255, 96, 96),
            rendering::PaintStyle::Stroke,
            std::max(1.5f, 2.0f * config.scale));
    }

    canvas.save();
    canvas.translate(hudBounds.x(), hudBounds.y());

    const core::Rect localHudBounds = hudBounds.makeOffset(-hudBounds.x(), -hudBounds.y());

    canvas.drawRoundRect(
        localHudBounds,
        12.0f * config.scale,
        12.0f * config.scale,
        core::colorARGB(210, 16, 20, 28));
    canvas.drawRoundRect(
        localHudBounds,
        12.0f * config.scale,
        12.0f * config.scale,
        core::colorARGB(255, 88, 126, 196),
        rendering::PaintStyle::Stroke,
        std::max(1.0f, 1.5f * config.scale));

    const float padding = 14.0f * config.scale;
    const float titleFontSize = 15.0f * config.scale;
    const float bodyFontSize = 12.0f * config.scale;
    const float lineHeight = std::max(16.0f * config.scale, bodyFontSize * 1.333f);
    float lineY = padding + titleFontSize;

    canvas.drawText("Debug HUD", padding, lineY, titleFontSize, core::colorRGB(222, 233, 248));
    lineY += lineHeight * 1.4f;

    const double avgFps = frameStats_.averageFrameMs > 0.001 ? 1000.0 / frameStats_.averageFrameMs : 0.0;
    const std::string redrawMode = fullRedraw ? "full" : "partial";
    const std::string dirtyInfo = dirtyRegion.isEmpty()
        ? "none"
        : fmt::format(
            "{:.0f},{:.0f} {:.0f}x{:.0f}",
            dirtyRegion.x(),
            dirtyRegion.y(),
            dirtyRegion.width(),
            dirtyRegion.height());

    const std::array<std::string, 6> lines {
        fmt::format("mode: {}", redrawMode),
        fmt::format("last / avg / max: {:.2f} / {:.2f} / {:.2f} ms", frameStats_.lastFrameMs, frameStats_.averageFrameMs, frameStats_.maxFrameMs),
        fmt::format("avg fps: {:.1f}", avgFps),
        fmt::format("frames f/p/s: {} / {} / {}", frameStats_.fullRedrawFrames, frameStats_.partialRedrawFrames, frameStats_.skippedFrames),
        fmt::format("loops wait/poll: {} / {}", frameStats_.waitEventLoops, frameStats_.pollEventLoops),
        fmt::format("dirty: {}", dirtyInfo),
    };

    for (const auto& line : lines) {
        canvas.drawText(line, padding, lineY, bodyFontSize, core::colorRGB(187, 198, 218));
        lineY += lineHeight;
    }

    canvas.restore();
}

}  // namespace tinalux::app
