#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "tinalux/app/Application.h"
#include "tinalux/app/android/AndroidRuntime.h"

#include "../../src/app/RuntimeHooks.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/platform/Window.h"
#include "tinalux/rendering/rendering.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

using tinalux::platform::GraphicsAPI;
using tinalux::rendering::Backend;

struct WindowCreateRecord {
    GraphicsAPI graphicsApi = GraphicsAPI::OpenGL;
    int width = 0;
    int height = 0;
    void* nativeWindow = nullptr;
    float dpiScale = 1.0f;
};

std::vector<WindowCreateRecord> gWindowCreates;
std::vector<Backend> gContextRequests;
std::size_t gContextCreates = 0;
std::size_t gSurfaceCreates = 0;
bool gShouldCloseWindow = false;

void destroyFakeHandle(void*, void*) {}

tinalux::rendering::RenderContext makeFakeContext(std::uintptr_t token)
{
    return tinalux::rendering::RenderAccess::makeContext(
        Backend::Vulkan,
        nullptr,
        reinterpret_cast<void*>(token),
        &destroyFakeHandle,
        nullptr);
}

class FakeWindow final : public tinalux::platform::Window {
public:
    explicit FakeWindow(const tinalux::platform::WindowConfig& config)
        : graphicsApi_(config.graphicsApi)
        , width_(config.width)
        , height_(config.height)
        , nativeWindow_(config.android.has_value() ? config.android->nativeWindow : nullptr)
        , dpiScale_(config.android.has_value() ? config.android->dpiScale : 1.0f)
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
    float dpiScale() const override { return dpiScale_; }

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

    tinalux::platform::GLGetProcFn glGetProcAddress() const override
    {
        return nullptr;
    }

    bool vulkanSupported() const override
    {
        return true;
    }

    std::vector<std::string> requiredVulkanInstanceExtensions() const override
    {
        return {};
    }

    tinalux::platform::VulkanGetInstanceProcFn vulkanGetInstanceProcAddress() const override
    {
        return nullptr;
    }

    GraphicsAPI graphicsApi() const { return graphicsApi_; }
    void* nativeWindow() const { return nativeWindow_; }

private:
    GraphicsAPI graphicsApi_ = GraphicsAPI::OpenGL;
    int width_ = 0;
    int height_ = 0;
    void* nativeWindow_ = nullptr;
    float dpiScale_ = 1.0f;
    bool shouldClose_ = false;
    std::string clipboardText_;
    tinalux::platform::EventCallback callback_;
};

std::unique_ptr<tinalux::platform::Window> createFakeWindow(const tinalux::platform::WindowConfig& config)
{
    gWindowCreates.push_back(WindowCreateRecord {
        .graphicsApi = config.graphicsApi,
        .width = config.width,
        .height = config.height,
        .nativeWindow = config.android.has_value() ? config.android->nativeWindow : nullptr,
        .dpiScale = config.android.has_value() ? config.android->dpiScale : 1.0f,
    });
    auto window = std::make_unique<FakeWindow>(config);
    if (gShouldCloseWindow) {
        window->requestClose();
    }
    return window;
}

tinalux::rendering::RenderContext createFakeContext(const tinalux::rendering::ContextConfig& config)
{
    gContextRequests.push_back(config.backend);
    ++gContextCreates;
    return makeFakeContext(gContextCreates + 100);
}

tinalux::rendering::RenderSurface createFakeWindowSurface(
    tinalux::rendering::RenderContext&,
    tinalux::platform::Window&)
{
    ++gSurfaceCreates;
    return tinalux::rendering::createRasterSurface(64, 64);
}

