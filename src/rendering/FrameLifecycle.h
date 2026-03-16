#pragma once

#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering {

enum class FramePrepareStatus {
    Ready,
    RetryLater,
    SurfaceLost,
};

enum class SurfaceFailureReason {
    None,
    InvalidContextOrSurface,
    BackendStateMissing,
    ZeroFramebufferSize,
    VulkanAcquireSemaphoreMissing,
    VulkanAcquireSemaphoreCreationFailed,
    VulkanAcquireOutOfDate,
    VulkanAcquireFailed,
    VulkanImageOutOfRange,
    VulkanImageSurfaceMissing,
    VulkanSkiaWaitRejected,
    VulkanPresentOutOfDate,
    VulkanPresentFailed,
    MetalLayerRefreshFailed,
    MetalLayerMissing,
    MetalDrawableUnavailable,
    MetalPresentCommandBufferUnavailable,
};

FramePrepareStatus prepareFrame(RenderContext& context, RenderSurface& surface);
SurfaceFailureReason lastSurfaceFailureReason(const RenderSurface& surface);
const char* surfaceFailureReasonName(SurfaceFailureReason reason);

}  // namespace tinalux::rendering
