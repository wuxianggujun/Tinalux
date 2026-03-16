#include "tinalux/rendering/rendering.h"

#include <cstdint>
#include <utility>

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "FrameLifecycle.h"
#include "MetalBackend.h"
#include "RenderHandles.h"
#include "VulkanBackend.h"
#include "tinalux/core/Log.h"
#include "tinalux/platform/Window.h"

namespace {

constexpr GrGLuint kDefaultFramebufferId = 0;
constexpr GrGLenum kDefaultFramebufferFormat = 0x8058;  // GL_RGBA8

sk_sp<SkSurface> createOpenGLWindowSurface(GrDirectContext* context, int width, int height)
{
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

}  // namespace

namespace tinalux::rendering {

FramePrepareStatus prepareFrame(RenderContext& context, RenderSurface& surface)
{
    if (!context || !surface) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::InvalidContextOrSurface);
        return FramePrepareStatus::SurfaceLost;
    }

    switch (context.backend()) {
    case Backend::Metal:
        if (!metalSurfaceAwaitingDrawable(surface)) {
            RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::None);
            return FramePrepareStatus::Ready;
        }
        if (ensureMetalSurfaceDrawable(surface)) {
            return FramePrepareStatus::Ready;
        }
        return surface ? FramePrepareStatus::RetryLater : FramePrepareStatus::SurfaceLost;
    case Backend::OpenGL:
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::None);
        return surface ? FramePrepareStatus::Ready : FramePrepareStatus::SurfaceLost;
    case Backend::Vulkan:
        return prepareVulkanFrame(surface);
    case Backend::Auto:
    default:
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::BackendStateMissing);
        return FramePrepareStatus::SurfaceLost;
    }
}

RenderSurface createWindowSurface(RenderContext& context, platform::Window& window)
{
    const Backend backend = context.backend();
    const int width = window.framebufferWidth();
    const int height = window.framebufferHeight();
    if (width <= 0 || height <= 0) {
        core::logWarnCat(
            "render",
            "Skipping window surface creation because backend={} width={} height={}",
            backendName(backend),
            width,
            height);
        return {};
    }

    sk_sp<SkSurface> surface;
    switch (backend) {
    case Backend::Auto:
    case Backend::OpenGL:
        if (auto* skia = RenderAccess::skiaContext(context); skia != nullptr) {
            surface = createOpenGLWindowSurface(skia, width, height);
            break;
        }
        core::logWarnCat(
            "render",
            "Skipping '{}' window surface creation because Skia context is null",
            backendName(backend));
        return {};
    case Backend::Vulkan:
        return createVulkanWindowSurface(context, window);
    case Backend::Metal:
        return createMetalWindowSurface(context, window);
    default:
        core::logErrorCat("render", "Unknown render backend during window surface creation");
        return {};
    }

    if (!surface) {
        core::logErrorCat(
            "render",
            "Failed to create '{}' window surface {}x{}",
            backendName(backend),
            width,
            height);
        return {};
    }

    core::logDebugCat(
        "render",
        "Created '{}' window surface {}x{}",
        backendName(backend),
        width,
        height);
    return RenderAccess::makeSurface(backend, std::move(surface));
}

void flushFrame(RenderContext& context, RenderSurface& surface)
{
    auto* skia = RenderAccess::skiaContext(context);
    if (skia == nullptr) {
        return;
    }

    if (context.backend() == Backend::Metal) {
        flushMetalFrame(context, surface);
        return;
    }

    if (context.backend() == Backend::Vulkan) {
        flushVulkanFrame(context, surface);
        return;
    }

    skia->flushAndSubmit();
}

}  // namespace tinalux::rendering
