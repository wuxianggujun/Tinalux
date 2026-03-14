#include "tinalux/platform/Window.h"

#if defined(TINALUX_PLATFORM_GLFW)
#include "glfw/GLFWWindow.h"
#endif

namespace tinalux::platform {

std::unique_ptr<Window> createWindow(const WindowConfig& config)
{
#if defined(TINALUX_PLATFORM_GLFW)
    auto window = std::make_unique<GLFWWindow>(config);
    if (!window->valid()) {
        return nullptr;
    }
    return window;
#else
    (void)config;
    return nullptr;
#endif
}

}  // namespace tinalux::platform

