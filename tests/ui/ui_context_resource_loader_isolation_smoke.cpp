#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include "../../src/app/UIContext.h"
#include "tinalux/ui/ImageWidget.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ResourceLoader.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool waitUntil(const std::function<bool()>& predicate, int attempts = 200)
{
    for (int index = 0; index < attempts; ++index) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return predicate();
}

std::shared_ptr<tinalux::ui::ImageWidget> makeAsyncImageRoot(
    const std::filesystem::path& path,
    std::shared_ptr<tinalux::ui::Panel>& root)
{
    root = std::make_shared<tinalux::ui::Panel>();
    auto image = std::make_shared<tinalux::ui::ImageWidget>();
    root->addChild(image);
    root->arrange(tinalux::core::Rect::MakeXYWH(0.0f, 0.0f, 160.0f, 120.0f));
    image->arrange(tinalux::core::Rect::MakeXYWH(8.0f, 8.0f, 48.0f, 48.0f));
    image->loadImageAsync(path.string());
    return image;
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

    const std::filesystem::path asyncPath =
        std::filesystem::temp_directory_path() / "tinalux_ui_context_resource_loader_isolation.png";
    {
        std::ofstream output(asyncPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "ui context resource loader isolation file should open");
        output.write(
            reinterpret_cast<const char*>(pngBytes.data()),
            static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "ui context resource loader isolation file should be written");
    }

    ui::ResourceLoader::instance().clear();

    app::UIContext first;
    app::UIContext second;
    first.initializeFromEnvironment();
    second.initializeFromEnvironment();

    std::shared_ptr<ui::Panel> firstRoot;
    std::shared_ptr<ui::Panel> secondRoot;
    makeAsyncImageRoot(asyncPath, firstRoot);
    auto secondImage = makeAsyncImageRoot(asyncPath, secondRoot);
    first.setRootWidget(firstRoot);
    second.setRootWidget(secondRoot);

    first.shutdown();

    const bool secondLoaded = waitUntil([&second, &secondImage] {
        second.tickAsyncResources();
        return secondImage->loadState() == ui::ImageLoadState::Ready && static_cast<bool>(secondImage->image());
    });
    expect(
        secondLoaded,
        "shutting down one UIContext should not clear ResourceLoader callbacks needed by another live UIContext");

    second.shutdown();
    ui::ResourceLoader::instance().clear();
    std::filesystem::remove(asyncPath);
    return 0;
}
