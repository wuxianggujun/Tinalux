#include "tinalux/rendering/WindowSurface.h"

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"

namespace {

constexpr GrGLuint kDefaultFramebufferId = 0;
constexpr GrGLenum kDefaultFramebufferFormat = 0x8058;  // GL_RGBA8

}  // namespace

namespace tinalux::rendering {

sk_sp<SkSurface> createWindowSurface(GrDirectContext* context, int width, int height)
{
    if (context == nullptr || width <= 0 || height <= 0) {
        return nullptr;
    }

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = kDefaultFramebufferId;
    framebufferInfo.fFormat = kDefaultFramebufferFormat;

    GrBackendRenderTarget renderTarget =
        GrBackendRenderTargets::MakeGL(width, height, 0, 8, framebufferInfo);

    return SkSurfaces::WrapBackendRenderTarget(
        context,
        renderTarget,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr);
}

}  // namespace tinalux::rendering
