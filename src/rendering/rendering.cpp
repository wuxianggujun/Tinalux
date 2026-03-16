#include "tinalux/rendering/rendering.h"

#include <algorithm>
#include <utility>

#include "FontCache.h"
#include "FrameLifecycle.h"
#include "MetalBackendState.h"
#include "VulkanBackendState.h"
#include "../core/SkiaGeometry.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "MetalBackend.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace tinalux::rendering {

namespace {

SkPaint makePaint(core::Color color, PaintStyle style, float strokeWidth)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(core::toSkColor(color));
    if (style == PaintStyle::Stroke) {
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(strokeWidth);
    }
    return paint;
}

}  // namespace

bool RenderContext::Impl::valid() const
{
    switch (backend) {
    case Backend::Auto:
        return false;
    case Backend::OpenGL:
        return skiaContext != nullptr;
    case Backend::Vulkan:
        return nativeHandle != nullptr;
    case Backend::Metal:
        return skiaContext != nullptr
            && nativeHandle != nullptr
            && metalState != nullptr
            && metalState->device != nullptr
            && metalState->queue != nullptr;
    default:
        return false;
    }
}

bool RenderSurface::Impl::valid() const
{
    switch (backend) {
    case Backend::Auto:
        return false;
    case Backend::OpenGL:
        return skiaSurface != nullptr;
    case Backend::Vulkan:
        return nativeHandle != nullptr
            && vulkanSwapchainState != nullptr
            && vulkanSwapchainState->isValid();
    case Backend::Metal:
        return nativeHandle != nullptr
            && metalSurfaceState != nullptr
            && metalSurfaceState->isValid();
    default:
        return false;
    }
}

Canvas::Canvas(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

Canvas::~Canvas() = default;

Canvas::operator bool() const
{
    return impl_ != nullptr && impl_->skiaCanvas != nullptr;
}

bool Canvas::quickReject(core::Rect rect) const
{
    auto* skia = RenderAccess::skiaCanvas(*this);
    return skia == nullptr || skia->quickReject(core::toSkRect(rect));
}

void Canvas::save()
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->save();
    }
}

void Canvas::restore()
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->restore();
    }
}

void Canvas::translate(float dx, float dy)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->translate(dx, dy);
    }
}

void Canvas::clipRect(core::Rect rect)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->clipRect(core::toSkRect(rect));
    }
}

void Canvas::clear(core::Color color)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->clear(core::toSkColor(color));
    }
}

void Canvas::clearRect(core::Rect rect, core::Color color)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setColor(core::toSkColor(color));
        skia->save();
        skia->clipRect(core::toSkRect(rect));
        skia->drawPaint(paint);
        skia->restore();
    }
}

void Canvas::drawRect(core::Rect rect, core::Color color, PaintStyle style, float strokeWidth)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->drawRect(core::toSkRect(rect), makePaint(color, style, strokeWidth));
    }
}

void Canvas::drawRoundRect(
    core::Rect rect,
    float radiusX,
    float radiusY,
    core::Color color,
    PaintStyle style,
    float strokeWidth)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->drawRRect(
            SkRRect::MakeRectXY(core::toSkRect(rect), radiusX, radiusY),
            makePaint(color, style, strokeWidth));
    }
}

void Canvas::drawCircle(
    float centerX,
    float centerY,
    float radius,
    core::Color color,
    PaintStyle style,
    float strokeWidth)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->drawCircle(centerX, centerY, radius, makePaint(color, style, strokeWidth));
    }
}

void Canvas::drawLine(
    float x0,
    float y0,
    float x1,
    float y1,
    core::Color color,
    float strokeWidth,
    bool roundCap)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        SkPaint paint = makePaint(color, PaintStyle::Stroke, strokeWidth);
        if (roundCap) {
            paint.setStrokeCap(SkPaint::kRound_Cap);
        }
        skia->drawLine(x0, y0, x1, y1, paint);
    }
}

void Canvas::drawText(
    std::string_view text,
    float x,
    float y,
    float fontSize,
    core::Color color)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        const SkFont& font = cachedFont(fontSize);
        SkPaint paint = makePaint(color, PaintStyle::Fill, 1.0f);
        skia->drawString(text.data(), x, y, font, paint);
    }
}

