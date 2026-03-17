#pragma once

#include <optional>

namespace tinalux::app::detail {

enum class EventLoopMode {
    Poll,
    Wait,
};

struct EventLoopDecision {
    EventLoopMode mode = EventLoopMode::Wait;
    double waitSeconds = 0.0;
};

EventLoopDecision chooseEventLoopDecision(
    bool immediateRenderWork,
    std::optional<double> animationDelaySeconds,
    double idleWaitSeconds);

}  // namespace tinalux::app::detail
