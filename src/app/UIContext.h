#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "tinalux/core/Geometry.h"
#include "tinalux/app/Application.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::ui {
class AnimationSink;
struct RuntimeState;
class Widget;
}

namespace tinalux::app {

class UIContext final {
public:
    UIContext();
    ~UIContext();

    void initializeFromEnvironment();
    void shutdown();

    void handleEvent(
        core::Event& event,
        const std::function<void()>& requestClose,
        float dpiScale = 1.0f);
    std::shared_ptr<ui::Widget> buildWidgetTree(
        const std::function<std::shared_ptr<ui::Widget>()>& builder);
    void setRootWidget(std::shared_ptr<ui::Widget> root);
    void setOverlayWidget(std::shared_ptr<ui::Widget> overlay);
    void clearOverlayWidget();
    ui::AnimationSink& animationSink();
    bool textInputActive();
    std::optional<core::Rect> imeCursorRect();

    FrameStats frameStats() const;
    void resetFrameStats();

    void setTheme(ui::Theme theme);
    ui::Theme theme() const;
    void setPartialRedrawEnabled(bool enabled);
    bool partialRedrawEnabled() const;

    void setPerfLogConfig(PerfLogConfig config);
    PerfLogConfig perfLogConfig() const;

    void setDebugHudConfig(DebugHudConfig config);
    DebugHudConfig debugHudConfig() const;

    void noteWaitLoop();
    void notePollLoop();
    void noteAnimationTickUpdated();
    void noteFrameRendered(bool fullRedraw, double frameMs);
    void noteFrameSkipped();
    bool tickAnimations(double nowSeconds);
    bool tickAsyncResources();
    bool hasActiveAnimations() const;
    std::optional<double> nextAnimationDelaySeconds(double nowSeconds) const;

    bool shouldRender() const;
    bool hasImmediateRenderWork() const;
    void notifyWindowMetricsChanged();
    bool render(
        rendering::Canvas& canvas,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale = 1.0f);
    void clearNeedsRedraw();

private:
    struct PerfLogIntervalStats {
        std::uint64_t renderedFrames = 0;
        std::uint64_t fullRedrawFrames = 0;
        std::uint64_t partialRedrawFrames = 0;
        std::uint64_t skippedFrames = 0;
        std::uint64_t waitEventLoops = 0;
        std::uint64_t pollEventLoops = 0;
        double totalFrameMs = 0.0;
        double lastFrameMs = 0.0;
        double maxFrameMs = 0.0;
    };

    void dispatchEvent(core::Event& event);
    ui::Widget* hitTestTopLevel(float x, float y) const;
    void collectActiveFocusables(std::vector<ui::Widget*>& out) const;
    void setFocus(const std::shared_ptr<ui::Widget>& widget);
    void advanceFocus(bool reverse = false);
    void maybeLogPeriodicPerfSummary();
    void logPeriodicPerfSummary(const char* reason);
    void bindThemeRuntime();
    void unbindThemeRuntime();
    void resetForStartup();
    core::Rect debugHudBounds(float logicalWidth, float logicalHeight) const;
    void drawDebugHud(
        rendering::Canvas& canvas,
        float logicalWidth,
        float logicalHeight,
        bool fullRedraw,
        const core::Rect& dirtyRegion);
    bool isInActiveTree(const ui::Widget* widget) const;
    std::shared_ptr<ui::Widget> lockActiveWidget(ui::Widget* widget) const;
    std::shared_ptr<ui::Widget> lockWidget(
        const std::weak_ptr<ui::Widget>& widget) const;

    std::shared_ptr<ui::Widget> rootWidget_;
    std::shared_ptr<ui::Widget> overlayWidget_;
    std::weak_ptr<ui::Widget> focusedWidget_;
    std::weak_ptr<ui::Widget> hoveredWidget_;
    std::weak_ptr<ui::Widget> mouseCaptureWidget_;
    bool needsRedraw_ = true;
    float layoutWidth_ = 0.0f;
    float layoutHeight_ = 0.0f;
    float lastDpiScale_ = 1.0f;
    FrameStats frameStats_ {};
    PerfLogConfig perfLogConfig_ {};
    PerfLogIntervalStats perfLogIntervalStats_ {};
    bool perfLogConfiguredExplicitly_ = false;
    DebugHudConfig debugHudConfig_ {};
    bool debugHudConfiguredExplicitly_ = false;
    std::uint64_t themeListenerId_ = 0;
    std::uint64_t themeRuntimeBindingId_ = 0;
    bool loggedEmptyScene_ = false;
    bool loggedSceneReady_ = false;
    bool partialRedrawEnabled_ = true;
    bool shutdown_ = false;
    std::unique_ptr<ui::RuntimeState> runtimeState_;
};

}  // namespace tinalux::app
