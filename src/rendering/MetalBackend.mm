#include "MetalBackend.h"

#if defined(__APPLE__)

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/ganesh/mtl/SkSurfaceMetal.h"
#include "MetalBackendState.h"
#include "tinalux/core/Log.h"
#include "tinalux/platform/Window.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace {

using tinalux::rendering::Backend;
using tinalux::rendering::MetalContextState;
using tinalux::rendering::MetalSurfaceState;
using tinalux::rendering::RenderAccess;
using tinalux::rendering::RenderContext;
using tinalux::rendering::RenderSurface;
using tinalux::rendering::SurfaceFailureReason;

void releaseRetainedHandle(void*& handle)
{
    if (handle == nullptr) {
        return;
    }

    [(id)handle release];
    handle = nullptr;
}

void destroyMetalContext(void*, void* userData)
{
    auto* state = static_cast<MetalContextState*>(userData);
    if (state == nullptr) {
        return;
    }

    if (state->queue != nullptr) {
        [(id<MTLCommandQueue>)state->queue release];
        state->queue = nullptr;
    }
    if (state->device != nullptr) {
        [(id<MTLDevice>)state->device release];
        state->device = nullptr;
    }

    delete state;
}

sk_sp<SkSurface> acquireMetalDrawableSurface(GrDirectContext* skiaContext, MetalSurfaceState& state)
{
    if (skiaContext == nullptr || !state.hasLayer()) {
        return nullptr;
    }

    GrMTLHandle drawableHandle = nullptr;
    sk_sp<SkSurface> surface = SkSurfaces::WrapCAMetalLayer(
        skiaContext,
        (GrMTLHandle)state.layerHandle(),
        kTopLeft_GrSurfaceOrigin,
        0,
        kBGRA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr,
        &drawableHandle);
    if (!surface || drawableHandle == nullptr) {
        return nullptr;
    }

    state.setDrawableHandle(const_cast<void*>(drawableHandle));
    return surface;
}

void destroyMetalSurface(void*, void* userData)
{
    auto* state = static_cast<MetalSurfaceState*>(userData);
    if (state == nullptr) {
        return;
    }

    tinalux::rendering::releaseMetalSurfaceDrawable(state);
    void* layerHandle = state->layerHandle();
    releaseRetainedHandle(layerHandle);
    state->setLayerHandle(layerHandle);
    delete state;
}

bool refreshMetalLayerState(RenderSurface& surface, MetalSurfaceState& state)
{
    if (state.windowHandle() == nullptr || state.contextState() == nullptr || state.contextState()->device == nullptr) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::BackendStateMissing);
        return false;
    }

    const int framebufferWidth = state.windowHandle()->framebufferWidth();
    const int framebufferHeight = state.windowHandle()->framebufferHeight();
    const float dpiScale = state.windowHandle()->dpiScale();
    const bool sizeChanged =
        state.framebufferWidth() != framebufferWidth || state.framebufferHeight() != framebufferHeight;
    const bool dpiChanged =
        dpiScale < state.scaleFactor() - 0.001f || dpiScale > state.scaleFactor() + 0.001f;
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        RenderAccess::clearMetalSurfaceFailures(surface);
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::ZeroFramebufferSize);
        core::logWarnCat(
            "render",
            "Skipping Metal drawable acquisition because framebuffer size is {}x{}",
            framebufferWidth,
            framebufferHeight);
        return false;
    }
    if (!state.windowHandle()->prepareMetalLayer(
            state.contextState()->device,
            framebufferWidth,
            framebufferHeight,
            dpiScale)) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::MetalLayerRefreshFailed);
        core::logWarnCat(
            "render",
            "Metal layer refresh failed for framebuffer {}x{} at dpi {:.2f}",
            framebufferWidth,
            framebufferHeight,
            dpiScale);
        return false;
    }

    void* latestLayer = state.windowHandle()->metalLayer();
    if (latestLayer == nullptr) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::MetalLayerMissing);
        core::logWarnCat("render", "Metal layer refresh succeeded but the platform window returned a null layer");
        return false;
    }

    const bool layerChanged = state.layerHandle() != latestLayer;
    if (state.layerHandle() != latestLayer) {
        void* layerHandle = state.layerHandle();
        releaseRetainedHandle(layerHandle);
        state.setLayerHandle((void*)[(id)latestLayer retain]);
    }
    state.updateMetrics(framebufferWidth, framebufferHeight, dpiScale);
    if (state.layerRefreshFailureCount() > 0) {
        core::logInfoCat(
            "render",
            "Metal layer refresh recovered after {} failed attempt(s)",
            state.layerRefreshFailureCount());
    }
    state.clearLayerRefreshFailures();
    if (layerChanged || sizeChanged || dpiChanged) {
        core::logInfoCat(
            "render",
            "Refreshed Metal layer state: layer_changed={} framebuffer={}x{} dpi={:.2f}",
            layerChanged,
            framebufferWidth,
            framebufferHeight,
            dpiScale);
    }
    return true;
}

}  // namespace

