#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "../../src/app/RuntimeHooks.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/platform/Window.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/ResourceManager.h"
#include "tinalux/ui/Widget.h"

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
    int framebufferWidth() const override
    {
        return static_cast<int>(std::lround(static_cast<float>(width_) * dpiScale_));
    }
    int framebufferHeight() const override
    {
        return static_cast<int>(std::lround(static_cast<float>(height_) * dpiScale_));
    }
    float dpiScale() const override { return dpiScale_; }

    void setClipboardText(const std::string& text) override
    {
        clipboardText_ = text;
    }

    std::string clipboardText() const override
    {
        return clipboardText_;
    }

    void setTextInputActive(bool active) override
    {
        textInputActive_ = active;
    }

    bool textInputActive() const override
    {
        return textInputActive_;
    }

    void setTextInputCursorRect(const std::optional<tinalux::core::Rect>& rect) override
    {
        textInputCursorRect_ = rect;
    }

    std::optional<tinalux::core::Rect> textInputCursorRect() const override
    {
        return textInputCursorRect_;
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
    float dpiScale_ = 2.0f;
    bool shouldClose_ = false;
    bool textInputActive_ = false;
    std::string clipboardText_;
    std::optional<tinalux::core::Rect> textInputCursorRect_;
    tinalux::platform::EventCallback callback_;
};

class DpiInputProbeWidget final : public tinalux::ui::Widget {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(40.0f, 30.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        Widget::arrange(bounds);
    }

    bool focusable() const override
    {
        return true;
    }

    bool wantsTextInput() const override
    {
        return true;
    }

    std::optional<tinalux::core::Rect> imeCursorRect() override
    {
        return tinalux::core::Rect::MakeXYWH(10.0f, 12.0f, 20.0f, 24.0f);
    }

    void onDraw(tinalux::rendering::Canvas&) override {}
};

std::shared_ptr<FakeWindow> gLastWindow;

std::unique_ptr<tinalux::platform::Window> createFakeWindow(const tinalux::platform::WindowConfig& config)
{
    auto window = std::make_unique<FakeWindow>(config);
    gLastWindow = std::shared_ptr<FakeWindow>(window.get(), [](FakeWindow*) {});
    return window;
}

tinalux::rendering::RenderContext createFakeContext(const tinalux::rendering::ContextConfig&)
{
    return tinalux::rendering::RenderAccess::makeContext(
        tinalux::rendering::Backend::Vulkan,
        nullptr,
        reinterpret_cast<void*>(0xD01),
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

    app::detail::RuntimeHooks hooks {
        .createWindow = &createFakeWindow,
        .createContext = &createFakeContext,
        .createWindowSurface = &createFakeWindowSurface,
    };
    app::detail::ScopedRuntimeHooksOverride scopedHooks(hooks);

    app::Application app;
    expect(
        app.init(app::ApplicationConfig {
            .window = platform::WindowConfig {
                .width = 480,
                .height = 320,
                .title = "ApplicationDpiSmoke",
            },
            .backend = rendering::Backend::Vulkan,
        }),
        "application dpi smoke should initialize");

    auto probe = std::make_shared<DpiInputProbeWidget>();
    app.setRootWidget(probe);

    expect(
        nearlyEqual(ui::ResourceManager::instance().devicePixelRatio(), 2.0f),
        "Application init should sync window dpi scale into ResourceManager");

    core::KeyEvent tabForward(
        core::keys::kTab,
        0,
        0,
        core::EventType::KeyPress);
    app.handleEvent(tabForward);

    expect(gLastWindow != nullptr, "fake window should stay available during dpi smoke");
    expect(gLastWindow->textInputActive(), "focused logical text widget should activate text input");
    expect(
        gLastWindow->textInputCursorRect().has_value(),
        "focused logical text widget should expose an IME rect");
    const core::Rect cursorRect = *gLastWindow->textInputCursorRect();
    expect(
        nearlyEqual(cursorRect.left(), 20.0f)
            && nearlyEqual(cursorRect.top(), 24.0f)
            && nearlyEqual(cursorRect.right(), 60.0f)
            && nearlyEqual(cursorRect.bottom(), 72.0f),
        "Application should convert logical IME cursor rects back into physical pixels");

    app.shutdown();
    return 0;
}
