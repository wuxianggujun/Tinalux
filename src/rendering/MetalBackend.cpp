#include "MetalBackend.h"

#include "tinalux/core/Log.h"

namespace tinalux::rendering {

RenderContext createMetalContextImpl()
{
    core::logErrorCat(
        "render",
        "Render backend selection requested '{}', but Metal is only available on Apple platforms",
        backendName(Backend::Metal));
    return {};
}

RenderSurface createMetalWindowSurface(RenderContext&, platform::Window&)
{
    core::logErrorCat(
        "render",
        "Window surface creation for backend '{}' is only available on Apple platforms",
        backendName(Backend::Metal));
    return {};
}

bool ensureMetalSurfaceDrawable(RenderSurface&)
{
    return false;
}

bool metalSurfaceAwaitingDrawable(const RenderSurface& surface)
{
    return RenderAccess::metalSurfaceValid(surface) && RenderAccess::skiaSurface(surface) == nullptr;
}

void flushMetalFrame(RenderContext& context, RenderSurface&)
{
    if (auto* skia = RenderAccess::skiaContext(context); skia != nullptr) {
        skia->flushAndSubmit();
    }
}

void releaseMetalSurfaceDrawable(MetalSurfaceState*)
{
}

}  // namespace tinalux::rendering
