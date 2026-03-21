#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/platform/Window.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::ui {
class Container;
class Widget;
}

namespace tinalux::app::detail {
struct ApplicationTestAccess;
}

namespace tinalux::app {

struct FrameStats {
    std::uint64_t totalFrames = 0;
    std::uint64_t fullRedrawFrames = 0;
    std::uint64_t partialRedrawFrames = 0;
    std::uint64_t deferredFrames = 0;
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

struct ApplicationConfig {
    platform::WindowConfig window {};
    rendering::Backend backend = rendering::Backend::Auto;
};

class Application final {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool init(const ApplicationConfig& config = {});
    int run();
    bool pumpOnce();
    void suspendRendering();
    bool resumeRendering(const platform::WindowConfig& windowConfig);
    bool renderingReady() const;
    void shutdown();

    void handleEvent(core::Event& event);
    std::shared_ptr<ui::Widget> buildWidgetTree(
        const std::function<std::shared_ptr<ui::Widget>()>& builder);
    void setRootWidget(std::shared_ptr<ui::Widget> root);
    void setOverlayWidget(std::shared_ptr<ui::Widget> overlay);
    platform::Window* window() const;
    void requestClose();
    FrameStats frameStats() const;
    void setTheme(ui::Theme theme);
    ui::Theme theme() const;
    void setPerfLogConfig(PerfLogConfig config);
    PerfLogConfig perfLogConfig() const;
    void setDebugHudConfig(DebugHudConfig config);
    DebugHudConfig debugHudConfig() const;
    rendering::Backend renderBackend() const;

private:
    friend struct detail::ApplicationTestAccess;

    struct Impl;
    bool renderFrame();
    bool tryInitializeBackend(
        const platform::WindowConfig& windowConfig,
        rendering::Backend backend,
        std::size_t backendIndex);
    bool tryPromoteNextBackend();
    platform::WindowConfig currentWindowConfigForRecovery() const;
    void resetRenderState();
    void syncTextInputState();

    std::unique_ptr<Impl> impl_;
};

}  // namespace tinalux::app
