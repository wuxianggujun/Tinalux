#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/ResourceManager.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
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

    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() / "tinalux_resource_manager_smoke";
    std::filesystem::create_directories(tempRoot);

    const std::filesystem::path basePath = tempRoot / "status.png";
    const std::filesystem::path path2x = tempRoot / "status@2x.png";
    const std::filesystem::path path3x = tempRoot / "status@3x.png";

    auto writePng = [&pngBytes](const std::filesystem::path& path) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        expect(output.good(), "resource test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "resource test file should be written");
    };

    writePng(basePath);
    writePng(path2x);
    writePng(path3x);

    ui::ResourceManager& manager = ui::ResourceManager::instance();
    manager.clearSearchPaths();
    manager.addSearchPath(tempRoot.string());

    manager.setDevicePixelRatio(1.0f);
    expect(
        manager.resolveImagePath("status.png") == basePath.string(),
        "1x device pixel ratio should prefer base image");

    manager.setDevicePixelRatio(2.0f);
    expect(
        manager.resolveImagePath("status.png") == path2x.string(),
        "2x device pixel ratio should prefer @2x image");

    manager.setDevicePixelRatio(3.0f);
    expect(
        manager.resolveImagePath("status.png") == path3x.string(),
        "3x device pixel ratio should prefer @3x image");

    expect(std::filesystem::remove(path3x), "resource manager test should remove @3x file");
    expect(
        manager.resolveImagePath("status.png") == path2x.string(),
        "3x lookup should fall back to @2x when @3x is missing");

    const rendering::Image image = manager.getImage("status.png");
    expect(static_cast<bool>(image), "resource manager should load selected image");

    manager.clearSearchPaths();
    manager.setDevicePixelRatio(1.0f);
    expect(
        manager.resolveImagePath(basePath.string()) == basePath.string(),
        "absolute path should resolve directly without search path");

    std::filesystem::remove(basePath);
    std::filesystem::remove(path2x);
    std::filesystem::remove_all(tempRoot);
    return 0;
}
