#include "UIContext.h"

#include <algorithm>
#include <array>
#include <cctype>
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

bool isKeyboardEvent(core::EventType type)
{
    return type == core::EventType::KeyPress
        || type == core::EventType::KeyRelease
        || type == core::EventType::KeyRepeat
        || type == core::EventType::TextInput;
}

std::vector<ui::Widget*> buildEventPath(ui::Widget* target)
{
    std::vector<ui::Widget*> path;
    for (ui::Widget* current = target; current != nullptr; current = current->parent()) {
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

bool dispatchAlongPath(core::Event& event, ui::Widget* target)
{
    if (target == nullptr) {
        return false;
    }

    const std::vector<ui::Widget*> path = buildEventPath(target);
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
        ui::Widget* current = path[i - 1];
        event.handled = current->onEvent(event) || event.handled;
        if (event.handled || event.stopPropagation) {
            return true;
        }
    }

    return event.handled;
}

void sendMouseCrossEvent(core::EventType type, ui::Widget* widget, float x, float y)
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
        needsRedraw_ = true;
    });
    themeManager.setAnimationSink(&runtimeState_->animationScheduler);
    themeManager.setInvalidateCallback([this] { needsRedraw_ = true; });
}

UIContext::~UIContext()
{
    ui::ThemeManager& themeManager = ui::ThemeManager::instance();
    themeManager.removeThemeChangeListener(themeListenerId_);
    themeManager.setAnimationSink(nullptr);
    themeManager.setInvalidateCallback({});
}

void UIContext::initializeFromEnvironment()
{
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

    runtimeState_->animationScheduler.clear();
    ui::ResourceLoader::instance().clear();
    ui::clearClipboardHandlers();

    if (focusedWidget_ != nullptr) {
        focusedWidget_->setFocused(false);
    }

    rootWidget_.reset();
    overlayWidget_.reset();
    focusedWidget_ = nullptr;
    hoveredWidget_ = nullptr;
    mouseCaptureWidget_ = nullptr;
    needsRedraw_ = true;
    layoutWidth_ = 0;
    layoutHeight_ = 0;
    perfLogIntervalStats_ = {};
}

void UIContext::handleEvent(core::Event& event, const std::function<void()>& requestClose)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);

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
            if (hoveredWidget_ != nullptr) {
                const core::Rect hoveredBounds = hoveredWidget_->globalBounds();
                sendMouseCrossEvent(
                    core::EventType::MouseLeave,
                    hoveredWidget_,
                    hoveredBounds.centerX(),
                    hoveredBounds.centerY());
            }
            setFocus(nullptr);
            hoveredWidget_ = nullptr;
            mouseCaptureWidget_ = nullptr;
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

    if (focusedWidget_ != nullptr) {
        focusedWidget_->setFocused(false);
    }

    rootWidget_ = std::move(root);
    focusedWidget_ = nullptr;
    hoveredWidget_ = nullptr;
    mouseCaptureWidget_ = nullptr;
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

void UIContext::setOverlayWidget(std::shared_ptr<ui::Widget> overlay)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);

    if (hoveredWidget_ != nullptr) {
        const core::Rect hoveredBounds = hoveredWidget_->globalBounds();
        sendMouseCrossEvent(
            core::EventType::MouseLeave,
            hoveredWidget_,
            hoveredBounds.centerX(),
            hoveredBounds.centerY());
    }

    setFocus(nullptr);
    hoveredWidget_ = nullptr;
    mouseCaptureWidget_ = nullptr;
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
        static_cast<unsigned int>(theme.background),
        static_cast<unsigned int>(theme.primary));
}

