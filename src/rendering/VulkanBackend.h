#pragma once

#include "FrameLifecycle.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::platform {

class Window;

}  // namespace tinalux::platform

namespace tinalux::rendering {

RenderSurface createVulkanWindowSurface(RenderContext& context, platform::Window& window);
FramePrepareStatus prepareVulkanFrame(RenderSurface& surface);
void flushVulkanFrame(RenderContext& context, RenderSurface& surface);

}  // namespace tinalux::rendering
