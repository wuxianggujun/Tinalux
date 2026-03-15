#include "tinalux/rendering/WindowSurface.h"

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace {

constexpr GrGLuint kDefaultFramebufferId = 0;
constexpr GrGLenum kDefaultFramebufferFormat = 0x8058;  // GL_RGBA8

}  // namespace

namespace tinalux::rendering {

RenderSurface createWindowSurface(const RenderContext& context, int width, int height)
{
    auto* skia = RenderAccess::skiaContext(context);
    if (skia == nullptr || width <= 0 || height <= 0) {
        core::logWarnCat(
            "render",
            "Skipping window surface creation because context={} width={} height={}",
            static_cast<const void*>(skia),
            width,
            height);
        return {};
    }

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = kDefaultFramebufferId;
    framebufferInfo.fFormat = kDefaultFramebufferFormat;

    GrBackendRenderTarget renderTarget =
        GrBackendRenderTargets::MakeGL(width, height, 0, 8, framebufferInfo);

    sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
        skia,
        renderTarget,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr);
    if (!surface) {
        core::logErrorCat(
            "render",
            "Failed to wrap backend render target into window surface {}x{}",
            width,
            height);
        return {};
    }

    core::logDebugCat("render", "Created window surface {}x{}", width, height);
    return RenderAccess::makeSurface(std::move(surface));
}

}  // namespace tinalux::rendering