ui::Theme UIContext::theme() const
{
    return runtimeState_ != nullptr
        ? runtimeState_->theme
        : ui::ThemeManager::instance().currentTheme();
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

bool UIContext::shouldRender() const
{
    return needsRedraw_
        || hasActiveAnimations()
        || (rootWidget_ != nullptr && rootWidget_->isDirty())
        || (overlayWidget_ != nullptr && overlayWidget_->isDirty());
}

bool UIContext::render(rendering::Canvas& canvas, int framebufferWidth, int framebufferHeight)
{
    ui::ScopedRuntimeState runtimeScope(*runtimeState_);
    if (!canvas || framebufferWidth <= 0 || framebufferHeight <= 0) {
        return true;
    }

    constexpr core::Color kClearColor = core::colorRGB(18, 20, 28);
    const bool fullRedraw = (rootWidget_ == nullptr && overlayWidget_ == nullptr)
        || (rootWidget_ != nullptr
            && (rootWidget_->isLayoutDirty() || !rootWidget_->hasDirtyRegion()))
        || (overlayWidget_ != nullptr
            && (overlayWidget_->isLayoutDirty() || !overlayWidget_->hasDirtyRegion()))
        || layoutWidth_ != framebufferWidth
        || layoutHeight_ != framebufferHeight;
    core::Rect redrawRegion = core::Rect::MakeEmpty();
    if (!fullRedraw) {
        if (rootWidget_ != nullptr && rootWidget_->hasDirtyRegion()) {
            redrawRegion = rootWidget_->dirtyRegion();
        }
        if (overlayWidget_ != nullptr && overlayWidget_->hasDirtyRegion()) {
            if (redrawRegion.isEmpty()) {
                redrawRegion = overlayWidget_->dirtyRegion();
            } else {
                redrawRegion.join(overlayWidget_->dirtyRegion());
            }
        }
    }
    if (debugHudConfig_.enabled) {
        const core::Rect hudBounds = debugHudBounds(framebufferWidth, framebufferHeight);
        if (redrawRegion.isEmpty()) {
            redrawRegion = hudBounds;
        } else {
            redrawRegion.join(hudBounds);
        }
    }

    if (fullRedraw) {
        canvas.clear(kClearColor);
    } else {
        canvas.clearRect(redrawRegion, kClearColor);
    }

    const auto fullBounds = core::Rect::MakeXYWH(
        0.0f,
        0.0f,
        static_cast<float>(framebufferWidth),
        static_cast<float>(framebufferHeight));
    const auto tightConstraints = ui::Constraints::tight(
        static_cast<float>(framebufferWidth),
        static_cast<float>(framebufferHeight));

    if (rootWidget_ != nullptr) {
        if (rootWidget_->isLayoutDirty()
            || layoutWidth_ != framebufferWidth
            || layoutHeight_ != framebufferHeight) {
            rootWidget_->measure(tightConstraints);
            rootWidget_->arrange(fullBounds);
        }
    }
    if (overlayWidget_ != nullptr) {
        if (overlayWidget_->isLayoutDirty()
            || layoutWidth_ != framebufferWidth
            || layoutHeight_ != framebufferHeight) {
            overlayWidget_->measure(tightConstraints);
            overlayWidget_->arrange(fullBounds);
        }
    }
    layoutWidth_ = framebufferWidth;
    layoutHeight_ = framebufferHeight;

    if (rootWidget_ != nullptr) {
        if (fullRedraw) {
            rootWidget_->draw(canvas);
        } else {
            canvas.save();
            canvas.clipRect(redrawRegion);
            rootWidget_->draw(canvas);
            canvas.restore();
        }
    }
    if (overlayWidget_ != nullptr) {
        if (fullRedraw) {
            overlayWidget_->draw(canvas);
        } else {
            canvas.save();
            canvas.clipRect(redrawRegion);
            overlayWidget_->draw(canvas);
            canvas.restore();
        }
    }

    if (debugHudConfig_.enabled) {
        drawDebugHud(canvas, framebufferWidth, framebufferHeight, fullRedraw, redrawRegion);
    }

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
        ui::Widget* target = hitTestTopLevel(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        if (target != hoveredWidget_) {
            sendMouseCrossEvent(
                core::EventType::MouseLeave,
                hoveredWidget_,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
            hoveredWidget_ = target;
            sendMouseCrossEvent(
                core::EventType::MouseEnter,
                hoveredWidget_,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }
        return;
    }
    case core::EventType::MouseLeave: {
        auto& mouseEvent = static_cast<core::MouseCrossEvent&>(event);
        sendMouseCrossEvent(
            core::EventType::MouseLeave,
            hoveredWidget_,
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        hoveredWidget_ = nullptr;
        return;
    }
    case core::EventType::MouseMove: {
        auto& mouseEvent = static_cast<core::MouseMoveEvent&>(event);
        ui::Widget* target = hitTestTopLevel(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));

        if (target != hoveredWidget_) {
            sendMouseCrossEvent(
                core::EventType::MouseLeave,
                hoveredWidget_,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
            hoveredWidget_ = target;
            sendMouseCrossEvent(
                core::EventType::MouseEnter,
                hoveredWidget_,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }

        ui::Widget* dispatchTarget = mouseCaptureWidget_ != nullptr
            ? mouseCaptureWidget_
            : target;
        dispatchAlongPath(event, dispatchTarget);
        return;
    }
    case core::EventType::MouseButtonPress: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        ui::Widget* target = hitTestTopLevel(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        mouseCaptureWidget_ = target;
        setFocus(findFocusableWidget(target));
        dispatchAlongPath(event, target);
        return;
    }
    case core::EventType::MouseButtonRelease: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        ui::Widget* target = mouseCaptureWidget_;
        if (target == nullptr) {
            target = hitTestTopLevel(
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }
        dispatchAlongPath(event, target);
        mouseCaptureWidget_ = nullptr;
        return;
    }
    case core::EventType::MouseScroll: {
        ui::Widget* target = hoveredWidget_ != nullptr
            ? hoveredWidget_
            : (overlayWidget_ != nullptr ? overlayWidget_.get() : rootWidget_.get());
        dispatchAlongPath(event, target);
        return;
    }
    default:
        break;
    }

    if (isKeyboardEvent(event.type()) && focusedWidget_ != nullptr) {
        dispatchAlongPath(event, focusedWidget_);
    }
}

void UIContext::setFocus(ui::Widget* widget)
{
    if (focusedWidget_ == widget) {
        return;
    }

    const ui::Widget* previousFocused = focusedWidget_;

    if (focusedWidget_ != nullptr) {
        focusedWidget_->setFocused(false);
    }

    focusedWidget_ = widget;
    if (focusedWidget_ != nullptr) {
        focusedWidget_->setFocused(true);
    }

    needsRedraw_ = true;
    core::logDebugCat(
        "app",
        "Focus changed from {} to {}",
        previousFocused != nullptr
            ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(previousFocused))
            : std::string("null"),
        focusedWidget_ != nullptr
            ? fmt::format("0x{:x}", reinterpret_cast<std::uintptr_t>(focusedWidget_))
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

    if (focusedWidget_ == nullptr) {
        setFocus(reverse ? focusables.back() : focusables.front());
        return;
    }

    const auto it = std::find(focusables.begin(), focusables.end(), focusedWidget_);
    if (it == focusables.end()) {
        setFocus(reverse ? focusables.back() : focusables.front());
        return;
    }

    const std::ptrdiff_t currentIndex = std::distance(focusables.begin(), it);
    const std::ptrdiff_t count = static_cast<std::ptrdiff_t>(focusables.size());
    const std::ptrdiff_t nextIndex = reverse
        ? (currentIndex - 1 + count) % count
        : (currentIndex + 1) % count;
    setFocus(focusables[static_cast<std::size_t>(nextIndex)]);
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

core::Rect UIContext::debugHudBounds(int framebufferWidth, int framebufferHeight) const
{
    const float scale = debugHudConfig_.scale;
    const float width = 280.0f * scale;
    const float height = 122.0f * scale;
    const float margin = 16.0f * scale;
    return core::Rect::MakeXYWH(
        std::max(0.0f, static_cast<float>(framebufferWidth) - width - margin),
        margin,
        width,
        std::min(height, std::max(0.0f, static_cast<float>(framebufferHeight) - margin * 2.0f)));
}

void UIContext::drawDebugHud(
    rendering::Canvas& canvas,
    int framebufferWidth,
    int framebufferHeight,
    bool fullRedraw,
    const core::Rect& dirtyRegion)
{
    if (!canvas) {
        return;
    }

    const DebugHudConfig config = debugHudConfig_;
    const core::Rect hudBounds = debugHudBounds(framebufferWidth, framebufferHeight);
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
