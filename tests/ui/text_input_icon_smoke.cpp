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
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ResourceLoader.h"
#include "tinalux/ui/TextInput.h"

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

tinalux::ui::TextInputStyle iconStyle()
{
    tinalux::ui::TextInputStyle style;
    style.backgroundColor.normal = tinalux::core::colorRGB(18, 24, 34);
    style.backgroundColor.hovered = tinalux::core::colorRGB(18, 24, 34);
    style.backgroundColor.focused = tinalux::core::colorRGB(18, 24, 34);
    style.borderColor.normal = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderColor.hovered = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderColor.focused = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderWidth.normal = 0.0f;
    style.borderWidth.hovered = 0.0f;
    style.borderWidth.focused = 0.0f;
    style.textStyle.fontSize = 16.0f;
    style.textColor = tinalux::core::colorARGB(0, 0, 0, 0);
    style.placeholderColor = tinalux::core::colorRGB(84, 132, 210);
    style.selectionColor = tinalux::core::colorARGB(0, 0, 0, 0);
    style.caretColor = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderRadius = 0.0f;
    style.paddingHorizontal = 12.0f;
    style.paddingVertical = 8.0f;
    style.minWidth = 0.0f;
    style.minHeight = 40.0f;
    return style;
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

    const rendering::RenderSurface surface = rendering::createRasterSurface(220, 80);
    expect(static_cast<bool>(surface), "text input icon smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "text input icon smoke should expose canvas");

    ui::TextInput plainInput("Search");
    plainInput.setStyle(iconStyle());
    plainInput.setText("abc");
    const core::Size plainSize = plainInput.measure(ui::Constraints::loose(280.0f, 80.0f));
    plainInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 40.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    plainInput.draw(canvas);
    const core::Color plainLeadingPixel = readPixel(surface, 20, 20);

    ui::IconRegistry::instance().clear();
    ui::IconRegistry::instance().registerIconFactory(ui::IconType::Search, [](float) {
        const std::array<std::uint8_t, 4> rgba { 255, 210, 60, 255 };
        return tinalux::rendering::createImageFromRGBA(1, 1, rgba);
    });
    ui::TextInput registryInput("Search");
    registryInput.setStyle(iconStyle());
    registryInput.setText("abc");
    registryInput.setLeadingIcon(ui::IconType::Search, 16.0f);
    const core::Size registrySize = registryInput.measure(ui::Constraints::loose(280.0f, 80.0f));
    expect(static_cast<bool>(registryInput.leadingIcon()), "text input should resolve leading icon from icon registry");
    expect(registrySize.width() > plainSize.width(), "leading icon should increase measured width");
    registryInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 40.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    registryInput.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 20, 20), plainLeadingPixel),
        "registry-backed leading icon should replace plain background pixel");

    ui::ResourceLoader::instance().clear();
    const std::filesystem::path asyncIconPath =
        std::filesystem::temp_directory_path() / "tinalux_text_input_icon_async.png";
    {
        std::ofstream output(asyncIconPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async text input icon test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async text input icon test file should be written");
    }

    ui::TextInput asyncInput("Search");
    asyncInput.setStyle(iconStyle());
    asyncInput.setText("abc");
    const core::Size asyncPlainSize = asyncInput.measure(ui::Constraints::loose(280.0f, 80.0f));
    asyncInput.loadTrailingIconAsync(asyncIconPath.string());
    expect(asyncInput.trailingIconLoading(), "text input should report loading after async trailing icon request");
    expect(
        asyncInput.trailingIconLoadState() == ui::TextInputIconLoadState::Loading,
        "text input should enter loading state while trailing icon is in flight");
    expect(
        asyncInput.trailingIconPath() == asyncIconPath.string(),
        "text input should remember requested trailing icon path");
    const core::Size asyncLoadingSize = asyncInput.measure(ui::Constraints::loose(280.0f, 80.0f));
    expect(asyncLoadingSize.width() > asyncPlainSize.width(), "loading trailing icon should reserve layout space");
    asyncInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 40.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncInput.draw(canvas);
    const core::Color loadingTrailingPixel = readPixel(surface, 160, 20);

    bool asyncLoaded = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (static_cast<bool>(asyncInput.trailingIcon())) {
            asyncLoaded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncLoaded, "text input should receive async trailing icon");
    expect(!asyncInput.trailingIconLoading(), "text input should stop reporting loading after async trailing icon callback");
    expect(
        asyncInput.trailingIconLoadState() == ui::TextInputIconLoadState::Ready,
        "text input should become ready after async trailing icon load succeeds");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncInput.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 160, 20), loadingTrailingPixel),
        "text input should render async-loaded trailing icon after completion");
    expect(std::filesystem::remove(asyncIconPath), "async text input icon test file should be removable");

    ui::TextInput failedInput("Search");
    failedInput.setStyle(iconStyle());
    failedInput.setText("abc");
    failedInput.loadTrailingIconAsync("__missing_async_text_input_icon_test__.png");
    bool failedResolved = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (!failedInput.trailingIconLoading()
            && failedInput.trailingIconLoadState() == ui::TextInputIconLoadState::Failed) {
            failedResolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(failedResolved, "text input should enter failed state when async trailing icon cannot load");
    failedInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 40.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    failedInput.draw(canvas);
    expect(!failedInput.isDirty(), "text input should draw cleanly in failed trailing icon state");

    bool trailingClicked = false;
    ui::TextInput interactiveInput("Search");
    interactiveInput.setStyle(iconStyle());
    interactiveInput.setText("query");
    interactiveInput.setTrailingIcon(ui::IconType::Search, 16.0f);
    interactiveInput.onTrailingIconClick([&interactiveInput, &trailingClicked] {
        trailingClicked = true;
        interactiveInput.setText("");
    });
    interactiveInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 40.0f));
    core::MouseButtonEvent iconPress(
        core::mouse::kLeft,
        0,
        160.0,
        20.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent iconRelease(
        core::mouse::kLeft,
        0,
        160.0,
        20.0,
        core::EventType::MouseButtonRelease);
    expect(interactiveInput.onEvent(iconPress), "trailing icon press should be handled by text input");
    expect(interactiveInput.onEvent(iconRelease), "trailing icon release should be handled by text input");
    expect(trailingClicked, "trailing icon click callback should run");
    expect(interactiveInput.text().empty(), "trailing icon callback should be able to clear text");

    return 0;
}
