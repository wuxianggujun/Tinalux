#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Panel.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using namespace tinalux;
    ui::RuntimeState runtime;
    ui::ScopedRuntimeState bind(runtime);

    int rootClicks = 0;
    int dialogClicks = 0;
    int dismissCount = 0;

    auto root = std::make_shared<ui::Panel>();
    auto rootButton = std::make_shared<ui::Button>("Root");
    rootButton->onClick([&rootClicks] { ++rootClicks; });
    root->addChild(rootButton);
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 320.0f, 200.0f));
    rootButton->arrange(core::Rect::MakeXYWH(20.0f, 20.0f, 96.0f, 40.0f));

    auto dialog = std::make_shared<ui::Dialog>("Confirm Action");
    auto dialogButton = std::make_shared<ui::Button>("Accept");
    dialogButton->onClick([&dialogClicks] { ++dialogClicks; });
    dialog->setContent(dialogButton);
    dialog->onDismiss([&dismissCount] { ++dismissCount; });
    dialog->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 320.0f, 200.0f));

    app::Application app;
    app.setRootWidget(root);
    app.setOverlayWidget(dialog);

    core::KeyEvent tabForward(
        core::keys::kTab,
        0,
        0,
        core::EventType::KeyPress);
    app.handleEvent(tabForward);
    expect(dialogButton->focused(), "dialog child should receive focus before root widgets");

    core::KeyEvent enterKey(
        core::keys::kEnter,
        0,
        0,
        core::EventType::KeyPress);
    app.handleEvent(enterKey);
    expect(dialogClicks == 1, "Enter on focused dialog button should trigger click");

    const core::Size dialogSize = dialog->measure(ui::Constraints::loose(320.0f, 200.0f));
    const double closeX = (320.0 - dialogSize.width()) * 0.5 + dialogSize.width() - 24.0;
    const double closeY = (200.0 - dialogSize.height()) * 0.5 + 24.0;
    core::MouseButtonEvent closePress(
        core::mouse::kLeft,
        0,
        closeX,
        closeY,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent closeRelease(
        core::mouse::kLeft,
        0,
        closeX,
        closeY,
        core::EventType::MouseButtonRelease);
    app.handleEvent(closePress);
    app.handleEvent(closeRelease);
    expect(dismissCount == 1, "close button click should dismiss dialog");

    core::MouseButtonEvent backdropPress(
        core::mouse::kLeft,
        0,
        8.0,
        8.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent backdropRelease(
        core::mouse::kLeft,
        0,
        8.0,
        8.0,
        core::EventType::MouseButtonRelease);
    app.handleEvent(backdropPress);
    app.handleEvent(backdropRelease);
    expect(dismissCount == 2, "backdrop click should dismiss dialog");

    app.clearOverlayWidget();

    const core::Rect rootButtonBounds = rootButton->globalBounds();
    core::MouseButtonEvent rootPress(
        core::mouse::kLeft,
        0,
        rootButtonBounds.centerX(),
        rootButtonBounds.centerY(),
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent rootRelease(
        core::mouse::kLeft,
        0,
        rootButtonBounds.centerX(),
        rootButtonBounds.centerY(),
        core::EventType::MouseButtonRelease);
    app.handleEvent(rootPress);
    app.handleEvent(rootRelease);
    expect(rootClicks == 1, "clearing overlay should restore root interaction");

    const rendering::RenderSurface surface = rendering::createRasterSurface(320, 200);
    expect(static_cast<bool>(surface), "dialog smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dialog smoke should expose raster canvas");
    dialog->draw(canvas);

    return 0;
}
