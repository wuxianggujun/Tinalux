#pragma once

#include "FrameLifecycle.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::platform {

class Window;

}  // namespace tinalux::platform

namespace tinalux::rendering {

bool canCreateVulkanContext(const ContextConfig& config);
RenderContext createVulkanContextImpl(const ContextConfig& config);
RenderSurface createVulkanWindowSurface(RenderContext& context, platform::Window& window);
FramePrepareStatus prepareVulkanFrame(RenderSurface& surface);
void flushVulkanFrame(RenderContext& context, RenderSurface& surface);

}  // namespace tinalux::rendering