namespace tinalux::rendering {

RenderContext createMetalContextImpl()
{
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        core::logErrorCat("render", "Failed to create Metal context: no default MTLDevice is available");
        return {};
    }

    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) {
        core::logErrorCat("render", "Failed to create Metal context: MTLCommandQueue creation returned null");
        [device release];
        return {};
    }

    auto* state = new MetalContextState();
    state->device = (void*)device;
    state->queue = (void*)queue;

    GrMtlBackendContext backendContext = {};
    backendContext.fDevice.retain((GrMTLHandle)device);
    backendContext.fQueue.retain((GrMTLHandle)queue);

    sk_sp<GrDirectContext> directContext = GrDirectContexts::MakeMetal(backendContext);
    if (!directContext) {
        core::logErrorCat("render", "Failed to create Skia Ganesh Metal direct context");
        destroyMetalContext(nullptr, state);
        return {};
    }

    core::logInfoCat("render", "Created Skia Ganesh Metal direct context");
    return RenderAccess::makeContext(
        Backend::Metal,
        std::move(directContext),
        state,
        &destroyMetalContext,
        state);
}

RenderSurface createMetalWindowSurface(RenderContext& context, platform::Window& window)
{
    auto* metalContext = RenderAccess::metalState(context);
    auto* skiaContext = RenderAccess::skiaContext(context);
    if (metalContext == nullptr || skiaContext == nullptr) {
        core::logErrorCat(
            "render",
            "Cannot create '{}' window surface because Metal context state is incomplete",
            backendName(Backend::Metal));
        return {};
    }

    const int width = window.framebufferWidth();
    const int height = window.framebufferHeight();
    if (!window.prepareMetalLayer(metalContext->device, width, height, window.dpiScale())) {
        core::logErrorCat(
            "render",
            "Failed to prepare Metal layer for framebuffer {}x{}",
            width,
            height);
        return {};
    }

    void* layerHandle = window.metalLayer();
    if (layerHandle == nullptr) {
        core::logErrorCat("render", "Platform window did not provide a Metal layer");
        return {};
    }

    auto* state = new MetalSurfaceState();
    state->bind(metalContext, &window, skiaContext);
    state->setLayerHandle((void*)[(id)layerHandle retain]);
    state->updateMetrics(width, height, window.dpiScale());

    sk_sp<SkSurface> surface = acquireMetalDrawableSurface(skiaContext, *state);
    if (!surface) {
        core::logErrorCat(
            "render",
            "Failed to acquire initial CAMetalDrawable for framebuffer {}x{}",
            width,
            height);
        destroyMetalSurface(nullptr, state);
        return {};
    }

    core::logInfoCat("render", "Created Metal window surface {}x{}", width, height);
    return RenderAccess::makeSurface(
        Backend::Metal,
        std::move(surface),
        state,
        &destroyMetalSurface,
        state);
}

