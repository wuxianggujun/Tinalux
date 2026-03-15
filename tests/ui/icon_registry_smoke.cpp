#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/IconRegistry.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::vector<std::uint8_t> solidPixels(int width, int height, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4), 0);
    for (int index = 0; index < width * height; ++index) {
        pixels[static_cast<std::size_t>(index * 4)] = r;
        pixels[static_cast<std::size_t>(index * 4 + 1)] = g;
        pixels[static_cast<std::size_t>(index * 4 + 2)] = b;
        pixels[static_cast<std::size_t>(index * 4 + 3)] = 255;
    }
    return pixels;
}

}  // namespace

int main()
{
    using namespace tinalux;

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

    ui::IconRegistry& registry = ui::IconRegistry::instance();
    registry.clear();
    expect(registry.registeredIconCount() == 0, "icon registry should start empty after clear");
    expect(!registry.hasIcon(ui::IconType::Check), "icon registry should report missing icon after clear");

    registry.registerIconData(ui::IconType::Check, pngBytes);
    expect(registry.hasIcon(ui::IconType::Check), "registerIconData should make icon discoverable");

    const rendering::Image encodedCheckIcon = registry.getIcon(ui::IconType::Check, 16.0f);
    expect(static_cast<bool>(encodedCheckIcon), "encoded icon should decode successfully");
    expect(
        encodedCheckIcon.width() == 1 && encodedCheckIcon.height() == 1,
        "encoded icon should preserve source dimensions");

    int factoryCallCount = 0;
    registry.registerIconFactory(ui::IconType::Refresh, [&factoryCallCount](float sizeHint) {
        ++factoryCallCount;
        const int side = std::max(1, static_cast<int>(sizeHint));
        const std::vector<std::uint8_t> pixels = solidPixels(side, side, 64, 128, 255);
        return rendering::createImageFromRGBA(side, side, pixels);
    });

    const rendering::Image refresh16 = registry.getIcon(ui::IconType::Refresh, 16.0f);
    expect(static_cast<bool>(refresh16), "factory icon should be created");
    expect(refresh16.width() == 16 && refresh16.height() == 16, "factory icon should honor requested size");
    expect(factoryCallCount == 1, "first factory lookup should create one icon");

    const rendering::Image refresh16Again = registry.getIcon(ui::IconType::Refresh, 16.0f);
    expect(static_cast<bool>(refresh16Again), "cached factory icon should still resolve");
    expect(factoryCallCount == 1, "repeated lookup with same size should reuse cached factory image");

    const rendering::Image refresh24 = registry.getIcon(ui::IconType::Refresh, 24.0f);
    expect(static_cast<bool>(refresh24), "factory icon should resolve for a second size");
    expect(refresh24.width() == 24 && refresh24.height() == 24, "factory icon should create size-specific image");
    expect(factoryCallCount == 2, "new size lookup should generate one more cached image");

    const std::filesystem::path iconPath =
        std::filesystem::temp_directory_path() / "tinalux_icon_registry_smoke.png";
    {
        std::ofstream output(iconPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "temporary icon file should open for writing");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "temporary icon file should be written");
    }

    rendering::clearImageFileCache();
    registry.registerIcon(ui::IconType::Search, iconPath.string());
    const rendering::Image searchIcon = registry.getIcon(ui::IconType::Search, 20.0f);
    expect(static_cast<bool>(searchIcon), "file-backed icon should load successfully");
    expect(std::filesystem::remove(iconPath), "temporary icon file should be removable");

    const rendering::Image cachedSearchIcon = registry.getIcon(ui::IconType::Search, 20.0f);
    expect(static_cast<bool>(cachedSearchIcon), "file-backed icon should reuse rendering cache after source removal");

    registry.unregisterIcon(ui::IconType::Search);
    expect(!registry.hasIcon(ui::IconType::Search), "unregisterIcon should remove icon mapping");
    expect(!registry.getIcon(ui::IconType::Search, 20.0f), "removed icon should no longer resolve");

    expect(registry.registeredIconCount() == 2, "registry should keep only remaining encoded and factory icons");
    registry.clear();
    expect(registry.registeredIconCount() == 0, "clear should remove all icon registrations");

    return 0;
}
