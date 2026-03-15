#pragma once

#include <cstdint>
#include <functional>
#include <memory>
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

    void handleEvent(core::Event& event, const std::function<void()>& requestClose);
    void setRootWidget(std::shared_ptr<ui::Widget> root);
    void setOverlayWidget(std::shared_ptr<ui::Widget> overlay);
    void clearOverlayWidget();
    ui::AnimationSink& animationSink();

    FrameStats frameStats() const;
    void resetFrameStats();

    void setTheme(ui::Theme theme);
    ui::Theme theme() const;

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
    bool hasActiveAnimations() const;

    bool shouldRender() const;
    bool render(rendering::Canvas& canvas, int framebufferWidth, int framebufferHeight);
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
    void setFocus(ui::Widget* widget);
    void advanceFocus(bool reverse = false);
    void maybeLogPeriodicPerfSummary();
    void logPeriodicPerfSummary(const char* reason);
    core::Rect debugHudBounds(int framebufferWidth, int framebufferHeight) const;
    void drawDebugHud(
        rendering::Canvas& canvas,
        int framebufferWidth,
        int framebufferHeight,
        bool fullRedraw,
        const core::Rect& dirtyRegion);

    std::shared_ptr<ui::Widget> rootWidget_;
    std::shared_ptr<ui::Widget> overlayWidget_;
    ui::Widget* focusedWidget_ = nullptr;
    ui::Widget* hoveredWidget_ = nullptr;
    ui::Widget* mouseCaptureWidget_ = nullptr;
    bool needsRedraw_ = true;
    int layoutWidth_ = 0;
    int layoutHeight_ = 0;
    FrameStats frameStats_ {};
    PerfLogConfig perfLogConfig_ {};
    PerfLogIntervalStats perfLogIntervalStats_ {};
    bool perfLogConfiguredExplicitly_ = false;
    DebugHudConfig debugHudConfig_ {};
    bool debugHudConfiguredExplicitly_ = false;
    std::uint64_t themeListenerId_ = 0;
    std::unique_ptr<ui::RuntimeState> runtimeState_;
};

}  // namespace tinalux::app
