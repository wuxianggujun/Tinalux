#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../../src/app/RuntimeHooks.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/app/Application.h"
#include "tinalux/ui/Clipboard.h"

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
        reinterpret_cast<void*>(0xCAFE),
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

    app::detail::RuntimeHooks hooks {
        .createWindow = &createFakeWindow,
        .createContext = &createFakeContext,
        .createWindowSurface = &createFakeWindowSurface,
    };
    app::detail::ScopedRuntimeHooksOverride scopedHooks(hooks);

    app::Application first;
    expect(first.init(app::ApplicationConfig {}), "first application should initialize");
    expect(gWindows.size() == 1, "first application should create one fake window");
    gWindows[0]->setClipboardText("first");

    expect(ui::hasClipboardHandler(), "first application should install clipboard handlers");
    expect(ui::clipboardText() == "first", "clipboard should read from the first application while it is active");

    app::Application second;
    expect(second.init(app::ApplicationConfig {}), "second application should initialize");
    expect(gWindows.size() == 2, "second application should create a second fake window");
    gWindows[1]->setClipboardText("second");

    expect(ui::clipboardText() == "second", "newest application clipboard binding should become active");
    ui::setClipboardText("updated-second");
    expect(
        gWindows[1]->clipboardText() == "updated-second",
        "clipboard writes should go to the newest active application");
    expect(
        gWindows[0]->clipboardText() == "first",
        "clipboard writes should not mutate older inactive applications");

    second.shutdown();
    expect(ui::hasClipboardHandler(), "shutting down one application should preserve older clipboard bindings");
    expect(
        ui::clipboardText() == "first",
        "clipboard binding should fall back to the previous application after the newest one shuts down");

    ui::setClipboardText("updated-first");
    expect(
        gWindows[0]->clipboardText() == "updated-first",
        "clipboard writes should fall back to the previous active application");

    first.shutdown();
    expect(!ui::hasClipboardHandler(), "clipboard handlers should clear once every application detaches");
    expect(ui::clipboardText().empty(), "clipboard reads should be empty after every application detaches");
    return 0;
}
