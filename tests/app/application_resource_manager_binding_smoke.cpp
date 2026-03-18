#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "../../src/app/RuntimeHooks.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/app/Application.h"
#include "tinalux/ui/ResourceManager.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

void destroyFakeHandle(void*, void*) {}

std::vector<float> gWindowDpiScales;
std::size_t gWindowCreateIndex = 0;

class FakeWindow final : public tinalux::platform::Window {
public:
    explicit FakeWindow(const tinalux::platform::WindowConfig& config, float dpiScale)
        : width_(config.width)
        , height_(config.height)
        , dpiScale_(dpiScale)
    {
    }

    bool shouldClose() const override { return shouldClose_; }
    void pollEvents() override {}
    void waitEventsTimeout(double) override {}
    void swapBuffers() override {}
    void requestClose() override { shouldClose_ = true; }

    int width() const override { return width_; }
    int height() const override { return height_; }
    int framebufferWidth() const override
    {
        return static_cast<int>(std::lround(static_cast<float>(width_) * dpiScale_));
    }
    int framebufferHeight() const override
    {
        return static_cast<int>(std::lround(static_cast<float>(height_) * dpiScale_));
    }
    float dpiScale() const override { return dpiScale_; }

    void setClipboardText(const std::string&) override {}
    std::string clipboardText() const override { return {}; }

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
    float dpiScale_ = 1.0f;
    bool shouldClose_ = false;
    tinalux::platform::EventCallback callback_;
};

std::unique_ptr<tinalux::platform::Window> createFakeWindow(
    const tinalux::platform::WindowConfig& config)
{
    expect(
        gWindowCreateIndex < gWindowDpiScales.size(),
        "application resource manager smoke should provide enough fake window dpi scales");
    return std::make_unique<FakeWindow>(config, gWindowDpiScales[gWindowCreateIndex++]);
}

tinalux::rendering::RenderContext createFakeContext(const tinalux::rendering::ContextConfig&)
{
    return tinalux::rendering::RenderAccess::makeContext(
        tinalux::rendering::Backend::Vulkan,
        nullptr,
        reinterpret_cast<void*>(0xFACE),
        &destroyFakeHandle,
        nullptr);
}

tinalux::rendering::RenderSurface createFakeWindowSurface(
    tinalux::rendering::RenderContext&,
    tinalux::platform::Window& window)
{
    return tinalux::rendering::createRasterSurface(
        window.framebufferWidth(),
        window.framebufferHeight());
}

}  // namespace

int main()
{
    using namespace tinalux;

    gWindowDpiScales = {2.0f, 3.0f};
    gWindowCreateIndex = 0;
    ui::ResourceManager::instance().setDevicePixelRatio(1.0f);

    app::detail::RuntimeHooks hooks {
        .createWindow = &createFakeWindow,
        .createContext = &createFakeContext,
        .createWindowSurface = &createFakeWindowSurface,
    };
    app::detail::ScopedRuntimeHooksOverride scopedHooks(hooks);

    app::Application first;
    expect(first.init(app::ApplicationConfig {}), "first application should initialize");
    expect(
        nearlyEqual(ui::ResourceManager::instance().devicePixelRatio(), 2.0f),
        "first application should bind its dpi scale into ResourceManager");

    app::Application second;
    expect(second.init(app::ApplicationConfig {}), "second application should initialize");
    expect(
        nearlyEqual(ui::ResourceManager::instance().devicePixelRatio(), 3.0f),
        "newest application should override ResourceManager dpi scale while active");

    second.shutdown();
    expect(
        nearlyEqual(ui::ResourceManager::instance().devicePixelRatio(), 2.0f),
        "shutting down the newest application should fall back to the previous dpi binding");

    first.shutdown();
    expect(
        nearlyEqual(ui::ResourceManager::instance().devicePixelRatio(), 1.0f),
        "removing every application dpi binding should fall back to the base ResourceManager ratio");
    return 0;
}