void resetScenario()
{
    gWindowCreates.clear();
    gContextRequests.clear();
    gContextCreates = 0;
    gSurfaceCreates = 0;
    gShouldCloseWindow = false;
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

    {
        resetScenario();

        app::Application app;
        platform::WindowConfig initialWindowConfig {
            .width = 960,
            .height = 640,
            .title = "SuspendResumeSmoke",
        };
        expect(
            app.init(app::ApplicationConfig {
                .window = initialWindowConfig,
                .backend = Backend::Vulkan,
            }),
            "Application init should succeed for render suspend smoke");
        expect(
            app.renderBackend() == Backend::Vulkan,
            "Application should activate the requested Vulkan backend after init");
        expect(gWindowCreates.size() == 1, "Init should create exactly one window");
        expect(gContextCreates == 1, "Init should create exactly one render context");
        expect(gSurfaceCreates == 1, "Init should create exactly one render surface");

        app.suspendRendering();
        expect(
            app.renderBackend() == Backend::Auto,
            "suspendRendering should release the active render backend");

        platform::WindowConfig resumedWindowConfig = initialWindowConfig;
        resumedWindowConfig.width = 1280;
        resumedWindowConfig.height = 720;
        expect(
            app.resumeRendering(resumedWindowConfig),
            "resumeRendering should rebuild the render state");
        expect(
            app.renderBackend() == Backend::Vulkan,
            "resumeRendering should restore the Vulkan render backend");
        expect(gWindowCreates.size() == 2, "resumeRendering should create a second window");
        expect(gContextCreates == 2, "resumeRendering should create a second render context");
        expect(gSurfaceCreates == 2, "resumeRendering should create a second render surface");
        expect(
            gWindowCreates.back().graphicsApi == GraphicsAPI::None,
            "Vulkan resume should request a no-api window");
        expect(
            gWindowCreates.back().width == 1280 && gWindowCreates.back().height == 720,
            "resumeRendering should use the provided window dimensions");
    }

    {
        resetScenario();

        app::android::AndroidRuntime runtime;
        expect(
            runtime.preferredBackend() == Backend::OpenGL,
            "Default Android runtime should prefer the OpenGL backend");
        expect(
            runtime.attachWindow(reinterpret_cast<void*>(0x001), 1.0f),
            "Android runtime should attach successfully with the default config");
        expect(
            gWindowCreates.size() == 1 && gWindowCreates[0].graphicsApi == GraphicsAPI::OpenGL,
            "Default Android runtime config should request an OpenGL window");
        expect(
            gContextRequests.size() == 1 && gContextRequests[0] == Backend::OpenGL,
            "Default Android runtime config should request the OpenGL backend");
    }

    {
        resetScenario();

        app::android::AndroidRuntime runtime;
        expect(
            runtime.attachWindow(reinterpret_cast<void*>(0x002), 0.0f),
            "Android runtime should tolerate an invalid dpi scale by falling back to 1.0");
        expect(gWindowCreates.size() == 1, "Fallback dpi attach should still create one window");
        expect(
            gWindowCreates[0].dpiScale == 1.0f,
            "Android runtime should sanitize invalid dpi scales before window creation");
    }

    {
        resetScenario();

        app::android::AndroidRuntime runtime;
        app::android::AndroidRuntimeConfig config;
        config.application.window.width = 800;
        config.application.window.height = 600;
        config.application.backend = Backend::Vulkan;
        runtime.setConfig(config);
        expect(
            runtime.preferredBackend() == Backend::Vulkan,
            "Explicit runtime config should preserve the preferred Vulkan backend");

        expect(
            runtime.attachWindow(reinterpret_cast<void*>(0x101), 2.0f),
            "Android runtime should attach the first window");
        expect(runtime.ready(), "Android runtime should be ready after the first attach");
        expect(gWindowCreates.size() == 1, "First attach should create one window");
        expect(gContextRequests.size() == 1, "First attach should create one backend request");
        expect(gContextRequests[0] == Backend::Vulkan, "Explicit Vulkan config should request the Vulkan backend");
        expect(gContextCreates == 1, "First attach should create one render context");
        expect(gSurfaceCreates == 1, "First attach should create one render surface");
        expect(
            gWindowCreates.back().nativeWindow == reinterpret_cast<void*>(0x101),
            "First attach should forward the native window handle");
        expect(
            gWindowCreates.back().dpiScale == 2.0f,
            "First attach should forward the dpi scale");

        runtime.setClipboardText("persisted clipboard");
        expect(
            runtime.clipboardText() == "persisted clipboard",
            "Runtime clipboard should reflect the attached window state");

        runtime.detachWindow();
        expect(!runtime.ready(), "detachWindow should release the current render state");
        expect(
            runtime.clipboardText() == "persisted clipboard",
            "Clipboard text should survive detachWindow");

        expect(
            runtime.attachWindow(reinterpret_cast<void*>(0x202), 1.5f),
            "Android runtime should reattach a second window");
        expect(runtime.ready(), "Android runtime should be ready after reattach");
        expect(gWindowCreates.size() == 2, "Reattach should create a second window");
        expect(gContextRequests.size() == 2, "Reattach should create a second backend request");
        expect(gContextRequests[1] == Backend::Vulkan, "Reattach should preserve the explicit Vulkan backend");
        expect(gContextCreates == 2, "Reattach should create a second render context");
        expect(gSurfaceCreates == 2, "Reattach should create a second render surface");
        expect(
            gWindowCreates.back().nativeWindow == reinterpret_cast<void*>(0x202),
            "Reattach should use the new native window handle");
        expect(
            runtime.clipboardText() == "persisted clipboard",
            "Clipboard text should be restored after reattach");

        runtime.setSuspended(true);
        expect(runtime.suspended(), "setSuspended(true) should mark the runtime suspended");
        expect(!runtime.ready(), "Suspending should release the current render state");
        expect(
            runtime.clipboardText() == "persisted clipboard",
            "Clipboard text should survive runtime suspension");

        runtime.setSuspended(false);
        expect(!runtime.suspended(), "setSuspended(false) should clear the suspended state");
        expect(runtime.ready(), "Resuming should rebuild the render state");
        expect(gWindowCreates.size() == 3, "Resuming should create a new window");
        expect(gContextRequests.size() == 3, "Resuming should create a new backend request");
        expect(gContextRequests[2] == Backend::Vulkan, "Suspend/resume should preserve the explicit Vulkan backend");
        expect(gContextCreates == 3, "Resuming should create a new render context");
        expect(gSurfaceCreates == 3, "Resuming should create a new render surface");
        expect(
            runtime.clipboardText() == "persisted clipboard",
            "Clipboard text should survive suspend/resume");

        runtime.setPreferredBackend(Backend::OpenGL);
        expect(
            runtime.preferredBackend() == Backend::OpenGL,
            "setPreferredBackend should update the preferred backend");
        expect(
            gWindowCreates.size() == 4,
            "Switching backend while active should recreate the window");
        expect(
            gContextRequests.size() == 4 && gContextRequests[3] == Backend::OpenGL,
            "Switching backend while active should request the new backend");
    }

    {
        resetScenario();

        app::android::AndroidRuntime runtime;
        app::android::AndroidRuntimeConfig config;
        config.application.backend = Backend::Vulkan;
        runtime.setConfig(config);

        expect(
            runtime.attachWindow(reinterpret_cast<void*>(0x303), 1.0f),
            "Android runtime should attach before close lifecycle smoke");
        expect(runtime.ready(), "Runtime should be ready before requestClose");

        runtime.requestClose();

        expect(
            !runtime.renderOnce(),
            "renderOnce should report false after the application requests close");
        expect(!runtime.ready(), "Runtime should no longer be ready after application shutdown");

        expect(
            runtime.attachWindow(reinterpret_cast<void*>(0x404), 1.25f),
            "Android runtime should create a fresh application session after close");
        expect(runtime.ready(), "Runtime should be ready again after reattach");
        expect(gWindowCreates.size() == 2, "Close and reattach should create a second window");
        expect(gContextCreates == 2, "Close and reattach should create a second render context");
        expect(gSurfaceCreates == 2, "Close and reattach should create a second render surface");
    }

    return 0;
}
