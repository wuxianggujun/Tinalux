#pragma once

#include <memory>

#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "FrameLifecycle.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering {

using NativeHandleDestroyFn = void (*)(void* handle, void* userData);

struct VulkanContextState;
struct VulkanSwapchainState;
struct MetalContextState;
struct MetalSurfaceState;

struct Canvas::Impl {
    SkCanvas* skiaCanvas = nullptr;
};

struct Image::Impl {
    sk_sp<SkImage> skiaImage;
};

struct RenderContext::Impl {
    sk_sp<GrDirectContext> skiaContext;
    Backend backend = Backend::Auto;
    void* nativeHandle = nullptr;
    NativeHandleDestroyFn destroyNativeHandle = nullptr;
    void* destroyNativeHandleUserData = nullptr;
    VulkanContextState* vulkanState = nullptr;
    MetalContextState* metalState = nullptr;

    ~Impl()
    {
        skiaContext.reset();
        if (destroyNativeHandle != nullptr && nativeHandle != nullptr) {
            destroyNativeHandle(nativeHandle, destroyNativeHandleUserData);
        }
    }

    bool valid() const;
};

struct RenderSurface::Impl {
    sk_sp<SkSurface> skiaSurface;
    Backend backend = Backend::Auto;
    SurfaceFailureReason failureReason = SurfaceFailureReason::None;
    void* nativeHandle = nullptr;
    NativeHandleDestroyFn destroyNativeHandle = nullptr;
    void* destroyNativeHandleUserData = nullptr;
    VulkanSwapchainState* vulkanSwapchainState = nullptr;
    MetalSurfaceState* metalSurfaceState = nullptr;

    ~Impl()
    {
        skiaSurface.reset();
        if (destroyNativeHandle != nullptr && nativeHandle != nullptr) {
            destroyNativeHandle(nativeHandle, destroyNativeHandleUserData);
        }
    }

    bool valid() const;
};

struct RenderAccess {
    static Canvas makeCanvas(SkCanvas* canvas);
    static Image makeImage(sk_sp<SkImage> image);
    static RenderContext makeContext(
        Backend backend,
        sk_sp<GrDirectContext> skiaContext = nullptr,
        void* nativeHandle = nullptr,
        NativeHandleDestroyFn destroyNativeHandle = nullptr,
        void* destroyNativeHandleUserData = nullptr);
    static RenderSurface makeSurface(
        Backend backend,
        sk_sp<SkSurface> skiaSurface = nullptr,
        void* nativeHandle = nullptr,
        NativeHandleDestroyFn destroyNativeHandle = nullptr,
        void* destroyNativeHandleUserData = nullptr);
    static SkCanvas* skiaCanvas(const Canvas& canvas);
    static SkImage* skiaImage(const Image& image);
    static GrDirectContext* skiaContext(const RenderContext& context);
    static SkSurface* skiaSurface(const RenderSurface& surface);
    static void* nativeHandle(const RenderContext& context);
    static VulkanContextState* vulkanState(const RenderContext& context);
    static MetalContextState* metalState(const RenderContext& context);
    static void setSkiaContext(RenderContext& context, sk_sp<GrDirectContext> skiaContext);
    static VulkanSwapchainState* vulkanSwapchainState(const RenderSurface& surface);
    static bool vulkanSurfaceValid(const RenderSurface& surface);
    static bool vulkanFrameAcquired(const RenderSurface& surface);
    static uint32_t vulkanCurrentImageIndex(const RenderSurface& surface);
    static void markVulkanFrameAcquired(
        RenderSurface& surface,
        uint32_t imageIndex,
        sk_sp<SkSurface> skiaSurface);
    static void clearVulkanFrame(RenderSurface& surface);
    static void invalidateVulkanSurfaceState(RenderSurface& surface);
    static MetalSurfaceState* metalSurfaceState(const RenderSurface& surface);
    static bool metalSurfaceValid(const RenderSurface& surface);
    static bool metalSurfaceHasDrawable(const RenderSurface& surface);
    static int metalLayerRefreshFailures(const RenderSurface& surface);
    static int metalDrawableAcquireFailures(const RenderSurface& surface);
    static int incrementMetalLayerRefreshFailures(RenderSurface& surface);
    static int incrementMetalDrawableAcquireFailures(RenderSurface& surface);
    static void clearMetalSurfaceFailures(RenderSurface& surface);
    static void invalidateMetalSurfaceState(RenderSurface& surface);
    static void setSkiaSurface(RenderSurface& surface, sk_sp<SkSurface> skiaSurface);
    static SurfaceFailureReason surfaceFailureReason(const RenderSurface& surface);
    static void setSurfaceFailureReason(RenderSurface& surface, SurfaceFailureReason reason);
    static void invalidateSurface(RenderSurface& surface);
};

}  // namespace tinalux::rendering
