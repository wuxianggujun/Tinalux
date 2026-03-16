#include <cstdlib>
#include <iostream>

#include "../../src/rendering/FrameLifecycle.h"
#include "../../src/rendering/MetalBackendState.h"
#include "../../src/rendering/RenderHandles.h"
#include "../../src/rendering/VulkanBackendState.h"
#include "tinalux/rendering/rendering.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void destroyDeferredVulkanSurface(void*, void* userData)
{
    delete static_cast<tinalux::rendering::VulkanSwapchainState*>(userData);
}

void destroyDeferredMetalSurface(void*, void* userData)
{
    delete static_cast<tinalux::rendering::MetalSurfaceState*>(userData);
}

void destroyFakeContextHandle(void*, void*) {}

tinalux::rendering::RenderContext makeFakeVulkanContext()
{
    using namespace tinalux::rendering;

    return RenderAccess::makeContext(
        Backend::Vulkan,
        nullptr,
        reinterpret_cast<void*>(0x1003),
        &destroyFakeContextHandle,
        nullptr);
}

tinalux::rendering::RenderSurface makeDeferredVulkanSurface()
{
    using namespace tinalux::rendering;

    auto* state = new VulkanSwapchainState();
    return RenderAccess::makeSurface(
        Backend::Vulkan,
        nullptr,
        reinterpret_cast<void*>(0x1001),
        &destroyDeferredVulkanSurface,
        state);
}

tinalux::rendering::RenderSurface makeDeferredMetalSurface()
{
    using namespace tinalux::rendering;

    auto* state = new MetalSurfaceState();
    return RenderAccess::makeSurface(
        Backend::Metal,
        nullptr,
        reinterpret_cast<void*>(0x1002),
        &destroyDeferredMetalSurface,
        state);
}

}  // namespace

int main()
{
    using namespace tinalux::rendering;

    RenderContext invalidContext;
    RenderSurface invalidSurface;
    expect(
        prepareFrame(invalidContext, invalidSurface) == FramePrepareStatus::SurfaceLost,
        "Preparing a frame without a context or surface should report SurfaceLost");

    RenderSurface deferredVulkanSurface = makeDeferredVulkanSurface();
    expect(
        static_cast<bool>(deferredVulkanSurface),
        "Deferred Vulkan surface should still be a valid handle");
    expect(
        !deferredVulkanSurface.canvas(),
        "Deferred Vulkan surface should not expose a canvas before prepareFrame");
    expect(
        !deferredVulkanSurface.snapshotImage(),
        "Deferred Vulkan surface should not expose a snapshot before prepareFrame");
    expect(
        prepareFrame(invalidContext, deferredVulkanSurface) == FramePrepareStatus::SurfaceLost,
        "Preparing a deferred Vulkan surface with an invalid context should report SurfaceLost");
    expect(
        lastSurfaceFailureReason(deferredVulkanSurface) == SurfaceFailureReason::InvalidContextOrSurface,
        "Invalid context should record an explicit surface failure reason");

    RenderContext fakeVulkanContext = makeFakeVulkanContext();
    RenderSurface brokenVulkanSurface = makeDeferredVulkanSurface();
    expect(
        prepareFrame(fakeVulkanContext, brokenVulkanSurface) == FramePrepareStatus::SurfaceLost,
        "Half-initialized Vulkan swapchain state should be treated as SurfaceLost");
    expect(
        lastSurfaceFailureReason(brokenVulkanSurface) == SurfaceFailureReason::BackendStateMissing,
        "Broken Vulkan swapchain state should record BackendStateMissing");
    expect(
        !static_cast<bool>(brokenVulkanSurface),
        "Fatal Vulkan frame preparation failure should invalidate the surface handle");

    RenderSurface deferredMetalSurface = makeDeferredMetalSurface();
    expect(
        static_cast<bool>(deferredMetalSurface),
        "Deferred Metal surface should still be a valid handle");
    expect(
        !deferredMetalSurface.canvas(),
        "Deferred Metal surface should not expose a canvas before prepareFrame");
    expect(
        !deferredMetalSurface.snapshotImage(),
        "Deferred Metal surface should not expose a snapshot before prepareFrame");

    return 0;
}
