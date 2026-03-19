#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "../../src/app/UIContext.h"
#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/DemoScene.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
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

std::uint64_t surfaceChecksum(
    const tinalux::rendering::RenderSurface& surface,
    int width,
    int height)
{
    std::uint64_t checksum = 1469598103934665603ull;
    for (int y = 0; y < height; y += 3) {
        for (int x = 0; x < width; x += 3) {
            const tinalux::core::Color pixel = readPixel(surface, x, y);
            checksum ^= static_cast<std::uint64_t>(pixel);
            checksum *= 1099511628211ull;
        }
    }
    return checksum;
}

std::uint64_t renderSceneChecksum(
    tinalux::app::UIContext& context,
    int width,
    int height)
{
    const tinalux::rendering::RenderSurface surface =
        tinalux::rendering::createRasterSurface(width, height);
    expect(static_cast<bool>(surface), "round-trip render smoke should create raster surface");
    tinalux::rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "round-trip render smoke should expose canvas");

    context.render(canvas, width, height, 1.0f);
    return surfaceChecksum(surface, width, height);
}

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::dark();
    ui::ScopedRuntimeState bind(runtime);

    app::UIContext context;
    auto rootWidget = ui::createDemoScene(runtime.theme, runtime.animationScheduler);
    auto root = std::dynamic_pointer_cast<ui::Container>(rootWidget);
    expect(root != nullptr, "demo scene should expose a container root");
    context.setRootWidget(rootWidget);

    const std::uint64_t smallBefore = renderSceneChecksum(context, 680, 960);
    const std::uint64_t largeChecksum = renderSceneChecksum(context, 1280, 960);
    const std::uint64_t smallAfter = renderSceneChecksum(context, 680, 960);

    expect(largeChecksum != 0, "wide render checksum should be valid");
    expect(
        smallBefore == smallAfter,
        "demo scene raster render should be stable after a narrow-wide-narrow round-trip");

    return 0;
}
