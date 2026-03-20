#pragma once

#include <chrono>
#include <memory>

#include "../rendering/FrameLifecycle.h"
#include "tinalux/platform/Window.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::app::detail {

using CreateWindowFn = std::unique_ptr<platform::Window> (*)(const platform::WindowConfig& config);
using CreateContextFn = rendering::RenderContext (*)(const rendering::ContextConfig& config);
using CreateWindowSurfaceFn =
    rendering::RenderSurface (*)(rendering::RenderContext& context, platform::Window& window);
using NowSteadyTimeFn = std::chrono::steady_clock::time_point (*)();
using PrepareFrameFn =
    rendering::FramePrepareStatus (*)(rendering::RenderContext& context, rendering::RenderSurface& surface);
using FlushFrameFn = void (*)(rendering::RenderContext& context, rendering::RenderSurface& surface);

struct RuntimeHooks {
    CreateWindowFn createWindow = nullptr;
    CreateContextFn createContext = nullptr;
    CreateWindowSurfaceFn createWindowSurface = nullptr;
    NowSteadyTimeFn nowSteadyTime = nullptr;
    PrepareFrameFn prepareFrame = nullptr;
    FlushFrameFn flushFrame = nullptr;
};

RuntimeHooks defaultRuntimeHooks();
RuntimeHooks& runtimeHooks();

class ScopedRuntimeHooksOverride {
public:
    explicit ScopedRuntimeHooksOverride(RuntimeHooks hooks);
    ~ScopedRuntimeHooksOverride();

    ScopedRuntimeHooksOverride(const ScopedRuntimeHooksOverride&) = delete;
    ScopedRuntimeHooksOverride& operator=(const ScopedRuntimeHooksOverride&) = delete;

private:
    RuntimeHooks previous_;
};

}  // namespace tinalux::app::detail
