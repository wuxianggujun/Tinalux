#pragma once

#include "tinalux/app/Application.h"

namespace tinalux::app::detail {

struct ApplicationTestAccess {
    static bool renderFrame(Application& app)
    {
        return app.renderFrame();
    }

    static platform::Window* window(Application& app)
    {
        return app.platformWindow();
    }

    static FrameStats frameStats(const Application& app)
    {
        return app.currentFrameStats();
    }

    static PerfLogConfig perfLogConfig(const Application& app)
    {
        return app.currentPerfLogConfig();
    }

    static DebugHudConfig debugHudConfig(const Application& app)
    {
        return app.currentDebugHudConfig();
    }
};

}  // namespace tinalux::app::detail
