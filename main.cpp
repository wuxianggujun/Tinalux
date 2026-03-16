#include "tinalux/app/Application.h"
#include "tinalux/ui/DemoScene.h"

int main()
{
    tinalux::app::Application app;
    if (!app.init({
            .window = { .width = 960, .height = 640, .title = "Tinalux" },
            .backend = tinalux::rendering::Backend::Auto,
        })) {
        return 1;
    }

    const tinalux::ui::Theme theme = tinalux::ui::Theme::dark();
    app.setTheme(theme);
    app.setRootWidget(tinalux::ui::createDemoScene(theme, app.animationSink()));
    return app.run();
}
