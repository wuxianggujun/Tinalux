#include <cstdlib>
#include <iostream>

#include "include/core/SkImageInfo.h"
#include "include/core/SkSurface.h"
#include "../../src/rendering/OpenGLWindowSurfaceState.h"
#include "../../src/rendering/RenderHandles.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct ViewportCapture {
    int x = -1;
    int y = -1;
    int width = -1;
    int height = -1;
    int viewportCalls = 0;
};

ViewportCapture* gCapture = nullptr;

void captureViewport(int x, int y, int width, int height)
{
    if (gCapture == nullptr) {
        return;
    }

    gCapture->x = x;
    gCapture->y = y;
    gCapture->width = width;
    gCapture->height = height;
    ++gCapture->viewportCalls;
}

}  // namespace

int main()
{
    using namespace tinalux::rendering;

    RenderSurface surface = RenderAccess::makeSurface(
        Backend::OpenGL,
        SkSurfaces::Raster(SkImageInfo::MakeN32Premul(1280, 720)));
    expect(static_cast<bool>(surface), "OpenGL smoke surface should be created");

    auto& hooks = detail::openGLWindowSurfaceHooks();
    const detail::OpenGLWindowSurfaceHooks originalHooks = hooks;

    ViewportCapture capture;
    gCapture = &capture;
    hooks.viewport = &captureViewport;

    detail::syncOpenGLWindowSurfaceState(surface);

    hooks = originalHooks;
    gCapture = nullptr;

    expect(capture.viewportCalls == 1, "Viewport sync should update the GL viewport once");
    expect(capture.x == 0 && capture.y == 0, "Viewport origin should stay at zero");
    expect(
        capture.width == 1280 && capture.height == 720,
        "Viewport size should match the wrapped surface dimensions");

    return 0;
}
