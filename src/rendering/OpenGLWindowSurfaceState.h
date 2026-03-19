#pragma once

#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering::detail {

struct OpenGLWindowSurfaceHooks {
    void (*viewport)(int x, int y, int width, int height) = nullptr;
};

OpenGLWindowSurfaceHooks& openGLWindowSurfaceHooks();
void syncOpenGLWindowSurfaceState(RenderSurface& surface);

}  // namespace tinalux::rendering::detail
