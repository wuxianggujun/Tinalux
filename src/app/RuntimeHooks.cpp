#include "RuntimeHooks.h"

namespace tinalux::app::detail {

namespace {

std::unique_ptr<platform::Window> defaultCreateWindow(const platform::WindowConfig& config)
{
    return platform::createWindow(config);
}

rendering::RenderContext defaultCreateContext(const rendering::ContextConfig& config)
{
    return rendering::createContext(config);
}

rendering::RenderSurface defaultCreateWindowSurface(
    rendering::RenderContext& context,
    platform::Window& window)
{
    return rendering::createWindowSurface(context, window);
}

std::chrono::steady_clock::time_point defaultNowSteadyTime()
{
    return std::chrono::steady_clock::now();
}

rendering::FramePrepareStatus defaultPrepareFrame(
    rendering::RenderContext& context,
    rendering::RenderSurface& surface)
{
    return rendering::prepareFrame(context, surface);
}

void defaultFlushFrame(rendering::RenderContext& context, rendering::RenderSurface& surface)
{
    rendering::flushFrame(context, surface);
}

RuntimeHooks withFallbackHooks(RuntimeHooks hooks, const RuntimeHooks& fallback)
{
    if (hooks.createWindow == nullptr) {
        hooks.createWindow = fallback.createWindow;
    }
    if (hooks.createContext == nullptr) {
        hooks.createContext = fallback.createContext;
    }
    if (hooks.createWindowSurface == nullptr) {
        hooks.createWindowSurface = fallback.createWindowSurface;
    }
    if (hooks.nowSteadyTime == nullptr) {
        hooks.nowSteadyTime = fallback.nowSteadyTime;
    }
    if (hooks.prepareFrame == nullptr) {
        hooks.prepareFrame = fallback.prepareFrame;
    }
    if (hooks.flushFrame == nullptr) {
        hooks.flushFrame = fallback.flushFrame;
    }
    return hooks;
}

}  // namespace

RuntimeHooks defaultRuntimeHooks()
{
    return RuntimeHooks {
        .createWindow = &defaultCreateWindow,
        .createContext = &defaultCreateContext,
        .createWindowSurface = &defaultCreateWindowSurface,
        .nowSteadyTime = &defaultNowSteadyTime,
        .prepareFrame = &defaultPrepareFrame,
        .flushFrame = &defaultFlushFrame,
    };
}

RuntimeHooks& runtimeHooks()
{
    static RuntimeHooks hooks = defaultRuntimeHooks();
    return hooks;
}

ScopedRuntimeHooksOverride::ScopedRuntimeHooksOverride(RuntimeHooks hooks)
    : previous_(runtimeHooks())
{
    runtimeHooks() = withFallbackHooks(hooks, previous_);
}

ScopedRuntimeHooksOverride::~ScopedRuntimeHooksOverride()
{
    runtimeHooks() = previous_;
}

}  // namespace tinalux::app::detail
