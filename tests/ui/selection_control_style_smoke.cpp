#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Toggle.h"

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

bool nearlyEqualColor(tinalux::core::Color lhs, tinalux::core::Color rhs, int tolerance = 2)
{
    const auto channelClose = [tolerance](tinalux::core::Color::Channel a, tinalux::core::Color::Channel b) {
        return std::abs(static_cast<int>(a) - static_cast<int>(b)) <= tolerance;
    };

    return channelClose(lhs.alpha(), rhs.alpha())
        && channelClose(lhs.red(), rhs.red())
        && channelClose(lhs.green(), rhs.green())
        && channelClose(lhs.blue(), rhs.blue());
}

tinalux::core::Color readPixel(
    const tinalux::rendering::RenderSurface& surface,
    int x,
    int y)
{
    const tinalux::rendering::Image snapshot = surface.snapshotImage();
    auto* skiaImage = tinalux::rendering::RenderAccess::skiaImage(snapshot);
    expect(skiaImage != nullptr, "snapshot image should expose native pixel access");

    std::vector<std::uint8_t> pixel(4, 0);
    const bool read = skiaImage->readPixels(
        SkImageInfo::Make(1, 1, kRGBA_8888_SkColorType, kPremul_SkAlphaType),
        pixel.data(),
        4,
        x,
        y);
    expect(read, "snapshot pixel read should succeed");
    return tinalux::core::colorARGB(pixel[3], pixel[0], pixel[1], pixel[2]);
}

class ProbeCheckbox final : public tinalux::ui::Checkbox {
public:
    using Checkbox::Checkbox;

    const tinalux::ui::CheckboxStyle& effectiveStyle() const
    {
        return resolvedStyle();
    }
};

class ProbeRadio final : public tinalux::ui::Radio {
public:
    using Radio::Radio;

    const tinalux::ui::RadioStyle& effectiveStyle() const
    {
        return resolvedStyle();
    }
};

class ProbeToggle final : public tinalux::ui::Toggle {
public:
    using Toggle::Toggle;

    const tinalux::ui::ToggleStyle& effectiveStyle() const
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

    ProbeCheckbox checkbox("Remember me");
    const core::Size checkboxDefault = checkbox.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(
        nearlyEqual(
            checkbox.effectiveStyle().indicatorSize,
            runtime.theme.checkboxStyle.indicatorSize),
        "checkbox should use theme checkbox style by default");

    ui::CheckboxStyle customCheckbox = runtime.theme.checkboxStyle;
    customCheckbox.indicatorSize = 30.0f;
    customCheckbox.labelGap = 20.0f;
    customCheckbox.minHeight = checkboxDefault.height() + 12.0f;
    checkbox.setStyle(customCheckbox);
    const core::Size checkboxCustom = checkbox.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(checkbox.style() != nullptr, "checkbox should keep installed custom style");
    expect(checkboxCustom.width() > checkboxDefault.width(), "checkbox custom indicator and gap should enlarge width");
    expect(checkboxCustom.height() >= customCheckbox.minHeight, "checkbox custom minHeight should be respected");
    checkbox.clearStyle();
    expect(checkbox.style() == nullptr, "checkbox clearStyle should remove override");

    ProbeRadio radio("Strict review", "group-a", false);
    const core::Size radioDefault = radio.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(
        nearlyEqual(
            radio.effectiveStyle().indicatorSize,
            runtime.theme.radioStyle.indicatorSize),
        "radio should use theme radio style by default");

    ui::RadioStyle customRadio = runtime.theme.radioStyle;
    customRadio.indicatorSize = 32.0f;
    customRadio.labelGap = 18.0f;
    customRadio.minHeight = radioDefault.height() + 10.0f;
    radio.setStyle(customRadio);
    const core::Size radioCustom = radio.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(radio.style() != nullptr, "radio should keep installed custom style");
    expect(radioCustom.width() > radioDefault.width(), "radio custom indicator and gap should enlarge width");
    expect(radioCustom.height() >= customRadio.minHeight, "radio custom minHeight should be respected");
    radio.clearStyle();
    expect(radio.style() == nullptr, "radio clearStyle should remove override");

