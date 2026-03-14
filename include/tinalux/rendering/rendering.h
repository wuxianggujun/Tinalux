#pragma once

#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"

namespace tinalux::rendering {

using GLGetProcFn = void* (*)(const char* name);

void initSkia();
void shutdownSkia();

sk_sp<GrDirectContext> createGLContext(GLGetProcFn getProc);
sk_sp<SkSurface> createWindowSurface(GrDirectContext* ctx, int framebufferWidth, int framebufferHeight);
void flushFrame(GrDirectContext* ctx);

}  // namespace tinalux::rendering
