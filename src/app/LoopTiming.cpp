#include "LoopTiming.h"

#include <algorithm>

namespace tinalux::app::detail {

EventLoopDecision chooseEventLoopDecision(
    bool immediateRenderWork,
    std::optional<double> animationDelaySeconds,
    double idleWaitSeconds)
{
    if (immediateRenderWork || (animationDelaySeconds.has_value() && *animationDelaySeconds <= 0.0)) {
        return {
            .mode = EventLoopMode::Poll,
            .waitSeconds = 0.0,
        };
    }

    return {
        .mode = EventLoopMode::Wait,
        .waitSeconds = animationDelaySeconds.has_value()
            ? std::clamp(*animationDelaySeconds, 0.0, idleWaitSeconds)
            : idleWaitSeconds,
    };
}

}  // namespace tinalux::app::detail
