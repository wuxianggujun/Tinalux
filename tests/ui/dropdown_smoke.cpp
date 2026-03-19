#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "../../src/rendering/RenderHandles.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ResourceLoader.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
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

    std::array<std::uint8_t, 4> pixel {};
    const bool read = skiaImage->readPixels(
        SkImageInfo::Make(1, 1, kRGBA_8888_SkColorType, kPremul_SkAlphaType),
        pixel.data(),
        4,
        x,
        y);
    expect(read, "snapshot pixel read should succeed");
    return tinalux::core::colorARGB(pixel[3], pixel[0], pixel[1], pixel[2]);
}

}  // namespace

int main()
{
    using namespace tinalux;
    ui::RuntimeState runtime;
    ui::ScopedRuntimeState bind(runtime);

    const auto pngBytes = std::to_array<std::uint8_t>({
        137, 80, 78, 71, 13, 10, 26, 10,
        0,   0,  0,  13, 73, 72, 68, 82,
        0,   0,  0,  1,  0,  0,  0,  1,
        8,   6,  0,  0,  0,  31, 21, 196,
        137, 0,  0,  0,  13, 73, 68, 65,
        84,  120, 156, 99, 248, 255, 255, 63,
        0,   5,  254, 2,  254, 167, 53, 129,
        132, 0,  0,  0,  0,  73, 69, 78,
        68,  174, 66, 96, 130,
    });

    const rendering::RenderSurface surface = rendering::createRasterSurface(160, 96);
    expect(static_cast<bool>(surface), "dropdown smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dropdown smoke should expose canvas");

    ui::Dropdown dropdown({ "Alpha", "Beta", "Gamma" });
    dropdown.setPlaceholder("Select item");
    const core::Size dropdownSize = dropdown.measure(ui::Constraints::loose(200.0f, 80.0f));
    expect(dropdownSize.width() >= 120.0f, "dropdown should honor minimum width");
    expect(dropdownSize.height() >= 32.0f, "dropdown should honor item height");

    dropdown.setSelectedIndex(1);
    expect(dropdown.selectedIndex() == 1, "dropdown should keep selected index");
    expect(dropdown.selectedItem() == "Beta", "dropdown should expose selected item text");

    ui::Dropdown fallbackDropdown({ "Alpha" });
    fallbackDropdown.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 140.0f, 32.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    fallbackDropdown.draw(canvas);
    const core::Color fallbackIndicatorPixel = readPixel(surface, 126, 16);

    ui::IconRegistry::instance().clear();
    ui::IconRegistry::instance().registerIconData(ui::IconType::ArrowDown, pngBytes);
    ui::Dropdown registryDropdown({ "Alpha" });
    registryDropdown.setIndicatorIcon(ui::IconType::ArrowDown, 12.0f);
    expect(static_cast<bool>(registryDropdown.indicatorIcon()), "dropdown should resolve indicator icon from icon registry");
    registryDropdown.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 140.0f, 32.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    registryDropdown.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 126, 16), fallbackIndicatorPixel),
        "registry-backed dropdown should replace fallback indicator rendering");

    ui::ResourceLoader::instance().clear();
    const std::filesystem::path asyncIconPath =
        std::filesystem::temp_directory_path() / "tinalux_dropdown_icon_async.png";
    {
        std::ofstream output(asyncIconPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async dropdown icon test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async dropdown icon test file should be written");
    }

    ui::Dropdown asyncDropdown({ "Alpha", "Beta" });
    asyncDropdown.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 140.0f, 32.0f));
    asyncDropdown.loadIndicatorIconAsync(asyncIconPath.string());
    expect(asyncDropdown.indicatorIconLoading(), "dropdown should report loading after async indicator request");
    expect(
        asyncDropdown.indicatorIconLoadState() == ui::DropdownIndicatorLoadState::Loading,
        "dropdown should enter loading state while indicator icon is in flight");
    expect(
        asyncDropdown.indicatorIconPath() == asyncIconPath.string(),
        "dropdown should remember requested indicator icon path");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncDropdown.draw(canvas);
    const core::Color loadingIndicatorPixel = readPixel(surface, 126, 16);

    bool asyncLoaded = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (static_cast<bool>(asyncDropdown.indicatorIcon())) {
            asyncLoaded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncLoaded, "dropdown should receive async indicator icon");
    expect(!asyncDropdown.indicatorIconLoading(), "dropdown should stop reporting loading after async indicator callback");
    expect(
        asyncDropdown.indicatorIconLoadState() == ui::DropdownIndicatorLoadState::Ready,
        "dropdown should become ready after async indicator load succeeds");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncDropdown.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 126, 16), loadingIndicatorPixel),
        "dropdown should render async-loaded indicator icon after completion");
    expect(std::filesystem::remove(asyncIconPath), "async dropdown icon test file should be removable");

    ui::Dropdown failedDropdown({ "Alpha" });
    failedDropdown.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 140.0f, 32.0f));
    failedDropdown.loadIndicatorIconAsync("__missing_async_dropdown_icon_test__.png");
    bool failedResolved = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (!failedDropdown.indicatorIconLoading()
            && failedDropdown.indicatorIconLoadState() == ui::DropdownIndicatorLoadState::Failed) {
            failedResolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(failedResolved, "dropdown should enter failed state when async indicator icon cannot load");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    failedDropdown.draw(canvas);
    expect(!failedDropdown.isDirty(), "dropdown should draw cleanly in failed indicator icon state");

    return 0;
}
