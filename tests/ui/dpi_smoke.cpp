#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "include/core/SkColor.h"

#include "../../src/app/UIContext.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Widget.h"

namespace {

constexpr tinalux::core::Color kClearColor = tinalux::core::colorRGB(18, 20, 28);
constexpr tinalux::core::Color kProbeColor = tinalux::core::colorRGB(64, 160, 96);

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

class DpiProbeWidget final : public tinalux::ui::Widget {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        lastConstraints = constraints;
        return constraints.constrain(tinalux::core::Size::Make(40.0f, 30.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        lastBounds = bounds;
        Widget::arrange(bounds);
    }

    bool onEvent(tinalux::core::Event& event) override
    {
        if (event.type() == tinalux::core::EventType::MouseButtonPress) {
            const auto& mouseEvent = static_cast<const tinalux::core::MouseButtonEvent&>(event);
            receivedPress = true;
            lastPressX = static_cast<float>(mouseEvent.x);
            lastPressY = static_cast<float>(mouseEvent.y);
            return true;
        }
        return false;
    }

    void onDraw(tinalux::rendering::Canvas& canvas) override
    {
        canvas.drawRect(tinalux::core::Rect::MakeXYWH(0.0f, 0.0f, 40.0f, 30.0f), kProbeColor);
    }

    tinalux::ui::Constraints lastConstraints {};
    tinalux::core::Rect lastBounds = tinalux::core::Rect::MakeEmpty();
    bool receivedPress = false;
    float lastPressX = 0.0f;
    float lastPressY = 0.0f;
};

}  // namespace

int main()
{
    using namespace tinalux;

    app::UIContext context;
    auto probe = std::make_shared<DpiProbeWidget>();
    context.setRootWidget(probe);

    const rendering::RenderSurface surface = rendering::createRasterSurface(100, 80);
    expect(static_cast<bool>(surface), "dpi smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dpi smoke should expose canvas");

    const bool fullRedraw = context.render(canvas, 100, 80, 2.0f);
    expect(fullRedraw, "initial dpi render should redraw the full surface");
    expect(
        nearlyEqual(probe->lastConstraints.minWidth, 50.0f)
            && nearlyEqual(probe->lastConstraints.maxWidth, 50.0f),
        "UIContext should measure widgets with logical width");
    expect(
        nearlyEqual(probe->lastConstraints.minHeight, 40.0f)
            && nearlyEqual(probe->lastConstraints.maxHeight, 40.0f),
        "UIContext should measure widgets with logical height");
    expect(
        nearlyEqual(probe->lastBounds.width(), 50.0f)
            && nearlyEqual(probe->lastBounds.height(), 40.0f),
        "UIContext should arrange root bounds in logical coordinates");
    expect(
        nearlyEqualColor(readPixel(surface, 60, 40), kProbeColor),
        "canvas scaling should expand logical drawing into physical pixels");
    expect(
        nearlyEqualColor(readPixel(surface, 90, 70), kClearColor),
        "pixels outside the scaled logical draw region should stay clear");

    const rendering::RenderSurface snappedSurface = rendering::createRasterSurface(48, 32);
    expect(static_cast<bool>(snappedSurface), "dpi smoke should create a raster surface for pixel snapping");
    rendering::Canvas snappedCanvas = snappedSurface.canvas();
    expect(static_cast<bool>(snappedCanvas), "pixel snapping surface should expose canvas");
    snappedCanvas.clear(kClearColor);
    snappedCanvas.save();
    snappedCanvas.scale(1.5f, 1.5f);
    snappedCanvas.translate(0.25f, 0.0f);
    snappedCanvas.drawRect(core::Rect::MakeXYWH(10.0f, 4.0f, 4.0f, 8.0f), kProbeColor);
    snappedCanvas.restore();
    expect(
        nearlyEqualColor(readPixel(snappedSurface, 14, 10), kClearColor),
        "pixel snapping should keep the pixel just outside the snapped left edge clear");
    expect(
        nearlyEqualColor(readPixel(snappedSurface, 15, 10), kProbeColor),
        "pixel snapping should align the filled rect left edge to a full physical pixel");
    expect(
        nearlyEqualColor(readPixel(snappedSurface, 20, 10), kProbeColor),
        "pixel snapping should preserve the snapped rect interior after non-integer scale and translation");
    expect(
        nearlyEqualColor(readPixel(snappedSurface, 21, 10), kClearColor),
        "pixel snapping should keep the pixel just outside the snapped right edge clear");

    core::MouseButtonEvent press(
        core::mouse::kLeft,
        0,
        80.0,
        40.0,
        core::EventType::MouseButtonPress);
    context.handleEvent(press, [] {}, 2.0f);
    expect(probe->receivedPress, "logical root should receive a normalized pointer press");
    expect(
        nearlyEqual(probe->lastPressX, 40.0f) && nearlyEqual(probe->lastPressY, 20.0f),
        "pointer coordinates should be converted from physical pixels to logical pixels");

    return 0;
}
