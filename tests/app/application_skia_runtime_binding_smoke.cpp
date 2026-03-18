#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../../src/app/RuntimeHooks.h"
#include "../../src/rendering/FontCache.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/app/Application.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void destroyFakeHandle(void*, void*) {}

class FakeWindow final : public tinalux::platform::Window {
public:
    explicit FakeWindow(const tinalux::platform::WindowConfig& config)
        : width_(config.width)
        , height_(config.height)
    {
    }

    bool shouldClose() const override { return shouldClose_; }
    void pollEvents() override {}
    void waitEventsTimeout(double) override {}
    void swapBuffers() override {}
    void requestClose() override { shouldClose_ = true; }

    int width() const override { return width_; }
    int height() const override { return height_; }
    int framebufferWidth() const override { return width_; }
    int framebufferHeight() const override { return height_; }
    float dpiScale() const override { return 1.0f; }

    void setClipboardText(const std::string& text) override
    {
        clipboardText_ = text;
    }

    std::string clipboardText() const override
    {
        return clipboardText_;
    }

    void setEventCallback(tinalux::platform::EventCallback callback) override
    {
        callback_ = std::move(callback);
    }

    tinalux::platform::GLGetProcFn glGetProcAddress() const override { return nullptr; }
    bool vulkanSupported() const override { return true; }
    std::vector<std::string> requiredVulkanInstanceExtensions() const override { return {}; }
    tinalux::platform::VulkanGetInstanceProcFn vulkanGetInstanceProcAddress() const override
    {
        return nullptr;
    }

private:
    int width_ = 0;
    int height_ = 0;
    bool shouldClose_ = false;
    std::string clipboardText_;
    tinalux::platform::EventCallback callback_;
};

std::vector<std::shared_ptr<FakeWindow>> gWindows;

std::unique_ptr<tinalux::platform::Window> createFakeWindow(
    const tinalux::platform::WindowConfig& config)
{
    auto window = std::make_unique<FakeWindow>(config);
    gWindows.push_back(std::shared_ptr<FakeWindow>(window.get(), [](FakeWindow*) {}));
    return window;
}

tinalux::rendering::RenderContext createFakeContext(const tinalux::rendering::ContextConfig&)
{
    return tinalux::rendering::RenderAccess::makeContext(
        tinalux::rendering::Backend::Vulkan,
        nullptr,
        reinterpret_cast<void*>(0xBEEF),
        &destroyFakeHandle,
        nullptr);
}

tinalux::rendering::RenderSurface createFakeWindowSurface(
    tinalux::rendering::RenderContext&,
    tinalux::platform::Window&)
{
    return tinalux::rendering::createRasterSurface(64, 64);
}

}  // namespace

int main()
{
    using namespace tinalux;

    rendering::clearCachedFonts();
    expect(rendering::cachedFontCount() == 0, "font cache should start empty for the Skia binding smoke");

    app::detail::RuntimeHooks hooks {
        .createWindow = &createFakeWindow,
        .createContext = &createFakeContext,
        .createWindowSurface = &createFakeWindowSurface,
    };
    app::detail::ScopedRuntimeHooksOverride scopedHooks(hooks);

    app::Application first;
    expect(first.init(app::ApplicationConfig {}), "first application should initialize");
    app::Application second;
    expect(second.init(app::ApplicationConfig {}), "second application should initialize");

    (void)rendering::cachedFont(16.0f);
    expect(
        rendering::cachedFontCount() == 1,
        "font cache should populate while multiple applications share the Skia runtime");

    first.shutdown();
    expect(
        rendering::cachedFontCount() == 1,
        "shutting down one application should not purge shared Skia font cache while another application is alive");

    (void)rendering::cachedFont(18.0f);
    expect(
        rendering::cachedFontCount() == 2,
        "remaining application should keep using the shared font cache after another application shuts down");

    second.shutdown();
    expect(
        rendering::cachedFontCount() == 0,
        "last application shutdown should release the shared Skia font cache");
    return 0;
}
