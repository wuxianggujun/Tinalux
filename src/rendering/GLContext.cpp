#include "tinalux/rendering/GLContext.h"

#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"

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

sk_sp<GrDirectContext> createGLContext(GLGetProcFn getProc)
{
    if (getProc == nullptr) {
        return nullptr;
    }

    ProcContext context{ getProc };
    sk_sp<const GrGLInterface> interface =
        GrGLMakeAssembledGLInterface(&context, &getProcAddress);
    if (!interface) {
        return nullptr;
    }

    return GrDirectContexts::MakeGL(interface);
}

}  // namespace tinalux::rendering
