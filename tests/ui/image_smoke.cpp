#include <array>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "../../src/rendering/RenderHandles.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ImageWidget.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/ResourceLoader.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
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

    const rendering::ImageCacheStats originalCacheStats = rendering::imageFileCacheStats();
    const std::filesystem::path lruPathA =
        std::filesystem::temp_directory_path() / "tinalux_image_cache_lru_a.png";
    const std::filesystem::path lruPathB =
        std::filesystem::temp_directory_path() / "tinalux_image_cache_lru_b.png";
    {
        std::ofstream outputA(lruPathA, std::ios::binary | std::ios::trunc);
        std::ofstream outputB(lruPathB, std::ios::binary | std::ios::trunc);
        expect(outputA.good() && outputB.good(), "should create temporary LRU cache test PNG files");
        outputA.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        outputB.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(outputA.good() && outputB.good(), "temporary LRU cache test PNG files should be written");
    }

    rendering::clearImageFileCache();
    rendering::setImageFileCacheLimits(1, originalCacheStats.maxMemoryUsage);

    const rendering::Image lruImageA = rendering::loadImageFromFile(lruPathA.string());
    expect(static_cast<bool>(lruImageA), "first LRU image should load");
    const rendering::ImageCacheStats afterFirstLoad = rendering::imageFileCacheStats();
    expect(afterFirstLoad.entryCount == 1, "cache should contain one entry after first image load");

    const rendering::Image lruImageAHit = rendering::loadImageFromFile(lruPathA.string());
    expect(static_cast<bool>(lruImageAHit), "cached LRU image should load");
    const rendering::ImageCacheStats afterCacheHit = rendering::imageFileCacheStats();
    expect(afterCacheHit.hitCount > afterFirstLoad.hitCount, "cache hit count should increase on repeated load");

    const rendering::Image lruImageB = rendering::loadImageFromFile(lruPathB.string());
    expect(static_cast<bool>(lruImageB), "second LRU image should load");
    const rendering::ImageCacheStats afterSecondLoad = rendering::imageFileCacheStats();
    expect(afterSecondLoad.entryCount == 1, "maxEntries limit should evict least recently used image");
    expect(afterSecondLoad.evictionCount > afterCacheHit.evictionCount, "loading beyond cache limit should evict one entry");

    expect(std::filesystem::remove(lruPathA), "first LRU cache test file should be removable");
    expect(std::filesystem::remove(lruPathB), "second LRU cache test file should be removable");

    const rendering::Image lruImageAEvicted = rendering::loadImageFromFile(lruPathA.string());
    expect(!lruImageAEvicted, "evicted image should not reload after source file is deleted");

    const rendering::Image lruImageBCached = rendering::loadImageFromFile(lruPathB.string());
    expect(static_cast<bool>(lruImageBCached), "most recent image should remain cached after source file is deleted");

    rendering::clearImageFileCache();
    rendering::setImageFileCacheLimits(
        originalCacheStats.maxEntries,
        originalCacheStats.maxMemoryUsage);

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

    ui::IconRegistry::instance().clear();
    ui::IconRegistry::instance().registerIconData(ui::IconType::Search, pngBytes);
    ui::Button registryButton("Open");
    registryButton.setIcon(ui::IconType::Search, 16.0f);
    registryButton.setIconSize(core::Size::Make(16.0f, 0.0f));
    const core::Size registryButtonSize = registryButton.measure(ui::Constraints::loose(200.0f, 80.0f));
    expect(static_cast<bool>(registryButton.icon()), "button should resolve icon directly from icon registry");
    expect(
        registryButtonSize.width() >= iconButtonSize.width(),
        "registry-backed button icon should reserve the same layout space as direct image icons");

    ui::ResourceLoader::instance().clear();
    const std::filesystem::path asyncWidgetPath =
        std::filesystem::temp_directory_path() / "tinalux_image_widget_async.png";
    {
        std::ofstream output(asyncWidgetPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async widget test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async widget test file should be written");
    }

    ui::ImageWidget asyncWidget;
    asyncWidget.setPlaceholderSize(core::Size::Make(18.0f, 12.0f));
    asyncWidget.setLoadingPlaceholderColor(core::colorARGB(255, 12, 70, 120));
    asyncWidget.loadImageAsync(asyncWidgetPath.string());
    expect(asyncWidget.loading(), "image widget should report loading after async request");
    expect(asyncWidget.loadState() == ui::ImageLoadState::Loading, "image widget should enter loading state");
    expect(asyncWidget.imagePath() == asyncWidgetPath.string(), "image widget should remember requested path");
    const core::Size loadingSize = asyncWidget.measure(ui::Constraints::loose(100.0f, 100.0f));
    expect(
        std::abs(loadingSize.width() - 18.0f) < 0.001f
            && std::abs(loadingSize.height() - 12.0f) < 0.001f,
        "loading image widget should use placeholder size when no preferred size is set");
    asyncWidget.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 24.0f, 24.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncWidget.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 12, 12), asyncWidget.loadingPlaceholderColor()),
        "loading image widget should render loading placeholder color");

    bool asyncLoaded = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (static_cast<bool>(asyncWidget.image())) {
            asyncLoaded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncLoaded, "image widget should receive async image");
    expect(!asyncWidget.loading(), "image widget should stop loading after async callback");
    expect(asyncWidget.loadState() == ui::ImageLoadState::Ready, "image widget should become ready after async load");
    expect(asyncWidget.image().width() == 1, "async image widget should expose loaded image");
    expect(std::filesystem::remove(asyncWidgetPath), "async widget test file should be removable");

    ui::ImageWidget failedWidget;
    failedWidget.setPlaceholderSize(core::Size::Make(20.0f, 14.0f));
    failedWidget.setFailedPlaceholderColor(core::colorARGB(255, 120, 24, 24));
    failedWidget.loadImageAsync("__missing_async_image_widget_test__.png");
    bool asyncFailed = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (!failedWidget.loading()
            && failedWidget.loadState() == ui::ImageLoadState::Failed) {
            asyncFailed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncFailed, "image widget should enter failed state when async load cannot resolve image");
    const core::Size failedSize = failedWidget.measure(ui::Constraints::loose(100.0f, 100.0f));
    expect(
        std::abs(failedSize.width() - 20.0f) < 0.001f
            && std::abs(failedSize.height() - 14.0f) < 0.001f,
        "failed image widget should keep placeholder size");
    failedWidget.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 24.0f, 24.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    failedWidget.draw(canvas);
    expect(
        nearlyEqualColor(readPixel(surface, 8, 8), failedWidget.failedPlaceholderColor()),
        "failed image widget should render failed placeholder color");

    ui::ResourceLoader::instance().clear();
    const std::filesystem::path asyncButtonPath =
        std::filesystem::temp_directory_path() / "tinalux_button_icon_async.png";
    {
        std::ofstream output(asyncButtonPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async button icon test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async button icon test file should be written");
    }

    ui::Button asyncButton("Load");
    asyncButton.setIconSize(core::Size::Make(16.0f, 0.0f));
    const core::Size plainAsyncButtonSize = asyncButton.measure(ui::Constraints::loose(240.0f, 80.0f));
    asyncButton.loadIconAsync(asyncButtonPath.string());
    expect(asyncButton.iconLoading(), "button should report loading after async icon request");
    expect(
        asyncButton.iconLoadState() == ui::ButtonIconLoadState::Loading,
        "button should enter loading state while async icon is in flight");
    expect(asyncButton.iconPath() == asyncButtonPath.string(), "button should remember requested icon path");
    const core::Size loadingButtonSize = asyncButton.measure(ui::Constraints::loose(240.0f, 80.0f));
    expect(
        loadingButtonSize.width() > plainAsyncButtonSize.width(),
        "button should reserve icon layout space while async icon is still loading");
    asyncButton.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, loadingButtonSize.width(), loadingButtonSize.height()));
    asyncButton.draw(canvas);
    expect(!asyncButton.isDirty(), "button should draw cleanly during async icon loading");

    bool asyncButtonLoaded = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (static_cast<bool>(asyncButton.icon())) {
            asyncButtonLoaded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncButtonLoaded, "button should receive async icon");
    expect(!asyncButton.iconLoading(), "button should stop reporting loading after async icon callback");
    expect(
        asyncButton.iconLoadState() == ui::ButtonIconLoadState::Ready,
        "button should become ready after async icon load succeeds");
    expect(asyncButton.icon().width() == 1, "button should expose async-loaded icon image");
    expect(std::filesystem::remove(asyncButtonPath), "async button icon test file should be removable");

    ui::Button failedButton("Retry");
    failedButton.setIconSize(core::Size::Make(16.0f, 0.0f));
    const core::Size plainFailedButtonSize = failedButton.measure(ui::Constraints::loose(240.0f, 80.0f));
    failedButton.loadIconAsync("__missing_async_button_icon_test__.png");
    bool failedButtonResolved = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (!failedButton.iconLoading()
            && failedButton.iconLoadState() == ui::ButtonIconLoadState::Failed) {
            failedButtonResolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(failedButtonResolved, "button should enter failed state when async icon cannot load");
    const core::Size failedButtonSize = failedButton.measure(ui::Constraints::loose(240.0f, 80.0f));
    expect(
        failedButtonSize.width() > plainFailedButtonSize.width(),
        "button should keep icon slot reserved after async icon load fails");
    failedButton.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, failedButtonSize.width(), failedButtonSize.height()));
    failedButton.draw(canvas);
    expect(!failedButton.isDirty(), "button should draw cleanly in failed icon state");

    ui::Checkbox registryCheckbox("", true);
    registryCheckbox.setStyle({
        .uncheckedBackgroundColor = { .normal = core::colorRGB(26, 32, 40) },
        .checkedBackgroundColor = { .normal = core::colorRGB(64, 112, 220) },
        .borderColor = { .normal = core::colorARGB(0, 0, 0, 0) },
        .borderWidth = { .normal = 0.0f },
        .labelColor = { .normal = core::colorRGB(255, 255, 255) },
        .textStyle = { .fontSize = 16.0f },
        .checkmarkColor = core::colorRGB(16, 18, 24),
        .indicatorSize = 22.0f,
        .indicatorRadius = 0.0f,
        .labelGap = 0.0f,
        .checkmarkStrokeWidth = 2.5f,
        .minHeight = 22.0f,
    });
    registryCheckbox.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 22.0f, 22.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    registryCheckbox.draw(canvas);
    const core::Color fallbackCheckboxCenter = readPixel(surface, 11, 11);

    ui::IconRegistry::instance().registerIconData(ui::IconType::Check, pngBytes);
    registryCheckbox.setCheckmarkIcon(ui::IconType::Check, 16.0f);
    expect(static_cast<bool>(registryCheckbox.checkmarkIcon()), "checkbox should resolve checkmark icon from icon registry");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    registryCheckbox.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 11, 11), fallbackCheckboxCenter),
        "registry-backed checkbox should replace fallback checkmark rendering");

    ui::ResourceLoader::instance().clear();
    const std::filesystem::path asyncCheckboxPath =
        std::filesystem::temp_directory_path() / "tinalux_checkbox_icon_async.png";
    {
        std::ofstream output(asyncCheckboxPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async checkbox icon test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async checkbox icon test file should be written");
    }

    ui::Checkbox asyncCheckbox("", true);
    asyncCheckbox.setStyle({
        .uncheckedBackgroundColor = { .normal = core::colorRGB(26, 32, 40) },
        .checkedBackgroundColor = { .normal = core::colorRGB(72, 128, 228) },
        .borderColor = { .normal = core::colorARGB(0, 0, 0, 0) },
        .borderWidth = { .normal = 0.0f },
        .labelColor = { .normal = core::colorRGB(255, 255, 255) },
        .textStyle = { .fontSize = 16.0f },
        .checkmarkColor = core::colorRGB(16, 18, 24),
        .indicatorSize = 22.0f,
        .indicatorRadius = 0.0f,
        .labelGap = 0.0f,
        .checkmarkStrokeWidth = 2.5f,
        .minHeight = 22.0f,
    });
    asyncCheckbox.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 22.0f, 22.0f));
    asyncCheckbox.loadCheckmarkIconAsync(asyncCheckboxPath.string());
    expect(asyncCheckbox.checkmarkIconLoading(), "checkbox should report loading after async checkmark request");
    expect(
        asyncCheckbox.checkmarkIconLoadState() == ui::CheckboxIconLoadState::Loading,
        "checkbox should enter loading state while async checkmark is in flight");
    expect(
        asyncCheckbox.checkmarkIconPath() == asyncCheckboxPath.string(),
        "checkbox should remember requested checkmark icon path");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncCheckbox.draw(canvas);
    const core::Color loadingCheckboxCenter = readPixel(surface, 11, 11);

    bool asyncCheckboxLoaded = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (static_cast<bool>(asyncCheckbox.checkmarkIcon())) {
            asyncCheckboxLoaded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncCheckboxLoaded, "checkbox should receive async checkmark icon");
    expect(!asyncCheckbox.checkmarkIconLoading(), "checkbox should stop reporting loading after async checkmark callback");
    expect(
        asyncCheckbox.checkmarkIconLoadState() == ui::CheckboxIconLoadState::Ready,
        "checkbox should become ready after async checkmark load succeeds");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncCheckbox.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 11, 11), loadingCheckboxCenter),
        "checkbox should render async-loaded checkmark icon after completion");
    expect(std::filesystem::remove(asyncCheckboxPath), "async checkbox icon test file should be removable");

    ui::Checkbox failedCheckbox("", true);
    failedCheckbox.setStyle({
        .uncheckedBackgroundColor = { .normal = core::colorRGB(26, 32, 40) },
        .checkedBackgroundColor = { .normal = core::colorRGB(72, 128, 228) },
        .borderColor = { .normal = core::colorARGB(0, 0, 0, 0) },
        .borderWidth = { .normal = 0.0f },
        .labelColor = { .normal = core::colorRGB(255, 255, 255) },
        .textStyle = { .fontSize = 16.0f },
        .checkmarkColor = core::colorRGB(16, 18, 24),
        .indicatorSize = 22.0f,
        .indicatorRadius = 0.0f,
        .labelGap = 0.0f,
        .checkmarkStrokeWidth = 2.5f,
        .minHeight = 22.0f,
    });
    failedCheckbox.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 22.0f, 22.0f));
    failedCheckbox.loadCheckmarkIconAsync("__missing_async_checkbox_icon_test__.png");
    bool failedCheckboxResolved = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (!failedCheckbox.checkmarkIconLoading()
            && failedCheckbox.checkmarkIconLoadState() == ui::CheckboxIconLoadState::Failed) {
            failedCheckboxResolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(failedCheckboxResolved, "checkbox should enter failed state when async checkmark icon cannot load");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    failedCheckbox.draw(canvas);
    expect(!failedCheckbox.isDirty(), "checkbox should draw cleanly in failed checkmark icon state");

    ui::Radio registryRadio("", "resource-group", true);
    registryRadio.setStyle({
        .unselectedBorderColor = { .normal = core::colorRGB(44, 80, 128) },
        .selectedBorderColor = { .normal = core::colorRGB(72, 128, 228) },
        .borderWidth = { .normal = 4.0f },
        .labelColor = { .normal = core::colorRGB(255, 255, 255) },
        .dotColor = core::colorRGB(18, 22, 30),
        .textStyle = { .fontSize = 16.0f },
        .indicatorSize = 22.0f,
        .innerDotSize = 10.0f,
        .labelGap = 0.0f,
        .minHeight = 22.0f,
    });
    registryRadio.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 22.0f, 22.0f));
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    registryRadio.draw(canvas);
    const core::Color fallbackRadioCenter = readPixel(surface, 11, 11);

    ui::IconRegistry::instance().registerIconData(ui::IconType::Clear, pngBytes);
    registryRadio.setSelectionIcon(ui::IconType::Clear, 10.0f);
    expect(static_cast<bool>(registryRadio.selectionIcon()), "radio should resolve selection icon from icon registry");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    registryRadio.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 11, 11), fallbackRadioCenter),
        "registry-backed radio should replace fallback selection dot rendering");

    ui::ResourceLoader::instance().clear();
    const std::filesystem::path asyncRadioPath =
        std::filesystem::temp_directory_path() / "tinalux_radio_icon_async.png";
    {
        std::ofstream output(asyncRadioPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async radio icon test file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async radio icon test file should be written");
    }

    ui::Radio asyncRadio("", "resource-group-async", true);
    asyncRadio.setStyle({
        .unselectedBorderColor = { .normal = core::colorRGB(44, 80, 128) },
        .selectedBorderColor = { .normal = core::colorRGB(72, 128, 228) },
        .borderWidth = { .normal = 4.0f },
        .labelColor = { .normal = core::colorRGB(255, 255, 255) },
        .dotColor = core::colorRGB(18, 22, 30),
        .textStyle = { .fontSize = 16.0f },
        .indicatorSize = 22.0f,
        .innerDotSize = 10.0f,
        .labelGap = 0.0f,
        .minHeight = 22.0f,
    });
    asyncRadio.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 22.0f, 22.0f));
    asyncRadio.loadSelectionIconAsync(asyncRadioPath.string());
    expect(asyncRadio.selectionIconLoading(), "radio should report loading after async selection icon request");
    expect(
        asyncRadio.selectionIconLoadState() == ui::RadioIndicatorLoadState::Loading,
        "radio should enter loading state while async selection icon is in flight");
    expect(
        asyncRadio.selectionIconPath() == asyncRadioPath.string(),
        "radio should remember requested selection icon path");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncRadio.draw(canvas);
    const core::Color loadingRadioCenter = readPixel(surface, 11, 11);

    bool asyncRadioLoaded = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (static_cast<bool>(asyncRadio.selectionIcon())) {
            asyncRadioLoaded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(asyncRadioLoaded, "radio should receive async selection icon");
    expect(!asyncRadio.selectionIconLoading(), "radio should stop reporting loading after async selection icon callback");
    expect(
        asyncRadio.selectionIconLoadState() == ui::RadioIndicatorLoadState::Ready,
        "radio should become ready after async selection icon load succeeds");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    asyncRadio.draw(canvas);
    expect(
        !nearlyEqualColor(readPixel(surface, 11, 11), loadingRadioCenter),
        "radio should render async-loaded selection icon after completion");
    expect(std::filesystem::remove(asyncRadioPath), "async radio icon test file should be removable");

    ui::Radio failedRadio("", "resource-group-failed", true);
    failedRadio.setStyle({
        .unselectedBorderColor = { .normal = core::colorRGB(44, 80, 128) },
        .selectedBorderColor = { .normal = core::colorRGB(72, 128, 228) },
        .borderWidth = { .normal = 4.0f },
        .labelColor = { .normal = core::colorRGB(255, 255, 255) },
        .dotColor = core::colorRGB(18, 22, 30),
        .textStyle = { .fontSize = 16.0f },
        .indicatorSize = 22.0f,
        .innerDotSize = 10.0f,
        .labelGap = 0.0f,
        .minHeight = 22.0f,
    });
    failedRadio.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 22.0f, 22.0f));
    failedRadio.loadSelectionIconAsync("__missing_async_radio_icon_test__.png");
    bool failedRadioResolved = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        ui::ResourceLoader::instance().pumpCallbacks();
        if (!failedRadio.selectionIconLoading()
            && failedRadio.selectionIconLoadState() == ui::RadioIndicatorLoadState::Failed) {
            failedRadioResolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(failedRadioResolved, "radio should enter failed state when async selection icon cannot load");
    canvas.clear(core::colorARGB(0, 0, 0, 0));
    failedRadio.draw(canvas);
    expect(!failedRadio.isDirty(), "radio should draw cleanly in failed selection icon state");

    return 0;
}
