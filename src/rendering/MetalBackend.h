#pragma once

#include "RenderHandles.h"

namespace tinalux::rendering {

RenderContext createMetalContextImpl();
RenderSurface createMetalWindowSurface(RenderContext& context, platform::Window& window);
bool ensureMetalSurfaceDrawable(RenderSurface& surface);
bool metalSurfaceAwaitingDrawable(const RenderSurface& surface);
void flushMetalFrame(RenderContext& context, RenderSurface& surface);
void releaseMetalSurfaceDrawable(MetalSurfaceState* state);

}  // namespace tinalux::rendering
