#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
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

bool nearlyEqualColor(tinalux::core::Color lhs, tinalux::core::Color rhs)
{
    const auto channelClose = [](tinalux::core::Color::Channel a, tinalux::core::Color::Channel b) {
        return std::abs(static_cast<int>(a) - static_cast<int>(b)) <= 2;
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

class ProbeButton final : public tinalux::ui::Button {
public:
    using Button::Button;

    const tinalux::ui::ButtonStyle& effectiveStyle() const
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

    ProbeButton button("Apply theme");
    const core::Size defaultSize = button.measure(ui::Constraints::loose(400.0f, 200.0f));
    expect(
        nearlyEqual(
            button.effectiveStyle().textStyle.fontSize,
            runtime.theme.buttonStyle.textStyle.fontSize),
        "button should use theme button typography by default");
    expect(
        defaultSize.width() >= runtime.theme.buttonStyle.minWidth,
        "button measure should respect theme button minWidth");

    ui::ButtonStyle customStyle = runtime.theme.buttonStyle;
    customStyle.paddingHorizontal = 36.0f;
    customStyle.paddingVertical = 20.0f;
    customStyle.textStyle.fontSize = 24.0f;
    customStyle.iconSpacing = 20.0f;
    customStyle.borderColor.normal = runtime.theme.primary;
    customStyle.borderWidth.normal = 2.0f;

    button.setStyle(customStyle);
    const core::Size customSize = button.measure(ui::Constraints::loose(400.0f, 200.0f));
    expect(button.style() != nullptr, "setStyle should install per-button style override");
    expect(
        nearlyEqual(button.effectiveStyle().paddingHorizontal, 36.0f),
        "custom button style should override theme padding");
    expect(customSize.width() > defaultSize.width(), "custom horizontal padding should enlarge button width");
    expect(customSize.height() > defaultSize.height(), "custom typography and vertical padding should enlarge button height");

    button.clearStyle();
    expect(button.style() == nullptr, "clearStyle should remove per-button style override");
    const core::Size resetSize = button.measure(ui::Constraints::loose(400.0f, 200.0f));
    expect(nearlyEqual(resetSize.width(), defaultSize.width()), "clearing style should restore theme-driven width");
    expect(nearlyEqual(resetSize.height(), defaultSize.height()), "clearing style should restore theme-driven height");

    runtime.theme = ui::Theme::dark();
    runtime.theme.buttonStyle =
        ui::ButtonStyle::outlined(runtime.theme.colors, runtime.theme.typography, runtime.theme.spacingScale);
    expect(
        button.effectiveStyle().borderWidth.normal > 0.0f,
        "button should observe theme-provided outlined border style");
    expect(
        button.effectiveStyle().borderColor.normal == runtime.theme.colors.border,
        "theme button style override should flow into runtime button resolution");

    ui::ButtonStyle animatedStyle = runtime.theme.buttonStyle;
    animatedStyle.backgroundColor.normal = core::colorRGB(12, 24, 48);
    animatedStyle.backgroundColor.hovered = core::colorRGB(140, 180, 220);
    animatedStyle.borderColor.normal = core::colorARGB(0, 0, 0, 0);
    animatedStyle.borderWidth.normal = 0.0f;
    animatedStyle.textColor.normal = core::colorARGB(0, 0, 0, 0);
    animatedStyle.paddingHorizontal = 12.0f;
    animatedStyle.paddingVertical = 12.0f;

    ProbeButton animatedButton("");
    animatedButton.setStyle(animatedStyle);
    animatedButton.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 96.0f, 48.0f));

    const rendering::RenderSurface surface = rendering::createRasterSurface(128, 96);
    expect(static_cast<bool>(surface), "button animation test should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "button animation test should expose canvas");

    animatedButton.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 48, 24), animatedStyle.backgroundColor.normal),
        "button should start with normal background color");

    core::MouseCrossEvent enterEvent(48.0, 24.0, core::EventType::MouseEnter);
    animatedButton.onEvent(enterEvent);
    expect(runtime.animationScheduler.hasActiveAnimations(), "hover enter should enqueue a tween");

    runtime.animationScheduler.tick(100.0);
    animatedButton.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 48, 24), animatedStyle.backgroundColor.normal),
        "hover tween should begin from the normal color");

    runtime.animationScheduler.tick(100.075);
    animatedButton.draw(canvas);
    const core::Color midHoverColor = readPixel(surface, 48, 24);
    expect(
        !nearlyEqualColor(midHoverColor, animatedStyle.backgroundColor.normal),
        "hover tween midpoint should differ from the normal color");
    expect(
        !nearlyEqualColor(midHoverColor, *animatedStyle.backgroundColor.hovered),
        "hover tween midpoint should differ from the hovered color");

    runtime.animationScheduler.tick(100.15);
    animatedButton.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 48, 24), *animatedStyle.backgroundColor.hovered),
        "hover tween should finish at the hovered color");
    expect(!runtime.animationScheduler.hasActiveAnimations(), "hover tween should complete");

    core::MouseCrossEvent leaveEvent(48.0, 24.0, core::EventType::MouseLeave);
    animatedButton.onEvent(leaveEvent);
    expect(runtime.animationScheduler.hasActiveAnimations(), "hover leave should enqueue a tween");

    runtime.animationScheduler.tick(200.0);
    runtime.animationScheduler.tick(200.075);
    animatedButton.draw(canvas);
    const core::Color midLeaveColor = readPixel(surface, 48, 24);
    expect(
        !nearlyEqualColor(midLeaveColor, animatedStyle.backgroundColor.normal),
        "leave tween midpoint should still be transitioning");
    expect(
        !nearlyEqualColor(midLeaveColor, *animatedStyle.backgroundColor.hovered),
        "leave tween midpoint should move away from hovered color");

    runtime.animationScheduler.tick(200.15);
    animatedButton.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 48, 24), animatedStyle.backgroundColor.normal),
        "leave tween should finish at the normal color");
    expect(!runtime.animationScheduler.hasActiveAnimations(), "leave tween should complete");

    return 0;
}