    ProbeToggle toggle("Auto refresh activity feed");
    const core::Size toggleDefault = toggle.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(
        nearlyEqual(
            toggle.effectiveStyle().trackWidth,
            runtime.theme.toggleStyle.trackWidth),
        "toggle should use theme toggle style by default");

    ui::ToggleStyle customToggle = runtime.theme.toggleStyle;
    customToggle.trackWidth = runtime.theme.toggleStyle.trackWidth + 18.0f;
    customToggle.labelGap = runtime.theme.toggleStyle.labelGap + 8.0f;
    customToggle.minHeight = toggleDefault.height() + 10.0f;
    toggle.setStyle(customToggle);
    const core::Size toggleCustom = toggle.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(toggle.style() != nullptr, "toggle should keep installed custom style");
    expect(toggleCustom.width() > toggleDefault.width(), "toggle custom track and gap should enlarge width");
    expect(toggleCustom.height() >= customToggle.minHeight, "toggle custom minHeight should be respected");
    toggle.clearStyle();
    expect(toggle.style() == nullptr, "toggle clearStyle should remove override");

    runtime.theme = ui::Theme::dark();
    runtime.theme.checkboxStyle.labelGap = 24.0f;
    runtime.theme.radioStyle.innerDotSize = 14.0f;
    runtime.theme.toggleStyle.trackWidth = 60.0f;

    expect(
        nearlyEqual(checkbox.effectiveStyle().labelGap, 24.0f),
        "checkbox should observe runtime theme checkbox style changes");
    expect(
        nearlyEqual(radio.effectiveStyle().innerDotSize, 14.0f),
        "radio should observe runtime theme radio style changes");
    expect(
        nearlyEqual(toggle.effectiveStyle().trackWidth, 60.0f),
        "toggle should observe runtime theme toggle style changes");

