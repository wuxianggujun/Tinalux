#include "tinalux/rendering/GLContext.h"

#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace {

struct ProcContext {
    // 通过外部注入的 getProc 取 GL 函数指针，避免 Rendering 依赖 Platform/GLFW。
    tinalux::rendering::GLGetProcFn getProc = nullptr;
};

GrGLFuncPtr getProcAddress(void* ctx, const char name[])
{
    auto* proc = static_cast<ProcContext*>(ctx);
    if (proc == nullptr || proc->getProc == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<GrGLFuncPtr>(proc->getProc(name));
}

}  // namespace

namespace tinalux::rendering {

RenderContext createGLContext(GLGetProcFn getProc)
{
    if (getProc == nullptr) {
        core::logErrorCat("render", "Cannot create GL context: getProc callback is null");
        return {};
    }

    ProcContext context{ getProc };
    sk_sp<const GrGLInterface> interface =
        GrGLMakeAssembledGLInterface(&context, &getProcAddress);
    if (!interface) {
        core::logErrorCat("render", "Failed to assemble OpenGL interface for Skia");
        return {};
    }

    sk_sp<GrDirectContext> directContext = GrDirectContexts::MakeGL(interface);
    if (!directContext) {
        core::logErrorCat("render", "Failed to create Skia Ganesh GL direct context");
        return {};
    }

    core::logInfoCat("render", "Created Skia Ganesh GL direct context");
    return RenderAccess::makeContext(std::move(directContext));
}

}  // namespace tinalux::rendering
