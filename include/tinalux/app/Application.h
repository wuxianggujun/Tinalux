#pragma once

#include <cstdint>
#include <memory>
#include "tinalux/ui/Theme.h"
#include "tinalux/platform/Window.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::ui {
class AnimationSink;
class Container;
class Widget;
}

namespace tinalux::app {

struct FrameStats {
    std::uint64_t totalFrames = 0;
    std::uint64_t fullRedrawFrames = 0;
    std::uint64_t partialRedrawFrames = 0;
    std::uint64_t skippedFrames = 0;
    std::uint64_t waitEventLoops = 0;
    std::uint64_t pollEventLoops = 0;
    double lastFrameMs = 0.0;
    double averageFrameMs = 0.0;
    double maxFrameMs = 0.0;
};

struct PerfLogConfig {
    bool enabled = false;
    std::uint64_t frameInterval = 120;
};

struct DebugHudConfig {
    bool enabled = false;
    bool highlightDirtyRegion = true;
    float scale = 1.0f;
};

class Application final {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool init(const platform::WindowConfig& config = {});
    int run();
    void shutdown();

    void handleEvent(core::Event& event);
    void setRootWidget(std::shared_ptr<ui::Widget> root);
    void setOverlayWidget(std::shared_ptr<ui::Widget> overlay);
    void clearOverlayWidget();
    platform::Window* window() const;
    ui::AnimationSink& animationSink();
    void requestClose();
    FrameStats frameStats() const;
    void resetFrameStats();
    void setTheme(ui::Theme theme);
    ui::Theme theme() const;
    void setPerfLogConfig(PerfLogConfig config);
    PerfLogConfig perfLogConfig() const;
    void setDebugHudConfig(DebugHudConfig config);
    DebugHudConfig debugHudConfig() const;

private:
    struct Impl;
    bool renderFrame();

    std::unique_ptr<Impl> impl_;
};

}  // namespace tinalux::app
