#include "tinalux/app/Application.h"
#include "tinalux/ui/DemoScene.h"

int main()
{
    tinalux::app::Application app;
    if (!app.init({
            .window = { .width = 1180, .height = 820, .title = "Tinalux" },
            .backend = tinalux::rendering::Backend::Auto,
        })) {
        return 1;
    }

    app.setTheme(tinalux::ui::Theme::dark());
    app.setRootWidget(app.buildWidgetTree([&app] {
        return tinalux::ui::createDemoScene(app.theme(), app.animationSink());
    }));
    return app.run();
}
