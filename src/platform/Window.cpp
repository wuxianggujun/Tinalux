#include "tinalux/platform/Window.h"
#include "tinalux/core/Log.h"

#if defined(TINALUX_PLATFORM_GLFW)
#include "glfw/GLFWWindow.h"
#elif defined(TINALUX_PLATFORM_ANDROID)
#include "android/AndroidWindow.h"
#endif

namespace tinalux::platform {

std::unique_ptr<Window> createWindow(const WindowConfig& config)
{
#if defined(TINALUX_PLATFORM_GLFW)
    core::logDebugCat(
        "platform",
        "Creating GLFW window title='{}' size={}x{} resizable={} vsync={}",
        config.title != nullptr ? config.title : "Tinalux",
        config.width,
        config.height,
        config.resizable,
        config.vsync);
    auto window = std::make_unique<GLFWWindow>(config);
    if (!window->valid()) {
        core::logErrorCat("platform", "GLFW window creation returned invalid window");
        return nullptr;
    }
    core::logInfoCat(
        "platform",
        "Created GLFW window size={}x{} framebuffer={}x{} dpi_scale={:.2f}",
        window->width(),
        window->height(),
        window->framebufferWidth(),
        window->framebufferHeight(),
        window->dpiScale());
    return window;
#elif defined(TINALUX_PLATFORM_ANDROID)
    core::logDebugCat(
        "platform",
        "Creating Android window title='{}' size={}x{} resizable={} vsync={} graphics_api={}",
        config.title != nullptr ? config.title : "Tinalux",
        config.width,
        config.height,
        config.resizable,
        config.vsync,
        config.graphicsApi == GraphicsAPI::OpenGL ? "OpenGL" : "None");
    auto window = std::make_unique<AndroidWindow>(config);
    if (!window->valid()) {
        core::logErrorCat("platform", "Android window creation returned invalid window");
        return nullptr;
    }
    return window;
#else
    (void)config;
    core::logErrorCat("platform", "No platform window backend is enabled at build time");
    return nullptr;
#endif
}

}  // namespace tinalux::platform