    const rendering::RenderSurface surface = rendering::createRasterSurface(96, 96);
    expect(static_cast<bool>(surface), "selection control animation test should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "selection control animation test should expose canvas");

    ui::CheckboxStyle animatedCheckboxStyle = runtime.theme.checkboxStyle;
    animatedCheckboxStyle.indicatorSize = 32.0f;
    animatedCheckboxStyle.indicatorRadius = 0.0f;
    animatedCheckboxStyle.labelGap = 0.0f;
    animatedCheckboxStyle.minHeight = 32.0f;
    animatedCheckboxStyle.borderWidth.normal = 0.0f;
    animatedCheckboxStyle.uncheckedBackgroundColor.normal = core::colorRGB(16, 28, 40);
    animatedCheckboxStyle.uncheckedBackgroundColor.hovered = core::colorRGB(144, 172, 208);

    ProbeCheckbox animatedCheckbox("", false);
    animatedCheckbox.setStyle(animatedCheckboxStyle);
    animatedCheckbox.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 32.0f, 32.0f));

    animatedCheckbox.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 16, 16), animatedCheckboxStyle.uncheckedBackgroundColor.normal),
        "checkbox should start with normal indicator background");

    core::MouseCrossEvent checkboxEnter(16.0, 16.0, core::EventType::MouseEnter);
    animatedCheckbox.onEvent(checkboxEnter);
    expect(runtime.animationScheduler.hasActiveAnimations(), "checkbox hover should enqueue a tween");

    runtime.animationScheduler.tick(10.0);
    animatedCheckbox.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 16, 16), animatedCheckboxStyle.uncheckedBackgroundColor.normal),
        "checkbox hover tween should begin from the normal background");

    runtime.animationScheduler.tick(10.075);
    animatedCheckbox.draw(canvas);
    const core::Color checkboxMidHover = readPixel(surface, 16, 16);
    expect(
        !nearlyEqualColor(checkboxMidHover, animatedCheckboxStyle.uncheckedBackgroundColor.normal),
        "checkbox hover midpoint should differ from the normal background");
    expect(
        !nearlyEqualColor(checkboxMidHover, *animatedCheckboxStyle.uncheckedBackgroundColor.hovered),
        "checkbox hover midpoint should differ from the hovered background");

    runtime.animationScheduler.tick(10.15);
    animatedCheckbox.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 16, 16), *animatedCheckboxStyle.uncheckedBackgroundColor.hovered),
        "checkbox hover tween should finish at the hovered background");

    core::MouseCrossEvent checkboxLeave(16.0, 16.0, core::EventType::MouseLeave);
    animatedCheckbox.onEvent(checkboxLeave);
    runtime.animationScheduler.tick(20.0);
    runtime.animationScheduler.tick(20.15);
    animatedCheckbox.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 16, 16), animatedCheckboxStyle.uncheckedBackgroundColor.normal),
        "checkbox leave tween should return to the normal background");

    ui::ToggleStyle animatedToggleStyle = runtime.theme.toggleStyle;
    animatedToggleStyle.trackWidth = 36.0f;
    animatedToggleStyle.trackHeight = 20.0f;
    animatedToggleStyle.thumbRadius = 6.0f;
    animatedToggleStyle.thumbInset = 4.0f;
    animatedToggleStyle.labelGap = 0.0f;
    animatedToggleStyle.minHeight = 20.0f;
    animatedToggleStyle.trackBorderWidth.normal = 0.0f;
    animatedToggleStyle.offTrackColor.normal = core::colorRGB(24, 40, 56);
    animatedToggleStyle.offTrackColor.hovered = core::colorRGB(180, 200, 216);

    ProbeToggle animatedToggle("", false);
    animatedToggle.setStyle(animatedToggleStyle);
    animatedToggle.arrange(core::Rect::MakeXYWH(0.0f, 40.0f, 36.0f, 20.0f));

    animatedToggle.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 30, 50), animatedToggleStyle.offTrackColor.normal),
        "toggle should start with normal track color");

    core::MouseCrossEvent toggleEnter(18.0, 50.0, core::EventType::MouseEnter);
    animatedToggle.onEvent(toggleEnter);
    runtime.animationScheduler.tick(30.0);
    runtime.animationScheduler.tick(30.075);
    animatedToggle.draw(canvas);
    const core::Color toggleMidHover = readPixel(surface, 30, 50);
    expect(
        !nearlyEqualColor(toggleMidHover, animatedToggleStyle.offTrackColor.normal),
        "toggle hover midpoint should differ from the normal track color");
    expect(
        !nearlyEqualColor(toggleMidHover, *animatedToggleStyle.offTrackColor.hovered),
        "toggle hover midpoint should differ from the hovered track color");

    runtime.animationScheduler.tick(30.15);
    animatedToggle.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 30, 50), *animatedToggleStyle.offTrackColor.hovered),
        "toggle hover tween should finish at the hovered track color");

    ui::RadioStyle animatedRadioStyle = runtime.theme.radioStyle;
    animatedRadioStyle.indicatorSize = 32.0f;
    animatedRadioStyle.innerDotSize = 10.0f;
    animatedRadioStyle.labelGap = 0.0f;
    animatedRadioStyle.minHeight = 32.0f;
    animatedRadioStyle.borderWidth.normal = 8.0f;
    animatedRadioStyle.unselectedBorderColor.normal = core::colorRGB(28, 56, 84);
    animatedRadioStyle.unselectedBorderColor.hovered = core::colorRGB(196, 212, 228);

    ProbeRadio animatedRadio("", "hover-group", false);
    animatedRadio.setStyle(animatedRadioStyle);
    animatedRadio.arrange(core::Rect::MakeXYWH(40.0f, 0.0f, 32.0f, 32.0f));

    animatedRadio.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 56, 4), animatedRadioStyle.unselectedBorderColor.normal, 8),
        "radio should start with normal border color");

    core::MouseCrossEvent radioEnter(56.0, 16.0, core::EventType::MouseEnter);
    animatedRadio.onEvent(radioEnter);
    runtime.animationScheduler.tick(40.0);
    runtime.animationScheduler.tick(40.075);
    animatedRadio.draw(canvas);
    const core::Color radioMidHover = readPixel(surface, 56, 4);
    expect(
        !nearlyEqualColor(radioMidHover, animatedRadioStyle.unselectedBorderColor.normal, 8),
        "radio hover midpoint should differ from the normal border color");
    expect(
        !nearlyEqualColor(radioMidHover, *animatedRadioStyle.unselectedBorderColor.hovered, 8),
        "radio hover midpoint should differ from the hovered border color");

    runtime.animationScheduler.tick(40.15);
    animatedRadio.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 56, 4), *animatedRadioStyle.unselectedBorderColor.hovered, 8),
        "radio hover tween should finish at the hovered border color");

    return 0;
}
