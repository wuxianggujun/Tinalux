#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "tinalux/app/Application.h"

#include "../../src/app/ApplicationTestAccess.h"
#include "../../src/app/RuntimeHooks.h"
#include "../../src/rendering/RenderHandles.h"
#include "tinalux/core/events/Event.h"
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

std::vector<GraphicsAPI> gWindowApis;
std::vector<Backend> gContextRequests;
std::size_t gSurfaceCreateCalls = 0;
std::size_t gPrepareFrameCalls = 0;
std::size_t gFlushFrameCalls = 0;
bool gFailFirstVulkanContext = false;
bool gFailSecondVulkanSurface = false;
bool gForceReadyPrepareFrame = false;
bool gInvalidateSurfaceOnFlush = false;
int gFramebufferWidthOverride = 0;
int gFramebufferHeightOverride = 0;
int gPendingFramebufferWidth = 0;
int gPendingFramebufferHeight = 0;
std::chrono::steady_clock::time_point gNow =
    std::chrono::steady_clock::time_point {} + std::chrono::seconds(1);

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
    {
    }

    bool shouldClose() const override { return false; }
    void pollEvents() override
    {
        applyPendingFramebufferMetrics();
    }
    void waitEventsTimeout(double) override
    {
        applyPendingFramebufferMetrics();
    }
    void swapBuffers() override {}
    void requestClose() override {}

    int width() const override { return width_; }
    int height() const override { return height_; }
    int framebufferWidth() const override
    {
        return gFramebufferWidthOverride > 0 ? gFramebufferWidthOverride : width_;
    }
    int framebufferHeight() const override
    {
        return gFramebufferHeightOverride > 0 ? gFramebufferHeightOverride : height_;
    }
    float dpiScale() const override { return 1.0f; }

    void setClipboardText(const std::string&) override {}
    std::string clipboardText() const override { return {}; }
    void setEventCallback(tinalux::platform::EventCallback callback) override
    {
        callback_ = std::move(callback);
    }
    tinalux::platform::GLGetProcFn glGetProcAddress() const override { return nullptr; }
    bool vulkanSupported() const override { return true; }
    std::vector<std::string> requiredVulkanInstanceExtensions() const override { return {}; }
    tinalux::platform::VulkanGetInstanceProcFn vulkanGetInstanceProcAddress() const override { return nullptr; }

    GraphicsAPI graphicsApi() const { return graphicsApi_; }

private:
    static void applyPendingFramebufferMetrics()
    {
        if (gPendingFramebufferWidth <= 0 || gPendingFramebufferHeight <= 0) {
            return;
        }

        gFramebufferWidthOverride = gPendingFramebufferWidth;
        gFramebufferHeightOverride = gPendingFramebufferHeight;
        gPendingFramebufferWidth = 0;
        gPendingFramebufferHeight = 0;
    }

    GraphicsAPI graphicsApi_ = GraphicsAPI::OpenGL;
    int width_ = 0;
    int height_ = 0;
    tinalux::platform::EventCallback callback_;
};

std::unique_ptr<tinalux::platform::Window> createFakeWindow(const tinalux::platform::WindowConfig& config)
{
    gWindowApis.push_back(config.graphicsApi);
    return std::make_unique<FakeWindow>(config);
}

tinalux::rendering::RenderContext createFakeContext(const tinalux::rendering::ContextConfig& config)
{
    gContextRequests.push_back(config.backend);
    if (gFailFirstVulkanContext
        && config.backend == Backend::Vulkan
        && gContextRequests.size() == 1) {
        return {};
    }
    return makeFakeContext(gContextRequests.size() + 1);
}

tinalux::rendering::RenderSurface createFakeWindowSurface(
    tinalux::rendering::RenderContext&,
    tinalux::platform::Window&)
{
    ++gSurfaceCreateCalls;
    if (gFailSecondVulkanSurface && gSurfaceCreateCalls == 2) {
        return {};
    }
    return tinalux::rendering::createRasterSurface(64, 64);
}

tinalux::rendering::FramePrepareStatus fakePrepareFrame(
    tinalux::rendering::RenderContext& context,
    tinalux::rendering::RenderSurface& surface)
{
    ++gPrepareFrameCalls;
    if (gForceReadyPrepareFrame) {
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::None);
        return tinalux::rendering::FramePrepareStatus::Ready;
    }
    return tinalux::rendering::prepareFrame(context, surface);
}

void fakeFlushFrame(tinalux::rendering::RenderContext& context, tinalux::rendering::RenderSurface& surface)
{
    ++gFlushFrameCalls;
    if (gInvalidateSurfaceOnFlush) {
        gInvalidateSurfaceOnFlush = false;
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanPresentOutOfDate);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return;
    }
    tinalux::rendering::flushFrame(context, surface);
}

std::chrono::steady_clock::time_point fakeNowSteadyTime()
{
    return gNow;
}

