#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

class ProbeDialog final : public tinalux::ui::Dialog {
public:
    using Dialog::Dialog;

    tinalux::ui::DialogStyle effectiveStyle() const
    {
        return resolvedStyle();
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::light();
    ui::ScopedRuntimeState bind(runtime);

    auto button = std::make_shared<ui::Button>("Accept");
    ProbeDialog dialog("Confirm Action");
    dialog.setContent(button);

    const core::Size defaultSize = dialog.measure(ui::Constraints::loose(640.0f, 480.0f));
    expect(
        nearlyEqual(dialog.effectiveStyle().padding, runtime.theme.dialogStyle.padding),
        "dialog should use theme dialog padding by default");

    ui::DialogStyle customStyle = runtime.theme.dialogStyle;
    customStyle.padding = runtime.theme.dialogStyle.padding + 16.0f;
    customStyle.titleGap = runtime.theme.dialogStyle.titleGap + 10.0f;
    customStyle.cornerRadius = runtime.theme.dialogStyle.cornerRadius + 6.0f;
    customStyle.titleTextStyle.fontSize = runtime.theme.dialogStyle.titleTextStyle.fontSize + 8.0f;
    customStyle.maxSize = core::Size::Make(420.0f, 360.0f);
    dialog.setStyle(customStyle);
    const core::Size customSize = dialog.measure(ui::Constraints::loose(640.0f, 480.0f));
    expect(dialog.style() != nullptr, "setStyle should install per-dialog style override");
    expect(customSize.width() > defaultSize.width(), "custom dialog padding should enlarge measured width");
    expect(customSize.height() > defaultSize.height(), "custom dialog title spacing should enlarge measured height");

    dialog.setShowCloseButton(false);
    const core::Size hiddenCloseButtonSize = dialog.measure(ui::Constraints::loose(640.0f, 480.0f));
    expect(!dialog.showCloseButton(), "dialog should expose hidden close button state");
    expect(
        hiddenCloseButtonSize.width() <= customSize.width(),
        "hiding dialog close button should not increase measured width");
    dialog.setShowCloseButton(true);
    expect(dialog.showCloseButton(), "dialog should expose visible close button state");

    dialog.setPadding(52.0f);
    dialog.setCornerRadius(30.0f);
    dialog.setBackdropColor(core::colorARGB(200, 1, 2, 3));
    dialog.setBackgroundColor(core::colorRGB(20, 30, 40));
    dialog.setMaxSize(core::Size::Make(360.0f, 280.0f));
    const ui::DialogStyle resolvedAfterSetter = dialog.effectiveStyle();
    expect(nearlyEqual(resolvedAfterSetter.padding, 52.0f), "setter padding should override installed style");
    expect(nearlyEqual(resolvedAfterSetter.cornerRadius, 30.0f), "setter corner radius should override installed style");
    expect(
        resolvedAfterSetter.backdropColor == core::colorARGB(200, 1, 2, 3),
        "setter backdrop color should override installed style");
    expect(
        resolvedAfterSetter.backgroundColor == core::colorRGB(20, 30, 40),
        "setter background color should override installed style");
    expect(
        resolvedAfterSetter.maxSize == core::Size::Make(360.0f, 280.0f),
        "setter maxSize should override installed style");

    dialog.clearStyle();
    expect(dialog.style() == nullptr, "clearStyle should remove per-dialog style override");
    const ui::DialogStyle resolvedAfterClear = dialog.effectiveStyle();
    expect(nearlyEqual(resolvedAfterClear.padding, 52.0f), "setter padding should persist after clearing style");
    expect(
        resolvedAfterClear.backgroundColor == core::colorRGB(20, 30, 40),
        "setter background override should persist after clearing style");

    ProbeDialog themedDialog("Runtime themed");
    themedDialog.setContent(std::make_shared<ui::Button>("Continue"));
    runtime.theme = ui::Theme::dark();
    runtime.theme.dialogStyle.padding = 28.0f;
    runtime.theme.dialogStyle.titleGap = 18.0f;
    runtime.theme.dialogStyle.cornerRadius = 24.0f;
    runtime.theme.dialogStyle.titleColor = runtime.theme.colors.primary;
    const ui::DialogStyle themedStyle = themedDialog.effectiveStyle();
    expect(nearlyEqual(themedStyle.padding, 28.0f), "dialog should observe runtime theme padding");
    expect(nearlyEqual(themedStyle.titleGap, 18.0f), "dialog should observe runtime theme title gap");
    expect(nearlyEqual(themedStyle.cornerRadius, 24.0f), "dialog should observe runtime theme corner radius");
    expect(
        themedStyle.titleColor == runtime.theme.colors.primary,
        "dialog should observe runtime theme title color");

    const rendering::RenderSurface surface = rendering::createRasterSurface(360, 220);
    expect(static_cast<bool>(surface), "dialog style smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    themedDialog.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));
    themedDialog.draw(canvas);

    return 0;
}
