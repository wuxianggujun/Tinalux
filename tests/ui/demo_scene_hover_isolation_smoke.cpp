#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "../../src/app/UIContext.h"
#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/DemoScene.h"
#include "tinalux/ui/Panel.h"
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

std::uint64_t sampleRegionChecksum(
    const tinalux::rendering::RenderSurface& surface,
    tinalux::core::Rect bounds,
    int surfaceWidth,
    int surfaceHeight)
{
    std::uint64_t checksum = 1469598103934665603ull;
    const int left = std::clamp(static_cast<int>(bounds.left()), 0, surfaceWidth - 1);
    const int top = std::clamp(static_cast<int>(bounds.top()), 0, surfaceHeight - 1);
    const int right = std::clamp(static_cast<int>(bounds.right()) - 1, left, surfaceWidth - 1);
    const int bottom = std::clamp(static_cast<int>(bounds.bottom()) - 1, top, surfaceHeight - 1);

    for (int y = top; y <= bottom; y += 2) {
        for (int x = left; x <= right; x += 2) {
            const tinalux::core::Color pixel = readPixel(surface, x, y);
            checksum ^= static_cast<std::uint64_t>(pixel);
            checksum *= 1099511628211ull;
        }
    }

    return checksum;
}

std::shared_ptr<tinalux::ui::Button> nthButton(
    const std::shared_ptr<tinalux::ui::Container>& container,
    std::size_t buttonIndex)
{
    std::size_t currentIndex = 0;
    for (const auto& child : container->children()) {
        auto button = std::dynamic_pointer_cast<tinalux::ui::Button>(child);
        if (button == nullptr) {
            continue;
        }

        if (currentIndex == buttonIndex) {
            return button;
        }
        ++currentIndex;
    }

    return {};
}

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::dark();
    ui::ScopedRuntimeState bind(runtime);

    app::UIContext context;
    auto rootWidget = ui::createDemoScene(runtime.theme);
    auto root = std::dynamic_pointer_cast<ui::Container>(rootWidget);
    expect(root != nullptr, "demo scene should expose a container root");
    context.setRootWidget(rootWidget);

    const rendering::RenderSurface surface = rendering::createRasterSurface(1280, 900);
    expect(static_cast<bool>(surface), "demo scene hover isolation smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "demo scene hover isolation smoke should expose canvas");

    const bool firstFullRedraw = context.render(canvas, 1280, 900, 1.0f);
    expect(firstFullRedraw, "initial demo scene render should redraw the full scene");

    auto shell = std::dynamic_pointer_cast<ui::Container>(root->children().back());
    expect(shell != nullptr, "demo scene should expose a showcase shell container");

    auto navPanel = std::dynamic_pointer_cast<ui::Panel>(shell->children()[0]);
    auto contentPanel = std::dynamic_pointer_cast<ui::Panel>(shell->children()[1]);
    expect(navPanel != nullptr, "demo scene should expose a navigation panel");
    expect(contentPanel != nullptr, "demo scene should expose a content panel");

    auto navButtons = std::dynamic_pointer_cast<ui::Container>(navPanel->children()[2]);
    expect(navButtons != nullptr, "navigation panel should expose a button column");

    auto layoutButton = nthButton(navButtons, 1);
    expect(layoutButton != nullptr, "navigation button column should expose the Layout button");

    const core::Rect contentBounds = contentPanel->globalBounds();
    expect(!contentBounds.isEmpty(), "content panel bounds should be valid after the initial render");
    const std::uint64_t baselineChecksum = sampleRegionChecksum(surface, contentBounds, 1280, 900);

    const core::Rect layoutButtonBounds = layoutButton->globalBounds();
    const float hoverX = layoutButtonBounds.centerX();
    const float hoverY = layoutButtonBounds.centerY();
    core::MouseMoveEvent hoverEvent(hoverX, hoverY);
    context.handleEvent(hoverEvent, [] {}, 1.0f);

    const bool secondFullRedraw = context.render(canvas, 1280, 900, 1.0f);
    expect(!secondFullRedraw, "hovering a navigation button should stay on the partial redraw path");

    const std::uint64_t hoverChecksum = sampleRegionChecksum(surface, contentBounds, 1280, 900);
    expect(
        hoverChecksum == baselineChecksum,
        "hovering a left navigation button should not alter the right content panel pixels");

    return 0;
}
