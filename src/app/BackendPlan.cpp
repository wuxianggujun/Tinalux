#include "BackendPlan.h"

namespace tinalux::app::detail {

platform::GraphicsAPI graphicsApiForBackend(rendering::Backend backend)
{
    switch (backend) {
    case rendering::Backend::Auto:
        return platform::GraphicsAPI::None;
    case rendering::Backend::OpenGL:
        return platform::GraphicsAPI::OpenGL;
    case rendering::Backend::Vulkan:
    case rendering::Backend::Metal:
        return platform::GraphicsAPI::None;
    default:
        return platform::GraphicsAPI::OpenGL;
    }
}

std::vector<rendering::Backend> backendCandidates(rendering::Backend requestedBackend)
{
    switch (requestedBackend) {
    case rendering::Backend::Auto:
#ifdef __APPLE__
        return {
            rendering::Backend::Metal,
            rendering::Backend::Vulkan,
            rendering::Backend::OpenGL,
        };
#elif defined(_WIN32)
        return {
            rendering::Backend::OpenGL,
            rendering::Backend::Vulkan,
        };
#else
        return {
            rendering::Backend::Vulkan,
            rendering::Backend::OpenGL,
        };
#endif
    case rendering::Backend::OpenGL:
    case rendering::Backend::Vulkan:
    case rendering::Backend::Metal:
        return { requestedBackend };
    default:
        return { rendering::Backend::OpenGL };
    }
}

BackendPlan::BackendPlan(rendering::Backend requestedBackend)
{
    reset(requestedBackend);
}

void BackendPlan::reset(rendering::Backend requestedBackend)
{
    requestedBackend_ = requestedBackend;
    candidates_ = backendCandidates(requestedBackend);
    activeIndex_ = 0;
}

rendering::Backend BackendPlan::requestedBackend() const
{
    return requestedBackend_;
}

const std::vector<rendering::Backend>& BackendPlan::candidates() const
{
    return candidates_;
}

std::size_t BackendPlan::activeIndex() const
{
    return activeIndex_;
}

rendering::Backend BackendPlan::activeBackend() const
{
    return activeIndex_ < candidates_.size()
        ? candidates_[activeIndex_]
        : rendering::Backend::Auto;
}

bool BackendPlan::activate(std::size_t index)
{
    if (index >= candidates_.size()) {
        return false;
    }

    activeIndex_ = index;
    return true;
}

bool BackendPlan::hasFallback() const
{
    return activeIndex_ + 1 < candidates_.size();
}

std::size_t BackendPlan::nextFallbackIndex() const
{
    return hasFallback() ? activeIndex_ + 1 : candidates_.size();
}

}  // namespace tinalux::app::detail
