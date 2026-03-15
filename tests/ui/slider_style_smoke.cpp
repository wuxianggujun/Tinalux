#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Slider.h"
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

class ProbeSlider final : public tinalux::ui::Slider {
public:
    using Slider::Slider;

    const tinalux::ui::SliderStyle& effectiveStyle() const
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

    ProbeSlider slider;
    slider.setRange(0.0f, 100.0f);
    slider.setStep(5.0f);
    slider.setValue(25.0f);

    const core::Size defaultSize = slider.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(
        nearlyEqual(
            slider.effectiveStyle().preferredWidth,
            runtime.theme.sliderStyle.preferredWidth),
        "slider should use theme slider style by default");

    ui::SliderStyle customStyle = runtime.theme.sliderStyle;
    customStyle.preferredWidth = runtime.theme.sliderStyle.preferredWidth + 80.0f;
    customStyle.preferredHeight = runtime.theme.sliderStyle.preferredHeight + 14.0f;
    customStyle.horizontalInset = runtime.theme.sliderStyle.horizontalInset + 6.0f;
    customStyle.trackHeight = runtime.theme.sliderStyle.trackHeight + 2.0f;
    customStyle.thumbRadius = runtime.theme.sliderStyle.thumbRadius + 3.0f;
    customStyle.activeThumbRadius = customStyle.thumbRadius + 2.0f;

    slider.setStyle(customStyle);
    const core::Size customSize = slider.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(slider.style() != nullptr, "setStyle should install per-slider style override");
    expect(customSize.width() >= customStyle.preferredWidth, "custom preferredWidth should affect measure");
    expect(customSize.height() >= customStyle.preferredHeight, "custom preferredHeight should affect measure");

    slider.clearStyle();
    expect(slider.style() == nullptr, "clearStyle should remove slider override");
    const core::Size resetSize = slider.measure(ui::Constraints::loose(640.0f, 200.0f));
    expect(nearlyEqual(resetSize.width(), defaultSize.width()), "clearing style should restore default width");
    expect(nearlyEqual(resetSize.height(), defaultSize.height()), "clearing style should restore default height");

    runtime.theme = ui::Theme::dark();
    runtime.theme.sliderStyle =
        ui::SliderStyle::standard(runtime.theme.colors, runtime.theme.typography, runtime.theme.spacingScale);
    runtime.theme.sliderStyle.preferredWidth = 320.0f;
    runtime.theme.sliderStyle.trackHeight = 8.0f;
    runtime.theme.sliderStyle.thumbRadius = 13.0f;

    expect(
        nearlyEqual(slider.effectiveStyle().preferredWidth, 320.0f),
        "slider should observe runtime theme slider preferredWidth");
    expect(
        nearlyEqual(slider.effectiveStyle().trackHeight, 8.0f),
        "slider should observe runtime theme slider trackHeight");
    expect(
        nearlyEqual(slider.effectiveStyle().thumbRadius, 13.0f),
        "slider should observe runtime theme slider thumbRadius");

    slider.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 320.0f, 48.0f));
    core::KeyEvent keyRight(core::keys::kRight, 0, 0, core::EventType::KeyPress);
    slider.setFocused(true);
    slider.onEvent(keyRight);
    expect(std::abs(slider.value() - 30.0f) < 0.001f, "slider keyboard behavior should remain intact after styling");

    ui::SliderStyle animatedStyle = runtime.theme.sliderStyle;
    animatedStyle.preferredWidth = 120.0f;
    animatedStyle.preferredHeight = 32.0f;
    animatedStyle.horizontalInset = 12.0f;
    animatedStyle.trackHeight = 6.0f;
    animatedStyle.thumbRadius = 8.0f;
    animatedStyle.activeThumbRadius = 8.0f;
    animatedStyle.focusHaloPadding = 0.0f;
    animatedStyle.trackColor.normal = core::colorRGB(10, 10, 10);
    animatedStyle.fillColor.normal = core::colorRGB(24, 48, 80);
    animatedStyle.fillColor.hovered = core::colorRGB(180, 208, 232);
    animatedStyle.fillColor.focused = core::colorRGB(236, 192, 120);
    animatedStyle.thumbColor.normal = core::colorRGB(32, 64, 96);
    animatedStyle.thumbColor.hovered = core::colorRGB(188, 220, 240);
    animatedStyle.thumbColor.focused = core::colorRGB(240, 184, 96);
    animatedStyle.thumbInnerColor = animatedStyle.thumbColor;

    ProbeSlider animatedSlider;
    animatedSlider.setStyle(animatedStyle);
    animatedSlider.setRange(0.0f, 100.0f);
    animatedSlider.setValue(50.0f);
    animatedSlider.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 120.0f, 32.0f));

    const rendering::RenderSurface surface = rendering::createRasterSurface(128, 48);
    expect(static_cast<bool>(surface), "slider animation test should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "slider animation test should expose canvas");

    animatedSlider.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 35, 16), animatedStyle.fillColor.normal),
        "slider should start with normal fill color");

    core::MouseCrossEvent enterEvent(60.0, 16.0, core::EventType::MouseEnter);
    animatedSlider.onEvent(enterEvent);
    expect(runtime.animationScheduler.hasActiveAnimations(), "slider hover should enqueue a tween");

    runtime.animationScheduler.tick(10.0);
    animatedSlider.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 35, 16), animatedStyle.fillColor.normal),
        "slider hover tween should begin from normal fill color");

    runtime.animationScheduler.tick(10.075);
    animatedSlider.draw(canvas);
    const core::Color midHoverFill = readPixel(surface, 35, 16);
    expect(
        !nearlyEqualColor(midHoverFill, animatedStyle.fillColor.normal),
        "slider hover midpoint should differ from normal fill color");
    expect(
        !nearlyEqualColor(midHoverFill, *animatedStyle.fillColor.hovered),
        "slider hover midpoint should differ from hovered fill color");

    runtime.animationScheduler.tick(10.15);
    animatedSlider.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 35, 16), *animatedStyle.fillColor.hovered),
        "slider hover tween should finish at hovered fill color");

    core::MouseCrossEvent leaveEvent(60.0, 16.0, core::EventType::MouseLeave);
    animatedSlider.onEvent(leaveEvent);
    runtime.animationScheduler.tick(20.0);
    runtime.animationScheduler.tick(20.15);
    animatedSlider.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 35, 16), animatedStyle.fillColor.normal),
        "slider leave tween should return to normal fill color");

    animatedSlider.setFocused(true);
    runtime.animationScheduler.tick(30.0);
    runtime.animationScheduler.tick(30.15);
    animatedSlider.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 60, 16), animatedStyle.thumbColor.resolve(ui::WidgetState::Focused)),
        "slider focus tween should finish at focused thumb color");

    return 0;
}