void resetScenario()
{
    gWindowApis.clear();
    gContextRequests.clear();
    gSurfaceCreateCalls = 0;
    gPrepareFrameCalls = 0;
    gFlushFrameCalls = 0;
    gFailFirstVulkanContext = false;
    gFailSecondVulkanSurface = false;
    gForceReadyPrepareFrame = false;
    gInvalidateSurfaceOnFlush = false;
    gFramebufferWidthOverride = 0;
    gFramebufferHeightOverride = 0;
    gPendingFramebufferWidth = 0;
    gPendingFramebufferHeight = 0;
    gNow = std::chrono::steady_clock::time_point {} + std::chrono::seconds(1);
}

}  // namespace

int main()
{
    using namespace tinalux;

    app::detail::RuntimeHooks hooks {
        .createWindow = &createFakeWindow,
        .createContext = &createFakeContext,
        .createWindowSurface = &createFakeWindowSurface,
        .nowSteadyTime = &fakeNowSteadyTime,
        .prepareFrame = &fakePrepareFrame,
        .flushFrame = &fakeFlushFrame,
    };
    app::detail::ScopedRuntimeHooksOverride scopedHooks(hooks);

    {
        resetScenario();
        gFailFirstVulkanContext = true;

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should fall back to the next backend when Vulkan context creation fails");
        expect(gWindowApis.size() == 2, "Auto backend init should create two windows during fallback");
        expect(gWindowApis[0] == GraphicsAPI::None, "Vulkan candidate should request a no-api window");
        expect(gWindowApis[1] == GraphicsAPI::OpenGL, "OpenGL fallback should request an OpenGL window");
        expect(gContextRequests.size() == 2, "Auto backend init should try two render backends");
        expect(gContextRequests[0] == Backend::Vulkan, "Auto backend init should try Vulkan first");
        expect(gContextRequests[1] == Backend::OpenGL, "Auto backend init should fall back to OpenGL second");
        expect(gSurfaceCreateCalls == 1, "Only the successful fallback backend should create an initial surface");
    }

    {
        resetScenario();
        gFailSecondVulkanSurface = true;

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should succeed on the first Vulkan attempt");
        expect(gWindowApis.size() == 1, "Initial Vulkan startup should create one window");
        expect(gContextRequests.size() == 1, "Initial Vulkan startup should create one context");
        expect(gSurfaceCreateCalls == 1, "Initial Vulkan startup should create one surface");

        gFramebufferWidthOverride = 1280;
        gFramebufferHeightOverride = 720;
        expect(
            app::detail::ApplicationTestAccess::renderFrame(app),
            "Render frame should recover by promoting the next backend when Vulkan surface recreation fails");
        expect(gWindowApis.size() == 2, "Runtime fallback should create a second window for the fallback backend");
        expect(gWindowApis[1] == GraphicsAPI::OpenGL, "Runtime fallback should recreate the window as OpenGL");
        expect(gContextRequests.size() == 2, "Runtime fallback should create a second context");
        expect(gContextRequests[1] == Backend::OpenGL, "Runtime fallback should try the OpenGL backend next");
        expect(gSurfaceCreateCalls == 3, "Runtime fallback should retry surface creation on the fallback backend");
        auto* finalWindow = dynamic_cast<FakeWindow*>(app.window());
        expect(finalWindow != nullptr, "Runtime fallback should leave a fake window instance active");
        expect(finalWindow->graphicsApi() == GraphicsAPI::OpenGL, "Runtime fallback should leave the OpenGL window active");
    }

    {
        resetScenario();

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should succeed for surface recreation smoke");
        expect(gSurfaceCreateCalls == 1, "Init should create exactly one initial surface");

        expect(
            app::detail::ApplicationTestAccess::renderFrame(app),
            "Render frame should succeed without recreating the surface when framebuffer size is unchanged");
        expect(
            gSurfaceCreateCalls == 1,
            "Stable framebuffer size should reuse the existing surface");

        gFramebufferWidthOverride = 1440;
        gFramebufferHeightOverride = 900;
        expect(
            app::detail::ApplicationTestAccess::renderFrame(app),
            "Render frame should recreate the surface when framebuffer size changes");
        expect(
            gSurfaceCreateCalls == 2,
            "Framebuffer resize should trigger exactly one surface recreation");
        expect(
            gWindowApis.size() == 1,
            "Surface recreation without backend failure should not create a second window");
        expect(
            gContextRequests.size() == 1,
            "Surface recreation without backend failure should not create a second context");
    }

    {
        resetScenario();

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should succeed for metrics-driven redraw smoke");
        expect(
            gSurfaceCreateCalls == 1,
            "Init should create exactly one initial surface before metrics-driven redraw smoke");

        gNow += std::chrono::milliseconds(1);
        gPendingFramebufferWidth = 1600;
        gPendingFramebufferHeight = 900;
        expect(
            app.pumpOnce(),
            "Pump loop should keep the application alive when framebuffer metrics change during event polling");
        expect(
            gSurfaceCreateCalls == 1,
            "Interactive framebuffer metric changes should coalesce surface recreation inside the resize interval");

        gNow += std::chrono::milliseconds(4);
        gPendingFramebufferWidth = 1700;
        gPendingFramebufferHeight = 960;
        expect(
            app.pumpOnce(),
            "Pump loop should stay alive while coalescing another framebuffer metric change");
        expect(
            gSurfaceCreateCalls == 1,
            "Multiple framebuffer metric changes inside the coalescing interval should still reuse the current surface");

        gNow += std::chrono::milliseconds(20);
        expect(
            app.pumpOnce(),
            "Pump loop should recreate the surface once the resize coalescing interval elapses");
        expect(
            gSurfaceCreateCalls == 2,
            "Coalesced framebuffer metric changes should trigger exactly one deferred surface recreation");
        expect(
            gWindowApis.size() == 1,
            "Metrics-driven redraw should not create a second window");
        expect(
            gContextRequests.size() == 1,
            "Metrics-driven redraw should not create a second context");
    }

    {
        resetScenario();

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should succeed for resize-followed surface loss coalescing smoke");
        expect(
            gSurfaceCreateCalls == 1,
            "Init should create exactly one initial surface before resize-followed surface loss coalescing smoke");

        gNow += std::chrono::milliseconds(20);
        gPendingFramebufferWidth = 1600;
        gPendingFramebufferHeight = 900;
        expect(
            app.pumpOnce(),
            "Pump loop should stay alive when resize-triggered recreation is immediately followed by backend-reported surface loss");
        expect(
            gSurfaceCreateCalls == 2,
            "Resize-triggered recreation should still recreate the surface once before exercising backend-reported surface loss coalescing");

        gNow += std::chrono::milliseconds(4);
        expect(
            app.pumpOnce(),
            "Pump loop should keep coalescing repeated backend-reported surface loss inside the interval");
        expect(
            gSurfaceCreateCalls == 2,
            "Repeated backend-reported surface loss inside the coalescing interval should still reuse the current recreate budget");

        gNow += std::chrono::milliseconds(20);
        expect(
            app.pumpOnce(),
            "Pump loop should recreate the surface once the backend-reported surface loss coalescing interval elapses");
        expect(
            gSurfaceCreateCalls == 3,
            "Coalesced backend-reported surface loss should trigger exactly one deferred retry after the resize-triggered recreation");
    }

    {
        resetScenario();
        gFailSecondVulkanSurface = true;

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should succeed for deferred frame stats smoke");
        const app::FrameStats statsBeforeFallback = app.frameStats();
        expect(
            statsBeforeFallback.totalFrames == 0,
            "Fallback smoke should start with zero recorded rendered frames");
        expect(
            statsBeforeFallback.skippedFrames == 0,
            "Fallback smoke should start with zero skipped frames");

        gFramebufferWidthOverride = 1280;
        gFramebufferHeightOverride = 720;
        core::WindowResizeEvent resizeEvent(gFramebufferWidthOverride, gFramebufferHeightOverride);
        app.handleEvent(resizeEvent);

        expect(app.pumpOnce(), "Runtime fallback frame should keep the application alive");
        const app::FrameStats statsAfterFallback = app.frameStats();
        expect(
            statsAfterFallback.totalFrames == statsBeforeFallback.totalFrames,
            "Backend recovery without a presented frame should not increment rendered frame count");
        expect(
            statsAfterFallback.skippedFrames == statsBeforeFallback.skippedFrames + 1,
            "Backend recovery without a presented frame should be recorded as a skipped frame");
    }

    {
        resetScenario();
        gForceReadyPrepareFrame = true;
        gInvalidateSurfaceOnFlush = true;

        app::Application app;
        expect(
            app.init(app::ApplicationConfig { .backend = Backend::Auto }),
            "Auto backend init should succeed for present-loss redraw retention smoke");
        expect(gSurfaceCreateCalls == 1, "Init should create one surface before present-loss redraw retention smoke");

        expect(
            app.pumpOnce(),
            "Pump loop should stay alive when flush invalidates the surface after a prepared frame");
        const app::FrameStats statsAfterPresentLoss = app.frameStats();
        expect(
            statsAfterPresentLoss.totalFrames == 0,
            "A frame invalidated during flush should not be counted as presented");
        expect(
            statsAfterPresentLoss.skippedFrames == 1,
            "A frame invalidated during flush should be recorded as skipped");
        expect(
            gSurfaceCreateCalls == 1,
            "Flush-stage surface invalidation should not recreate the surface until the next frame");
        expect(
            gFlushFrameCalls == 1,
            "Flush-stage surface invalidation smoke should exercise the flush hook exactly once on the first frame");

        expect(
            app.pumpOnce(),
            "Pump loop should retry rendering on the next frame after flush invalidates the surface");
        const app::FrameStats statsAfterRetry = app.frameStats();
        expect(
            statsAfterRetry.totalFrames == 1,
            "The next frame should present successfully after recreating the invalidated surface");
        expect(
            statsAfterRetry.skippedFrames == 1,
            "Successful retry after flush invalidation should preserve the earlier skipped-frame accounting");
        expect(
            gSurfaceCreateCalls == 2,
            "Retry after flush-stage surface invalidation should recreate the surface exactly once");
    }

    return 0;
}
