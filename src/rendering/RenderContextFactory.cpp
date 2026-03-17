#include "tinalux/rendering/rendering.h"

#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "MetalBackend.h"
#include "RenderHandles.h"
#include "VulkanBackend.h"
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

bool canCreateOpenGLContext(tinalux::rendering::GLGetProcFn getProc)
{
    return getProc != nullptr;
}

}  // namespace

namespace tinalux::rendering {

namespace {

RenderContext createOpenGLContextImpl(GLGetProcFn getProc)
{
    if (getProc == nullptr) {
        core::logErrorCat("render", "Cannot create GL context: getProc callback is null");
        return {};
    }

    ProcContext context{ getProc };
    sk_sp<const GrGLInterface> interface =
        GrGLMakeAssembledInterface(&context, &getProcAddress);
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
    return RenderAccess::makeContext(Backend::OpenGL, std::move(directContext));
}

}  // namespace

RenderContext createContext(const ContextConfig& config)
{
    switch (config.backend) {
    case Backend::Auto:
    {
#if defined(__APPLE__)
        if (RenderContext metalContext = createMetalContextImpl()) {
            core::logInfoCat("render", "Render backend selection: Auto -> Metal");
            return metalContext;
        }
#endif
        if (canCreateVulkanContext(config)) {
            if (RenderContext vulkanContext = createVulkanContextImpl(config)) {
                core::logInfoCat("render", "Render backend selection: Auto -> Vulkan");
                return vulkanContext;
            }
        } else {
            core::logDebugCat(
                "render",
                "Render backend selection: Auto skipped Vulkan because bootstrap callbacks or extensions are missing");
        }
        if (canCreateOpenGLContext(config.glGetProc)) {
            if (RenderContext openGlContext = createOpenGLContextImpl(config.glGetProc)) {
                core::logInfoCat("render", "Render backend selection: Auto -> OpenGL");
                return openGlContext;
            }
        } else {
            core::logDebugCat(
                "render",
                "Render backend selection: Auto skipped OpenGL because the GL proc loader callback is null");
        }
        core::logErrorCat("render", "Render backend selection: Auto found no available backend");
        return {};
    }
    case Backend::OpenGL:
        return createOpenGLContextImpl(config.glGetProc);
    case Backend::Vulkan:
        return createVulkanContextImpl(config);
    case Backend::Metal:
        return createMetalContextImpl();
    default:
        core::logErrorCat("render", "Unknown render backend selection");
        return {};
    }
}

}  // namespace tinalux::rendering
