#pragma once

#include <cstddef>
#include <vector>

#include "tinalux/platform/Window.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::app::detail {

platform::GraphicsAPI graphicsApiForBackend(rendering::Backend backend);
std::vector<rendering::Backend> backendCandidates(rendering::Backend requestedBackend);

class BackendPlan {
public:
    BackendPlan() = default;
    explicit BackendPlan(rendering::Backend requestedBackend);

    void reset(rendering::Backend requestedBackend);

    rendering::Backend requestedBackend() const;
    const std::vector<rendering::Backend>& candidates() const;
    std::size_t activeIndex() const;
    rendering::Backend activeBackend() const;

    bool activate(std::size_t index);
    bool hasFallback() const;
    std::size_t nextFallbackIndex() const;

private:
    rendering::Backend requestedBackend_ = rendering::Backend::Auto;
    std::vector<rendering::Backend> candidates_;
    std::size_t activeIndex_ = 0;
};

}  // namespace tinalux::app::detail
