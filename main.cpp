#include <cstdlib>
#include <string_view>

#include "tinalux/app/Application.h"
#include "tinalux/ui/DemoScene.h"

namespace {

tinalux::rendering::Backend resolveDesktopBackend()
{
    const char* raw = std::getenv("TINALUX_DESKTOP_BACKEND");
    if (raw == nullptr) {
        return tinalux::rendering::Backend::Auto;
    }

    const std::string_view value(raw);
    if (value == "opengl" || value == "OpenGL" || value == "OPENGL") {
        return tinalux::rendering::Backend::OpenGL;
    }
    if (value == "vulkan" || value == "Vulkan" || value == "VULKAN") {
        return tinalux::rendering::Backend::Vulkan;
    }
    return tinalux::rendering::Backend::Auto;
}

}  // namespace

int main()
{
    tinalux::app::Application app;
    if (!app.init({
            .window = { .width = 1180, .height = 820, .title = "Tinalux" },
            .backend = resolveDesktopBackend(),
        })) {
        return 1;
    }

    const tinalux::ui::Theme desktopTheme = tinalux::ui::Theme::dark();
    app.setTheme(desktopTheme);
    app.setRootWidget(app.buildWidgetTree([desktopTheme] {
        return tinalux::ui::createDemoScene(desktopTheme);
    }));
    return app.run();
}
