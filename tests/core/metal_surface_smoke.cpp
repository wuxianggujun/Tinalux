#include <cstdlib>
#include <iostream>
#include <vector>

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

class FakeMetalWindow final : public tinalux::platform::Window {
public:
    bool shouldClose() const override { return false; }
    void pollEvents() override {}
    void waitEventsTimeout(double) override {}
    void swapBuffers() override {}
    void requestClose() override {}

    int width() const override { return 960; }
    int height() const override { return 640; }
    int framebufferWidth() const override { return framebufferWidth_; }
    int framebufferHeight() const override { return framebufferHeight_; }
    float dpiScale() const override { return dpiScale_; }

    void setClipboardText(const std::string&) override {}
    std::string clipboardText() const override { return {}; }
    void setEventCallback(tinalux::platform::EventCallback) override {}
    tinalux::platform::GLGetProcFn glGetProcAddress() const override { return nullptr; }

    bool prepareMetalLayer(
        void* device,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale) override
    {
        prepareCalls++;
        lastDevice = device;
        lastFramebufferWidth = framebufferWidth;
        lastFramebufferHeight = framebufferHeight;
        lastDpiScale = dpiScale;
        return prepareResult;
    }

    void* metalLayer() const override
    {
        return metalLayerHandle;
    }

    int framebufferWidth_ = 960;
    int framebufferHeight_ = 640;
    float dpiScale_ = 2.0f;
    bool prepareResult = false;
    void* metalLayerHandle = nullptr;
    int prepareCalls = 0;
    void* lastDevice = nullptr;
    int lastFramebufferWidth = 0;
    int lastFramebufferHeight = 0;
    float lastDpiScale = 0.0f;
};

}  // namespace

int main()
{
    using namespace tinalux::rendering;

#if defined(__APPLE__)
    RenderContext context = createContext(ContextConfig { .backend = Backend::Metal });
    expect(context, "Metal surface smoke should create a Metal direct context on Apple platforms");

    FakeMetalWindow prepareFailWindow;
    prepareFailWindow.prepareResult = false;
    RenderSurface prepareFailedSurface = createWindowSurface(context, prepareFailWindow);
    expect(!prepareFailedSurface, "Metal surface creation should fail when platform layer preparation fails");
    expect(prepareFailWindow.prepareCalls == 1, "Metal surface creation should ask the window to prepare a layer exactly once");
    expect(prepareFailWindow.lastDevice != nullptr, "Metal surface creation should pass a non-null Metal device to the platform window");
    expect(prepareFailWindow.lastFramebufferWidth == 960, "Metal surface creation should pass framebuffer width to the platform window");
    expect(prepareFailWindow.lastFramebufferHeight == 640, "Metal surface creation should pass framebuffer height to the platform window");
    expect(prepareFailWindow.lastDpiScale == 2.0f, "Metal surface creation should pass dpi scale to the platform window");

    FakeMetalWindow nullLayerWindow;
    nullLayerWindow.prepareResult = true;
    nullLayerWindow.metalLayerHandle = nullptr;
    RenderSurface nullLayerSurface = createWindowSurface(context, nullLayerWindow);
    expect(!nullLayerSurface, "Metal surface creation should fail when the platform window exposes no Metal layer");
    expect(nullLayerWindow.prepareCalls == 1, "Metal surface creation should still prepare the layer before reading the layer handle");
#else
    RenderContext context = createContext(ContextConfig { .backend = Backend::Metal });
    expect(!context, "Metal surface smoke should fail cleanly on non-Apple platforms");
#endif

    return 0;
}