bool ensureMetalSurfaceDrawable(RenderSurface& surface)
{
    auto* metalSurface = RenderAccess::metalSurfaceState(surface);
    if (metalSurface == nullptr) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::BackendStateMissing);
        return false;
    }
    if (!RenderAccess::metalSurfaceValid(surface)) {
        return false;
    }
    if (metalSurface->skiaDirectContext() == nullptr) {
        core::logErrorCat("render", "Metal surface state lost its Skia context");
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::BackendStateMissing);
        RenderAccess::invalidateSurface(surface);
        return false;
    }
    if (RenderAccess::skiaSurface(surface) != nullptr && RenderAccess::metalSurfaceHasDrawable(surface)) {
        RenderAccess::clearMetalSurfaceFailures(surface);
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::None);
        return true;
    }
    if (!refreshMetalLayerState(surface, *metalSurface)) {
        const SurfaceFailureReason failureReason = lastSurfaceFailureReason(surface);
        if (failureReason == SurfaceFailureReason::BackendStateMissing) {
            RenderAccess::invalidateSurface(surface);
            return false;
        }
        if (failureReason == SurfaceFailureReason::ZeroFramebufferSize) {
            return false;
        }
        const int layerRefreshFailures = RenderAccess::incrementMetalLayerRefreshFailures(surface);
        core::logWarnCat(
            "render",
            "Metal layer refresh failed (reason='{}' attempt={} size={}x{} dpi={:.2f})",
            surfaceFailureReasonName(failureReason),
            layerRefreshFailures,
            metalSurface->framebufferWidth(),
            metalSurface->framebufferHeight(),
            metalSurface->scaleFactor());
        if (layerRefreshFailures >= 2) {
            core::logWarnCat(
                "render",
                "Metal layer refresh failed {} times with reason='{}'; invalidating the window surface",
                layerRefreshFailures,
                surfaceFailureReasonName(failureReason));
            RenderAccess::invalidateSurface(surface);
        }
        return false;
    }

    sk_sp<SkSurface> nextSurface = acquireMetalDrawableSurface(metalSurface->skiaDirectContext(), *metalSurface);
    if (!nextSurface) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::MetalDrawableUnavailable);
        const int drawableAcquireFailures = RenderAccess::incrementMetalDrawableAcquireFailures(surface);
        core::logWarnCat(
            "render",
            "Metal drawable acquisition failed (reason='{}' attempt={} size={}x{} dpi={:.2f})",
            surfaceFailureReasonName(lastSurfaceFailureReason(surface)),
            drawableAcquireFailures,
            metalSurface->framebufferWidth(),
            metalSurface->framebufferHeight(),
            metalSurface->scaleFactor());
        if (drawableAcquireFailures >= 2) {
            core::logWarnCat(
                "render",
                "Metal drawable acquisition failed {} times with reason='{}'; invalidating the window surface",
                drawableAcquireFailures,
                surfaceFailureReasonName(lastSurfaceFailureReason(surface)));
            RenderAccess::invalidateSurface(surface);
        }
        return false;
    }

    if (RenderAccess::metalDrawableAcquireFailures(surface) > 0) {
        core::logInfoCat(
            "render",
            "Metal drawable acquisition recovered after {} failed attempt(s)",
            RenderAccess::metalDrawableAcquireFailures(surface));
    }
    RenderAccess::clearMetalSurfaceFailures(surface);
    RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::None);
    RenderAccess::setSkiaSurface(surface, std::move(nextSurface));
    return true;
}

bool metalSurfaceAwaitingDrawable(const RenderSurface& surface)
{
    auto* metalSurface = RenderAccess::metalSurfaceState(surface);
    if (metalSurface == nullptr || !RenderAccess::metalSurfaceValid(surface)) {
        return false;
    }

    return RenderAccess::skiaSurface(surface) == nullptr || !RenderAccess::metalSurfaceHasDrawable(surface);
}

void flushMetalFrame(RenderContext& context, RenderSurface& surface)
{
    auto* skiaContext = RenderAccess::skiaContext(context);
    auto* metalContext = RenderAccess::metalState(context);
    auto* metalSurface = RenderAccess::metalSurfaceState(surface);
    SkSurface* skiaSurface = RenderAccess::skiaSurface(surface);
    if (skiaContext == nullptr || metalContext == nullptr || metalSurface == nullptr || skiaSurface == nullptr) {
        if (skiaContext != nullptr) {
            skiaContext->flushAndSubmit();
        }
        return;
    }

    if (!RenderAccess::metalSurfaceValid(surface) || !RenderAccess::metalSurfaceHasDrawable(surface) || skiaSurface == nullptr) {
        return;
    }

    skiaContext->flushAndSubmit(skiaSurface);

    id<MTLCommandQueue> queue = (id<MTLCommandQueue>)metalContext->queue;
    id<CAMetalDrawable> drawable = (id<CAMetalDrawable>)metalSurface->drawableHandle();
    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    if (commandBuffer == nil) {
        core::logErrorCat("render", "Failed to create Metal present command buffer");
        RenderAccess::setSurfaceFailureReason(
            surface,
            SurfaceFailureReason::MetalPresentCommandBufferUnavailable);
        RenderAccess::setSkiaSurface(surface, {});
        releaseMetalSurfaceDrawable(metalSurface);
        return;
    }

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];

    RenderAccess::setSkiaSurface(surface, {});
    releaseMetalSurfaceDrawable(metalSurface);
}

void releaseMetalSurfaceDrawable(MetalSurfaceState* state)
{
    if (state == nullptr || !state->hasDrawable()) {
        return;
    }

    [(id)state->drawableHandle() release];
    state->clearDrawableHandle();
}

}  // namespace tinalux::rendering

#endif