void Canvas::drawImage(const Image& image, core::Rect dest, float opacity)
{
    auto* skia = RenderAccess::skiaCanvas(*this);
    auto* skiaImage = RenderAccess::skiaImage(image);
    if (skia == nullptr || skiaImage == nullptr || dest.isEmpty()) {
        return;
    }

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setAlphaf(std::clamp(opacity, 0.0f, 1.0f));
    skia->drawImageRect(
        skiaImage,
        SkRect::MakeWH(static_cast<float>(skiaImage->width()), static_cast<float>(skiaImage->height())),
        core::toSkRect(dest),
        SkSamplingOptions(),
        &paint,
        SkCanvas::kFast_SrcRectConstraint);
}

RenderContext::RenderContext(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

RenderContext::~RenderContext() = default;

RenderContext::operator bool() const
{
    return impl_ != nullptr && impl_->valid();
}

Backend RenderContext::backend() const
{
    return impl_ != nullptr ? impl_->backend : Backend::Auto;
}

RenderSurface::RenderSurface(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

RenderSurface::~RenderSurface() = default;

RenderSurface::operator bool() const
{
    return impl_ != nullptr && impl_->valid();
}

Canvas RenderSurface::canvas() const
{
    return impl_ != nullptr && impl_->skiaSurface != nullptr
        ? RenderAccess::makeCanvas(impl_->skiaSurface->getCanvas())
        : Canvas {};
}

Image RenderSurface::snapshotImage() const
{
    if (impl_ == nullptr || impl_->skiaSurface == nullptr) {
        return {};
    }

    return RenderAccess::makeImage(impl_->skiaSurface->makeImageSnapshot());
}

Canvas RenderAccess::makeCanvas(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return {};
    }

    auto impl = std::make_shared<Canvas::Impl>();
    impl->skiaCanvas = canvas;
    return Canvas(std::move(impl));
}

Image RenderAccess::makeImage(sk_sp<SkImage> image)
{
    if (!image) {
        return {};
    }

    auto impl = std::make_shared<Image::Impl>();
    impl->skiaImage = std::move(image);
    return Image(std::move(impl));
}

RenderContext RenderAccess::makeContext(
    Backend backend,
    sk_sp<GrDirectContext> skiaContext,
    void* nativeHandle,
    NativeHandleDestroyFn destroyNativeHandle,
    void* destroyNativeHandleUserData)
{
    auto impl = std::make_shared<RenderContext::Impl>();
    impl->backend = backend;
    impl->skiaContext = std::move(skiaContext);
    impl->nativeHandle = nativeHandle;
    impl->destroyNativeHandle = destroyNativeHandle;
    impl->destroyNativeHandleUserData = destroyNativeHandleUserData;
    if (backend == Backend::Vulkan) {
        impl->vulkanState = static_cast<VulkanContextState*>(destroyNativeHandleUserData);
    } else if (backend == Backend::Metal) {
        impl->metalState = static_cast<MetalContextState*>(destroyNativeHandleUserData);
    }
    if (!impl->valid()) {
        return {};
    }
    return RenderContext(std::move(impl));
}

RenderSurface RenderAccess::makeSurface(
    Backend backend,
    sk_sp<SkSurface> skiaSurface,
    void* nativeHandle,
    NativeHandleDestroyFn destroyNativeHandle,
    void* destroyNativeHandleUserData)
{
    auto impl = std::make_shared<RenderSurface::Impl>();
    impl->backend = backend;
    impl->skiaSurface = std::move(skiaSurface);
    impl->nativeHandle = nativeHandle;
    impl->destroyNativeHandle = destroyNativeHandle;
    impl->destroyNativeHandleUserData = destroyNativeHandleUserData;
    if (backend == Backend::Vulkan) {
        impl->vulkanSwapchainState = static_cast<VulkanSwapchainState*>(destroyNativeHandleUserData);
    } else if (backend == Backend::Metal) {
        impl->metalSurfaceState = static_cast<MetalSurfaceState*>(destroyNativeHandleUserData);
    }
    if (!impl->valid()) {
        return {};
    }
    return RenderSurface(std::move(impl));
}

SkCanvas* RenderAccess::skiaCanvas(const Canvas& canvas)
{
    return canvas.impl_ != nullptr ? canvas.impl_->skiaCanvas : nullptr;
}

SkImage* RenderAccess::skiaImage(const Image& image)
{
    return image.impl_ != nullptr ? image.impl_->skiaImage.get() : nullptr;
}

GrDirectContext* RenderAccess::skiaContext(const RenderContext& context)
{
    return context.impl_ != nullptr ? context.impl_->skiaContext.get() : nullptr;
}

SkSurface* RenderAccess::skiaSurface(const RenderSurface& surface)
{
    return surface.impl_ != nullptr ? surface.impl_->skiaSurface.get() : nullptr;
}

void* RenderAccess::nativeHandle(const RenderContext& context)
{
    return context.impl_ != nullptr ? context.impl_->nativeHandle : nullptr;
}

VulkanContextState* RenderAccess::vulkanState(const RenderContext& context)
{
    return context.impl_ != nullptr ? context.impl_->vulkanState : nullptr;
}

MetalContextState* RenderAccess::metalState(const RenderContext& context)
{
    return context.impl_ != nullptr ? context.impl_->metalState : nullptr;
}

void RenderAccess::setSkiaContext(RenderContext& context, sk_sp<GrDirectContext> skiaContext)
{
    if (context.impl_ != nullptr) {
        context.impl_->skiaContext = std::move(skiaContext);
    }
}

VulkanSwapchainState* RenderAccess::vulkanSwapchainState(const RenderSurface& surface)
{
    return surface.impl_ != nullptr ? surface.impl_->vulkanSwapchainState : nullptr;
}

bool RenderAccess::vulkanSurfaceValid(const RenderSurface& surface)
{
    auto* state = vulkanSwapchainState(surface);
    return state != nullptr && state->isValid();
}

bool RenderAccess::vulkanFrameAcquired(const RenderSurface& surface)
{
    auto* state = vulkanSwapchainState(surface);
    return state != nullptr && state->hasAcquiredFrame();
}

uint32_t RenderAccess::vulkanCurrentImageIndex(const RenderSurface& surface)
{
    auto* state = vulkanSwapchainState(surface);
    return state != nullptr ? state->acquiredImageIndex() : VulkanSwapchainState::kInvalidImageIndex;
}

void RenderAccess::markVulkanFrameAcquired(
    RenderSurface& surface,
    uint32_t imageIndex,
    sk_sp<SkSurface> skiaSurface)
{
    auto* state = vulkanSwapchainState(surface);
    if (state == nullptr) {
        return;
    }

    state->markFrameAcquired(imageIndex);
    setSkiaSurface(surface, std::move(skiaSurface));
}

void RenderAccess::clearVulkanFrame(RenderSurface& surface)
{
    auto* state = vulkanSwapchainState(surface);
    if (state == nullptr) {
        return;
    }

    state->clearFrame();
}

void RenderAccess::invalidateVulkanSurfaceState(RenderSurface& surface)
{
    auto* state = vulkanSwapchainState(surface);
    if (state == nullptr) {
        return;
    }

    state->invalidate();
}

MetalSurfaceState* RenderAccess::metalSurfaceState(const RenderSurface& surface)
{
    return surface.impl_ != nullptr ? surface.impl_->metalSurfaceState : nullptr;
}

bool RenderAccess::metalSurfaceValid(const RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    return state != nullptr && state->isValid();
}

bool RenderAccess::metalSurfaceHasDrawable(const RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    return state != nullptr && state->hasDrawable();
}

int RenderAccess::metalLayerRefreshFailures(const RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    return state != nullptr ? state->layerRefreshFailureCount() : 0;
}

int RenderAccess::metalDrawableAcquireFailures(const RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    return state != nullptr ? state->drawableAcquireFailureCount() : 0;
}

int RenderAccess::incrementMetalLayerRefreshFailures(RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    return state != nullptr ? state->noteLayerRefreshFailure() : 0;
}

int RenderAccess::incrementMetalDrawableAcquireFailures(RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    return state != nullptr ? state->noteDrawableAcquireFailure() : 0;
}

void RenderAccess::clearMetalSurfaceFailures(RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    if (state == nullptr) {
        return;
    }

    state->clearFailureCounts();
}

void RenderAccess::invalidateMetalSurfaceState(RenderSurface& surface)
{
    auto* state = metalSurfaceState(surface);
    if (state == nullptr) {
        return;
    }

    state->invalidate();
}

void RenderAccess::setSkiaSurface(RenderSurface& surface, sk_sp<SkSurface> skiaSurface)
{
    if (surface.impl_ != nullptr) {
        surface.impl_->skiaSurface = std::move(skiaSurface);
        if (surface.impl_->skiaSurface != nullptr) {
            surface.impl_->failureReason = SurfaceFailureReason::None;
        }
    }
}

SurfaceFailureReason RenderAccess::surfaceFailureReason(const RenderSurface& surface)
{
    return surface.impl_ != nullptr ? surface.impl_->failureReason : SurfaceFailureReason::None;
}

void RenderAccess::setSurfaceFailureReason(RenderSurface& surface, SurfaceFailureReason reason)
{
    if (surface.impl_ != nullptr) {
        surface.impl_->failureReason = reason;
    }
}

void RenderAccess::invalidateSurface(RenderSurface& surface)
{
    if (surface.impl_ == nullptr) {
        return;
    }

    surface.impl_->skiaSurface.reset();
    if (surface.impl_->vulkanSwapchainState != nullptr) {
        invalidateVulkanSurfaceState(surface);
    }
    if (surface.impl_->metalSurfaceState != nullptr) {
        releaseMetalSurfaceDrawable(surface.impl_->metalSurfaceState);
        invalidateMetalSurfaceState(surface);
    }
}

SurfaceFailureReason lastSurfaceFailureReason(const RenderSurface& surface)
{
    return RenderAccess::surfaceFailureReason(surface);
}

const char* surfaceFailureReasonName(SurfaceFailureReason reason)
{
    switch (reason) {
    case SurfaceFailureReason::None:
        return "none";
    case SurfaceFailureReason::InvalidContextOrSurface:
        return "invalid_context_or_surface";
    case SurfaceFailureReason::BackendStateMissing:
        return "backend_state_missing";
    case SurfaceFailureReason::ZeroFramebufferSize:
        return "zero_framebuffer_size";
    case SurfaceFailureReason::VulkanAcquireSemaphoreMissing:
        return "vulkan_acquire_semaphore_missing";
    case SurfaceFailureReason::VulkanAcquireSemaphoreCreationFailed:
        return "vulkan_acquire_semaphore_creation_failed";
    case SurfaceFailureReason::VulkanAcquireOutOfDate:
        return "vulkan_acquire_out_of_date";
    case SurfaceFailureReason::VulkanAcquireFailed:
        return "vulkan_acquire_failed";
    case SurfaceFailureReason::VulkanImageOutOfRange:
        return "vulkan_image_out_of_range";
    case SurfaceFailureReason::VulkanImageSurfaceMissing:
        return "vulkan_image_surface_missing";
    case SurfaceFailureReason::VulkanSkiaWaitRejected:
        return "vulkan_skia_wait_rejected";
    case SurfaceFailureReason::VulkanPresentOutOfDate:
        return "vulkan_present_out_of_date";
    case SurfaceFailureReason::VulkanPresentFailed:
        return "vulkan_present_failed";
    case SurfaceFailureReason::MetalLayerRefreshFailed:
        return "metal_layer_refresh_failed";
    case SurfaceFailureReason::MetalLayerMissing:
        return "metal_layer_missing";
    case SurfaceFailureReason::MetalDrawableUnavailable:
        return "metal_drawable_unavailable";
    case SurfaceFailureReason::MetalPresentCommandBufferUnavailable:
        return "metal_present_command_buffer_unavailable";
    default:
        return "unknown";
    }
}

void initSkia()
{
    SkGraphics::Init();
    core::logDebugCat("render", "Skia runtime initialized");
}

void shutdownSkia()
{
    clearCachedFonts();
    SkGraphics::PurgeAllCaches();
    core::logDebugCat("render", "Skia runtime caches purged");
}

const char* backendName(Backend backend)
{
    switch (backend) {
    case Backend::Auto:
        return "Auto";
    case Backend::OpenGL:
        return "OpenGL";
    case Backend::Vulkan:
        return "Vulkan";
    case Backend::Metal:
        return "Metal";
    default:
        return "Unknown";
    }
}

}  // namespace tinalux::rendering
