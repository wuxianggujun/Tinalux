#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "tinalux/rendering/rendering.h"
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

    ui::ResourceLoader& loader = ui::ResourceLoader::instance();
    loader.clear();

    const std::filesystem::path asyncPath =
        std::filesystem::temp_directory_path() / "tinalux_async_resource_loader.png";
    {
        std::ofstream output(asyncPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "async resource file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "async resource file should be written");
    }

    bool callbackCalled = false;
    int callbackWidth = 0;
    ui::ResourceHandle<rendering::Image> handle = loader.loadImageAsync(asyncPath.string());
    handle.onReady([&callbackCalled, &callbackWidth](const rendering::Image& image) {
        callbackCalled = true;
        callbackWidth = image.width();
    });

    const bool completed = waitUntil([&loader, &handle, &callbackCalled] {
        loader.pumpCallbacks();
        return handle.isReady() && callbackCalled;
    });
    expect(completed, "async image load should complete and invoke callback");
    expect(handle.get().width() == 1 && handle.get().height() == 1, "async handle should expose loaded image");
    expect(callbackWidth == 1, "async callback should receive loaded image");

    int sharedCallbackCount = 0;
    ui::ResourceHandle<rendering::Image> sharedA = loader.loadImageAsync(asyncPath.string());
    ui::ResourceHandle<rendering::Image> sharedB = loader.loadImageAsync(asyncPath.string());
    sharedA.onReady([&sharedCallbackCount](const rendering::Image&) { ++sharedCallbackCount; });
    sharedB.onReady([&sharedCallbackCount](const rendering::Image&) { ++sharedCallbackCount; });
    const bool sharedCompleted = waitUntil([&loader, &sharedA, &sharedB, &sharedCallbackCount] {
        loader.pumpCallbacks();
        return sharedA.isReady() && sharedB.isReady() && sharedCallbackCount == 2;
    });
    expect(sharedCompleted, "concurrent async loads should complete for all waiting handles");

    loader.clear();
    const std::filesystem::path preloadPath =
        std::filesystem::temp_directory_path() / "tinalux_async_preload.png";
    {
        std::ofstream output(preloadPath, std::ios::binary | std::ios::trunc);
        expect(output.good(), "preload file should open");
        output.write(reinterpret_cast<const char*>(pngBytes.data()), static_cast<std::streamsize>(pngBytes.size()));
        expect(output.good(), "preload file should be written");
    }

    loader.preload({ preloadPath.string() });
    const bool preloadCompleted = waitUntil([&loader] {
        return loader.pumpCallbacks();
    });
    expect(preloadCompleted || rendering::loadImageFromFile(preloadPath.string()), "preload should complete or warm image cache");
    expect(std::filesystem::remove(preloadPath), "preload source should be removable after async load");
    const rendering::Image cachedImage = rendering::loadImageFromFile(preloadPath.string());
    expect(static_cast<bool>(cachedImage), "preload should warm rendering cache for later sync access");

    loader.clear();
    std::filesystem::remove(asyncPath);
    return 0;
}
