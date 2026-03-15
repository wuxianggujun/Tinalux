#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
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

class ProbeTextInput final : public tinalux::ui::TextInput {
public:
    using TextInput::TextInput;

    const tinalux::ui::TextInputStyle& effectiveStyle() const
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

    ProbeTextInput input("email");
    const core::Size defaultSize = input.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(
        nearlyEqual(
            input.effectiveStyle().textStyle.fontSize,
            runtime.theme.textInputStyle.textStyle.fontSize),
        "text input should use theme typography by default");
    expect(
        defaultSize.width() >= runtime.theme.textInputStyle.minWidth,
        "text input should respect theme minWidth");

    ui::TextInputStyle customStyle = runtime.theme.textInputStyle;
    customStyle.textStyle.fontSize = 22.0f;
    customStyle.paddingHorizontal = 28.0f;
    customStyle.paddingVertical = 18.0f;
    customStyle.minWidth = 320.0f;
    customStyle.minHeight = defaultSize.height() + 18.0f;
    customStyle.borderWidth.normal = 3.0f;

    input.setStyle(customStyle);
    const core::Size customSize = input.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(input.style() != nullptr, "setStyle should install per-input style override");
    expect(
        nearlyEqual(input.effectiveStyle().paddingHorizontal, 28.0f),
        "custom style should override horizontal padding");
    expect(customSize.width() >= 320.0f, "custom minWidth should be reflected in measure");
    expect(
        customSize.height() >= customStyle.minHeight,
        "custom minHeight should be reflected in measure");

    input.clearStyle();
    expect(input.style() == nullptr, "clearStyle should remove override");
    const core::Size resetSize = input.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(nearlyEqual(resetSize.width(), defaultSize.width()), "clearing style should restore default width");
    expect(nearlyEqual(resetSize.height(), defaultSize.height()), "clearing style should restore default height");

    runtime.theme = ui::Theme::dark();
    runtime.theme.textInputStyle =
        ui::TextInputStyle::standard(runtime.theme.colors, runtime.theme.typography, runtime.theme.spacingScale);
    runtime.theme.textInputStyle.borderWidth.focused = 3.0f;
    runtime.theme.textInputStyle.borderColor.focused = runtime.theme.colors.primaryVariant;
    input.setFocused(true);
    expect(
        input.effectiveStyle().borderWidth.resolve(ui::WidgetState::Focused) > 2.5f,
        "text input should expose focused border width from theme style");
    expect(
        input.effectiveStyle().borderColor.resolve(ui::WidgetState::Focused)
            == runtime.theme.colors.primaryVariant,
        "focused border color should come from theme style");

    ui::TextInputStyle animatedStyle = runtime.theme.textInputStyle;
    animatedStyle.minWidth = 160.0f;
    animatedStyle.minHeight = 40.0f;
    animatedStyle.paddingHorizontal = 12.0f;
    animatedStyle.paddingVertical = 8.0f;
    animatedStyle.borderRadius = 0.0f;
    animatedStyle.borderWidth.normal = 4.0f;
    animatedStyle.borderWidth.hovered = 4.0f;
    animatedStyle.borderWidth.focused = 4.0f;
    animatedStyle.backgroundColor.normal = core::colorRGB(8, 8, 8);
    animatedStyle.borderColor.normal = core::colorRGB(24, 48, 80);
    animatedStyle.borderColor.hovered = core::colorRGB(176, 208, 236);
    animatedStyle.borderColor.focused = core::colorRGB(244, 190, 92);
    animatedStyle.textColor = core::colorARGB(0, 0, 0, 0);
    animatedStyle.placeholderColor = core::colorARGB(0, 0, 0, 0);
    animatedStyle.caretColor = core::colorARGB(0, 0, 0, 0);
    animatedStyle.selectionColor = core::colorARGB(0, 0, 0, 0);

    ProbeTextInput animatedInput("");
    animatedInput.setStyle(animatedStyle);
    animatedInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 160.0f, 40.0f));

    const rendering::RenderSurface surface = rendering::createRasterSurface(180, 60);
    expect(static_cast<bool>(surface), "text input animation test should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "text input animation test should expose canvas");

    animatedInput.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 2, 20), animatedStyle.borderColor.normal),
        "text input should start with normal border color");

    core::MouseCrossEvent hoverEnter(80.0, 20.0, core::EventType::MouseEnter);
    animatedInput.onEvent(hoverEnter);
    expect(runtime.animationScheduler.hasActiveAnimations(), "text input hover should enqueue a tween");

    runtime.animationScheduler.tick(10.0);
    animatedInput.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 2, 20), animatedStyle.borderColor.normal),
        "text input hover tween should begin from normal border color");

    runtime.animationScheduler.tick(10.075);
    animatedInput.draw(canvas);
    const core::Color midHoverBorder = readPixel(surface, 2, 20);
    expect(
        !nearlyEqualColor(midHoverBorder, animatedStyle.borderColor.normal),
        "text input hover midpoint should differ from normal border color");
    expect(
        !nearlyEqualColor(midHoverBorder, *animatedStyle.borderColor.hovered),
        "text input hover midpoint should differ from hovered border color");

    runtime.animationScheduler.tick(10.15);
    animatedInput.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 2, 20), *animatedStyle.borderColor.hovered),
        "text input hover tween should finish at hovered border color");

    core::MouseCrossEvent hoverLeave(80.0, 20.0, core::EventType::MouseLeave);
    animatedInput.onEvent(hoverLeave);
    runtime.animationScheduler.tick(20.0);
    runtime.animationScheduler.tick(20.15);
    animatedInput.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 2, 20), animatedStyle.borderColor.normal),
        "text input leave tween should return to normal border color");

    animatedInput.setFocused(true);
    runtime.animationScheduler.tick(30.0);
    runtime.animationScheduler.tick(30.15);
    animatedInput.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 2, 20), *animatedStyle.borderColor.focused),
        "text input focus tween should finish at focused border color");

    return 0;
}
