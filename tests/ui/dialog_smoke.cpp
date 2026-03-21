#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "../../src/ui/TextPrimitives.h"
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

tinalux::core::Point dialogCloseButtonCenter(
    tinalux::ui::Dialog& dialog,
    const tinalux::ui::DialogStyle& style,
    float hostWidth,
    float hostHeight)
{
    const tinalux::ui::TextMetrics titleMetrics = dialog.title().empty()
        ? tinalux::ui::TextMetrics {}
        : tinalux::ui::measureTextMetrics(dialog.title(), style.titleTextStyle.fontSize);
    const float closeButtonSide = std::max(18.0f, style.titleTextStyle.fontSize * 0.85f);
    const float headerBoxHeight = std::max(titleMetrics.height, closeButtonSide);
    const tinalux::core::Size dialogSize = dialog.measure(
        tinalux::ui::Constraints::loose(hostWidth, hostHeight));
    const float cardLeft = (hostWidth - dialogSize.width()) * 0.5f;
    const float cardTop = (hostHeight - dialogSize.height()) * 0.5f;
    return tinalux::core::Point::Make(
        cardLeft + dialogSize.width() - style.padding - closeButtonSide * 0.5f,
        cardTop + style.padding + (headerBoxHeight - closeButtonSide) * 0.5f
            + closeButtonSide * 0.5f);
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

    const core::Point closeButtonCenter = dialogCloseButtonCenter(
        *dialog,
        runtime.theme.dialogStyle,
        320.0f,
        200.0f);
    core::MouseButtonEvent closePress(
        core::mouse::kLeft,
        0,
        closeButtonCenter.x(),
        closeButtonCenter.y(),
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent closeRelease(
        core::mouse::kLeft,
        0,
        closeButtonCenter.x(),
        closeButtonCenter.y(),
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

    app.setOverlayWidget(nullptr);

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
