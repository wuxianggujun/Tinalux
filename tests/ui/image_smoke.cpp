#include <array>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ImageWidget.h"

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

    const std::array<std::uint8_t, 32> pixels {
        255, 0, 0, 255,   0, 255, 0, 255,   0, 0, 255, 255,   255, 255, 0, 255,
        255, 0, 255, 255, 0, 255, 255, 255, 64, 64, 64, 255,  255, 255, 255, 255,
    };

    const rendering::Image invalidImage = rendering::createImageFromRGBA(
        2,
        2,
        std::span<const std::uint8_t>(pixels.data(), 3));
    expect(!invalidImage, "invalid RGBA buffer should not create image");

    const rendering::Image missingImage = rendering::loadImageFromFile("__missing_image_widget_test__.png");
    expect(!missingImage, "missing file should not load image");

    const rendering::Image invalidEncodedImage = rendering::loadImageFromEncoded({});
    expect(!invalidEncodedImage, "empty encoded bytes should not load image");

    const rendering::Image encodedImage = rendering::loadImageFromEncoded(pngBytes);
    expect(static_cast<bool>(encodedImage), "encoded PNG bytes should load image");
    expect(encodedImage.width() == 1 && encodedImage.height() == 1, "encoded PNG dimensions should be preserved");

    const std::filesystem::path cachePath =
        std::filesystem::temp_directory_path() / "tinalux_image_cache_test.png";
    {
        std::ofstream output(cachePath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "should be able to create temporary PNG file");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "temporary PNG file should be written successfully");
    }

    const rendering::Image fileImageFirst = rendering::loadImageFromFile(cachePath.string());
    expect(static_cast<bool>(fileImageFirst), "temporary PNG file should load successfully");
    expect(fileImageFirst.width() == 1 && fileImageFirst.height() == 1, "loaded PNG dimensions should match encoded data");
    const bool removedFile = std::filesystem::remove(cachePath);
    expect(removedFile, "temporary PNG file should be removable after first load");

    const rendering::Image fileImageCached = rendering::loadImageFromFile(cachePath.string());
    expect(static_cast<bool>(fileImageCached), "cached image load should succeed after source file is removed");
    expect(
        fileImageCached.width() == fileImageFirst.width() && fileImageCached.height() == fileImageFirst.height(),
        "cached image dimensions should stay stable");

    rendering::clearImageFileCache();
    const rendering::Image fileImageAfterClear = rendering::loadImageFromFile(cachePath.string());
    expect(!fileImageAfterClear, "cleared cache should not keep deleted file alive");

    const rendering::Image image = rendering::createImageFromRGBA(4, 2, pixels);
    expect(static_cast<bool>(image), "valid RGBA buffer should create image");
    expect(image.width() == 4 && image.height() == 2, "image dimensions should match source buffer");

    ui::ImageWidget widget(image);
    const core::Size naturalSize = widget.measure(ui::Constraints::loose(100.0f, 100.0f));
    expect(
        std::abs(naturalSize.width() - 4.0f) < 0.001f
            && std::abs(naturalSize.height() - 2.0f) < 0.001f,
        "image widget should default to natural image size");

    widget.setPreferredSize(core::Size::Make(40.0f, 0.0f));
    const core::Size widthOnlySize = widget.measure(ui::Constraints::loose(100.0f, 100.0f));
    expect(
        std::abs(widthOnlySize.width() - 40.0f) < 0.001f
            && std::abs(widthOnlySize.height() - 20.0f) < 0.001f,
        "preferred width should preserve aspect ratio");

    widget.setPreferredSize(core::Size::Make(30.0f, 12.0f));
    const core::Size explicitSize = widget.measure(ui::Constraints::loose(100.0f, 100.0f));
    expect(
        std::abs(explicitSize.width() - 30.0f) < 0.001f
            && std::abs(explicitSize.height() - 12.0f) < 0.001f,
        "explicit preferred size should override natural aspect size");

    widget.setFit(ui::ImageFit::Cover);
    widget.setOpacity(1.5f);
    expect(std::abs(widget.opacity() - 1.0f) < 0.001f, "opacity should clamp to 1");

    const rendering::RenderSurface surface = rendering::createRasterSurface(64, 64);
    expect(static_cast<bool>(surface), "raster surface should be created for image drawing");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "raster surface should expose a canvas");

    widget.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 32.0f, 32.0f));
    widget.draw(canvas);
    expect(!widget.isDirty(), "image widget should be clean after drawing");

    ui::Button plainButton("Open");
    const core::Size plainButtonSize = plainButton.measure(ui::Constraints::loose(200.0f, 80.0f));

    ui::Button iconButton("Open");
    iconButton.setIcon(image);
    iconButton.setIconSize(core::Size::Make(16.0f, 0.0f));
    const core::Size iconButtonSize = iconButton.measure(ui::Constraints::loose(200.0f, 80.0f));
    expect(iconButtonSize.width() > plainButtonSize.width(), "button icon should increase measured width");
    expect(iconButtonSize.height() >= plainButtonSize.height(), "button icon should not shrink measured height");

    iconButton.setIconPlacement(ui::ButtonIconPlacement::End);
    iconButton.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, iconButtonSize.width(), iconButtonSize.height()));
    iconButton.draw(canvas);
    expect(!iconButton.isDirty(), "icon button should be clean after drawing");

    return 0;
}
