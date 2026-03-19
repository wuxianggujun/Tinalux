#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/Geometry.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
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
    return std::abs(lhs - rhs) <= 0.05f;
}

}  // namespace

int main()
{
    using namespace tinalux;

    ui::Theme mobile = ui::Theme::mobile(1.25f);
    expect(nearlyEqual(mobile.fontScale, 1.25f), "mobile theme should preserve the requested font scale");
    expect(
        nearlyEqual(mobile.minimumTouchTargetSize, 48.0f),
        "mobile theme should reserve a 48dp touch target floor");
    expect(mobile.buttonStyle.minHeight >= 48.0f, "mobile button style should honor the touch target minimum");
    expect(mobile.textInputStyle.minWidth < 0.0f, "mobile text input should opt into fill-width layout");
    expect(mobile.textInputStyle.minHeight >= 48.0f, "mobile text input should honor the touch target minimum");
    expect(mobile.sliderStyle.preferredWidth < 0.0f, "mobile slider should opt into fill-width layout");
    expect(mobile.sliderStyle.preferredHeight >= 48.0f, "mobile slider should honor the touch target minimum");
    expect(mobile.checkboxStyle.minHeight >= 48.0f, "mobile checkbox should honor the touch target minimum");
    expect(mobile.radioStyle.minHeight >= 48.0f, "mobile radio should honor the touch target minimum");
    expect(mobile.toggleStyle.minHeight >= 48.0f, "mobile toggle should honor the touch target minimum");

    ui::Theme accessibleMobile = ui::Theme::mobile(2.0f);
    expect(
        nearlyEqual(accessibleMobile.typography.body1.fontSize, 32.0f),
        "mobile body typography should preserve accessibility scaling");
    expect(
        nearlyEqual(accessibleMobile.typography.h1.fontSize, 47.6f),
        "mobile headings should clamp extreme scaling to protect narrow layouts");

    ui::RuntimeState runtime;
    runtime.theme = mobile;
    ui::ScopedRuntimeState bind(runtime);

    ui::Button button("Tap");
    button.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 30.0f, 30.0f));
    expect(
        button.hitTest(-8.0f, 15.0f) == &button,
        "mobile touch target expansion should keep small buttons tappable");
    expect(
        button.hitTest(-12.0f, 15.0f) == nullptr,
        "mobile touch target expansion should remain bounded around the widget");

    ui::Slider slider;
    const core::Size sliderSize = slider.measure(ui::Constraints::loose(300.0f, 100.0f));
    expect(nearlyEqual(sliderSize.width(), 300.0f), "mobile slider should fill the available width");
    expect(sliderSize.height() >= 48.0f, "mobile slider should keep a touch-friendly height");

    ui::TextInput textInput("Search");
    const core::Size textInputSize = textInput.measure(ui::Constraints::loose(320.0f, 100.0f));
    expect(nearlyEqual(textInputSize.width(), 320.0f), "mobile text input should fill the available width");
    expect(textInputSize.height() >= 48.0f, "mobile text input should keep a touch-friendly height");

    ui::Dropdown dropdown({ "One", "Two" });
    const core::Size dropdownSize = dropdown.measure(ui::Constraints::loose(220.0f, 200.0f));
    expect(dropdownSize.height() >= 48.0f, "mobile dropdown should keep a touch-friendly row height");

    ui::ProgressBar progressBar;
    progressBar.setPreferredWidth(-1.0f);
    const core::Size progressBarSize = progressBar.measure(ui::Constraints::loose(280.0f, 100.0f));
    expect(nearlyEqual(progressBarSize.width(), 280.0f), "progress bar should fill the available width when requested");

    runtime.theme = ui::Theme::dark();
    button.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 30.0f, 30.0f));
    expect(
        button.hitTest(-1.0f, 15.0f) == nullptr,
        "desktop theme should not silently expand hit targets");
    return 0;
}
