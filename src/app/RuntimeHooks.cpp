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

}  // namespace

RuntimeHooks defaultRuntimeHooks()
{
    return RuntimeHooks {
        .createWindow = &defaultCreateWindow,
        .createContext = &defaultCreateContext,
        .createWindowSurface = &defaultCreateWindowSurface,
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
    runtimeHooks() = hooks;
}

ScopedRuntimeHooksOverride::~ScopedRuntimeHooksOverride()
{
    runtimeHooks() = previous_;
}

}  // namespace tinalux::app::detail
